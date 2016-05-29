/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * clientthread.c: Implementierung des Client-Threads
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <time.h>

#include "../common/server_loader_protocol.h"
#include "../common/util.h"
#include "clientthread.h"
#include "catalog.h"
#include "rfc.h"
#include "user.h"
#include "login.h"
#include "main.h"

static sem_t *sem;
int stdinPipe[2];
int stdoutPipe[2];

void *clientThread(void *arg)
{
	sem = mainGetSemaphor();
	int userId = *(int*) arg;

	if(userId == 0)
	{
		clientGamemaster();
	}
	else
	{
		clientRegularPlayer(userId);
	}

	if(userGetGameState() == 3)
	{
		mainCleanup();
	}
	return NULL;
}

int clientRegularPlayer(int userId)
{
	int socketId;
	int res;
	struct timespec spec;
	header head;

	userLockData();
	socketId = userGetPlayer(userId).playerSocket;
	userUnlockData();

	fd_set fds;
	spec.tv_sec = 1;
	spec.tv_nsec = 0;


	read(socketId,&head,sizeof(head));
	if(head.type == CatalogRequest && (ntohs(head.length) == 0))
	{
		rfcSendCatalogResponse(socketId);
	}


	while(head.type != 8 && userGetGameState() != 1)		//Question request already read
	{
		FD_ZERO(&fds);
		FD_SET(socketId,&fds);
		res = pselect(socketId + 1, &fds, NULL, NULL, &spec, NULL);
		if(res == -1)
		{
			perror("error pselect : ");
		}
		if(FD_ISSET(socketId,&fds))
		{
			if( (read(socketId,&head,sizeof(head)) == 0 ) )	//player left the game
			{
				printf("\nPlayer left\n");
				userRemovePlayer(userId);
				sem_post(sem);
				return -1;
			}
		}
	}

	printf("\nStarte Spielphase\n");
	clientGamePhase(userId, socketId);
	return 0;
}

int clientGamemaster(void)
{
	int socketId;
	char *filename;
	header catalogRequest;
	header head;
	catalogChange *cch = NULL;
	startGame *stg = NULL;

	userLockData();
	socketId = userGetPlayer(0).playerSocket;
	userUnlockData();
	catalogInit();

	read(socketId, &catalogRequest,sizeof(catalogRequest));
	if(catalogRequest.type == CatalogRequest && (ntohs(catalogRequest.length) == 0) )
	{
		rfcSendCatalogResponse(socketId);
	}

	while( userGetGameState() == 0 )
	{
		if ( (read(socketId, &head, sizeof(head)) == 0) )		//gamemaster left
		{
			userRemovePlayer(0);
			return -1;
		}
		else if(head.type == CatalogChange)
		{
			cch = rfcReadCatalaogChange(socketId, head);
			rfcSendCatalogChangeBroadcast(cch);
			free(cch);
		}
		else if(head.type == StartGame)
		{
			stg = rfcReadStartGame(socketId, head);
			if(userGetActivePlayers() < 2)
			{
				rfcSendErrorMessage(socketId, "Nicht genug Spieler",warning);
			}
			else
			{
				filename = malloc(ntohs(stg->length) + 1);
				memcpy(filename,stg->filename,ntohs(stg->length));
				filename[ntohs(stg->length) + 1] = '\0';
				if(catalogLoad(filename) != 1)
				{
					rfcSendErrorMessage(socketId, "Katalog kann nicht geladen werden", warning);
				}
				else
				{
					catalogBindSharedMemory();		//insert error handling
					rfcSendStartGameBroadcast();
					userSetGameState(1);
					clientGamePhase(0, socketId);
				}
			}
			free(filename);
			free(stg);
		}
	}

	return 0;
}

int clientGamePhase(int id, int socket)
{
	Question question;
	questionAnswered answer;
	header head;
	fd_set fds;
	struct timespec spec;
	struct timespec start, end;
	int questionNumber = 0;
	int res;
	unsigned long score;
	unsigned long time = 0;

	spec.tv_nsec = 0;
	FD_ZERO(&fds);
	FD_SET(socket,&fds);

	sem_post(sem);

	//question request was already received in clientRegularPhase
	if(id != 0)
	{
		question = catalogReadQuestion(questionNumber);
		rfcSendQuestion(question, socket);
		clock_gettime(CLOCK_MONOTONIC, &start);
		spec.tv_sec = question.timeout;
		questionNumber++;
	}
	else
	{
		spec.tv_sec = 5;
	}


	while(questionNumber <= catalogGetParamesters().numberOfQuestions)
	{
		res = pselect(socket + 1, &fds, NULL, NULL, &spec, NULL);
		{
			if(res == -1)
			{
				perror("error pselect game phase player");
				return - 1;
			}
			if(FD_ISSET(socket,&fds))
			{
				if( (read(socket, &head, sizeof(header)) == 0) )
				{
					userRemovePlayer(id);
					sem_post(sem);
					return -1;
				}
				else if(head.type == QuestionRequest && (questionNumber < catalogGetParamesters().numberOfQuestions) )		//more questions in catalog
				{
					question = catalogReadQuestion(questionNumber);
					rfcSendQuestion(question, socket);
					//startzeit setzen
					clock_gettime(CLOCK_MONOTONIC, &start);
					spec.tv_sec = question.timeout;
					questionNumber++;
				}
				else if(head.type == QuestionRequest && (questionNumber == catalogGetParamesters().numberOfQuestions) )		//all questions send
				{
					rfcSendNoQuestionsleft(socket);
					questionNumber++;
				}
				else if(head.type == QuestionAnswered)
				{
					//endzeit nehmen
					clock_gettime(CLOCK_MONOTONIC, &end);
					time = end.tv_sec - start.tv_sec;
					answer = rfcReadQuestionAnswered(socket,head);
					if(answer.answer == question.correct)		//answer was correct and in time
					{
						score = scoreForTimeLeft(question.timeout - time,question.timeout);
						rfcSendQuestionResult(socket,question.correct,0);
						userSetScore(id,score);
						sem_post(sem);
					}
					else										//answer was wrong and in time
					{
						rfcSendQuestionResult(socket,question.correct,0);
					}
				}
			}
			else		//timelimit reached
			{
				rfcSendQuestionResult(socket,question.correct,1);
			}
		}
		FD_ZERO(&fds);
		FD_SET(socket,&fds);
	}

	userSetUserFinished(id);
	clientGameEndPhase(socket, id);

	return 0;
}

int clientGameEndPhase(int socket, int id)
{
	while(userGetActivePlayers() != userGetFinishedPlayers())
	{

	}
	rfcSendGameOverMessage(socket, id);
	userSetGameState(3);
	return 0;
}
unsigned long scoreForTimeLeft(unsigned long timeLeft,unsigned long timeout)
{
	unsigned long score = (timeLeft * 1000UL)/timeout; /* auf 10er-Stellen runden */
	score = ((score+5UL)/10UL)*10UL;
	return score;
}

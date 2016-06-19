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

/*
 * void *clientThread(void *arg)
 *
 * This function represents a client thread. After every successful login
 * of a user one client thread is created.
 *
 */
void *clientThread(void *arg)
{
	sem = mainGetSemaphor();
	int userId = *(int*) arg;		//get the user id (thread parameter)

	if(userId == 0)					//user is gamemaster
	{
		clientGamemaster();
	}
	else							//user is regular player
	{
		clientRegularPlayer(userId);
	}

	if(userGetGameState() == 2 && userId == 0)
	{
		mainCleanup();
	}

	return NULL;
}

/*
 * int clientRegularPlayer(int userId)
 *
 * This function handles a regular player (not gamemaster) during the
 * preparation stage of the game. During this phase of the game the
 * gamemaster is choosing the catalog etc..
 *
 * Parameter:
 *int userId = user id of the corrosponding (necassary for identification)
 *
 */
int clientRegularPlayer(int userId)
{
	int socketId;
	//int res;
	//struct timespec spec;
	header head;

	userLockData();
	socketId = userGetPlayer(userId).playerSocket;		//get corrosponding socket descriptor
	userUnlockData();

	/*//preparation for pselect function
	fd_set fds;			//file descriptor set
	spec.tv_sec = 1;	//time intervall 1 second
	spec.tv_nsec = 0;*/


	read(socketId,&head,sizeof(head));
	if(head.type == CatalogRequest && (ntohs(head.length) == 0))	//catalog request received
	{
		rfcSendCatalogResponse(socketId);
	}


	while(head.type != 8 && userGetGameState() != 1)		//wait for question request and start of game
	{
		if( (read(socketId,&head,sizeof(head)) == 0 ) )		//player meanwhile has left the game
		{
			printf("\nPlayer left\n");
			userRemovePlayer(userId);
			sem_post(sem);
			return -1;
		}
	}

	/*while(head.type != 8 && userGetGameState() != 1)		//Question request already read
	{
		FD_ZERO(&fds);
		FD_SET(socketId,&fds);		//add socket descriptor to filedescriptor set
		res = pselect(socketId + 1, &fds, NULL, NULL, &spec, NULL);
		if(res == -1)
		{
			perror("error pselect : ");
		}
		if(FD_ISSET(socketId,&fds))
		{
			if( (read(socketId,&head,sizeof(head)) == 0 ) )		//player has left the game (connection closed)
			{
				printf("\nPlayer left\n");
				userRemovePlayer(userId);
				sem_post(sem);
				return -1;
			}
		}
	}*/

	clientGamePhase(userId, socketId);		//enter the game phase
	return 0;
}

/*
 * int clientGamemaster(void)
 *
 * This function handles the the requests of the gamemaster during the preparation phase.
 * During this phase the gamemaster is able to pick the catalog etc..
 * Parameter:
 *
 *
 */
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
		if ( (read(socketId, &head, sizeof(head)) == 0) )		//gamemaster left take => terminate server
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
			if(userGetActivePlayers() < 2)				//playing against yourself sucks (atleast in this game ;-))
			{
				rfcSendErrorMessage(socketId, "Nicht genug Spieler",warning);
			}
			else
			{
				filename = malloc(ntohs(stg->length) + 1);
				memcpy(filename,stg->filename,ntohs(stg->length));	//copy filename out of the stg message
				filename[ntohs(stg->length)] = '\0';
				if(catalogLoad(filename) != 1)						//tell the loader process to load the catalog into shmem
				{
					rfcSendErrorMessage(socketId, "Katalog kann nicht geladen werden", warning);
				}
				else
				{
					catalogBindSharedMemory();
					rfcSendStartGameBroadcast();
					userSetGameState(1);
					clientGamePhase(0, socketId);		//go from init phase to actual game phase
				}
			}
			free(filename);
			free(stg);
		}
	}

	return 0;
}

/*
 * int clientGamePhase(int id, int socket)
 *
 * This function handles the game phase of the game were every player is answering the questions
 * from the picked catalog.
 *
 * Parameters:
 * int id = id of the user
 * int socket = the socket connectetion to the player
 *
 */
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

	//question request was already received in clientRegularPhase => get first question and send it to client
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
		spec.tv_sec = 5;		//gamemaster(necassary because no time was set for first question questionrequest still unread)
	}


	while(questionNumber <= catalogGetParameters().numberOfQuestions)
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
				if( (read(socket, &head, sizeof(header)) == 0) )		//player has left the game
				{
					userRemovePlayer(id);
					sem_post(sem);
					return -1;
				}
				else if(head.type == QuestionRequest && (questionNumber < catalogGetParameters().numberOfQuestions) )		//more questions in catalog
				{
					question = catalogReadQuestion(questionNumber);
					rfcSendQuestion(question, socket);
					clock_gettime(CLOCK_MONOTONIC, &start);		//get starttime
					spec.tv_sec = question.timeout;				//set question timeout
					questionNumber++;
				}
				else if(head.type == QuestionRequest && (questionNumber == catalogGetParameters().numberOfQuestions) )		//all questions send
				{
					rfcSendNoQuestionsleft(socket);
					questionNumber++;
				}
				else if(head.type == QuestionAnswered)
				{
					clock_gettime(CLOCK_MONOTONIC, &end);			//get timestamp when question was answered
					time = end.tv_sec - start.tv_sec;				//calculate the elapsed time
					answer = rfcReadQuestionAnswered(socket,head);
					if(answer.answer == question.correct)			//answer was correct and in time
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
		FD_ZERO(&fds);			//reset pselect filedescriptorlist
		FD_SET(socket,&fds);	//set fildescriptorlist
	}

	userSetUserFinished(id);
	clientGameEndPhase(socket, id);

	return 0;
}

/*
 * int clientGameEndPhase(int socket, int id)
 *
 * This function gets active as soon as aplayer has answered all of the questions.
 * Inside the function the thread is waiting until all players have finished the
 * quiz.
 *
 * Parameters:
 * int id = id of the user
 * int socket = the socket connectetion to the player
 *
 */
int clientGameEndPhase(int socket, int id)
{
	while(userGetActivePlayers() != userGetFinishedPlayers())		//wait until all players have finished the game
	{

	}
	rfcSendGameOverMessage(socket, id);
	userSetGameState(2);
	return 0;
}

/*
 * unsigned long scoreForTimeLeft(unsigned long timeLeft,unsigned long timeout)
 *
 * This function calculates the score as soon as a question was answered right.
 *
 * Return value:
 * unsigned long = the reached score
 *
 * Parameters:
 * unsigned long timeLeft = the time that was left for answering the question
 * iunsigned long timeout = timeout of the answerd question
 *
 */
unsigned long scoreForTimeLeft(unsigned long timeLeft,unsigned long timeout)
{
	unsigned long score = (timeLeft * 1000UL)/timeout; /* auf 10er-Stellen runden */
	score = ((score+5UL)/10UL)*10UL;
	return score;
}

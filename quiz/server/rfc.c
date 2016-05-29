/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * rfc.c: Implementierung der Funktionen zum Senden und Empfangen von
 * Datenpaketen gemäß dem RFC
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "catalog.h"
#include "user.h"
#include "rfc.h"


/*
 * void rfcSendLoginResOk(int socket, int id)
 *
 * This function sends a login response ok message after
 * a successful login to the client.
 *
 * Parameter:
 * int socket = the socket descriptor from the client
 * int id = the client's id (id = 0 => gamemaster)
 *
 */
void rfcSendLoginResOk(int socket, int id)
{
	loginResOk msg;

	msg.type = LoginResponseOk;
	msg.length = htons(2);
	msg.version = 8;
	msg.clientId = id;

	write(socket, &msg,sizeof(msg));
}

/*
 * void rfcSendErrorMessage(int socket, char message[], int subtype)
 *
 * This function sends an error message to a client.
 *
 * Parameter:
 * int socket = the socket descriptor from the client
 * char message[] = the message which gets send to the client
 * int subtype = warning or fatal (definition see rfc.h)
 *
 */
void rfcSendErrorMessage(int socket, char message[], int subtype)
{
	errorMsg *msg;

	int length = strlen(message);
	msg = malloc( sizeof(errorMsg) + length);

	msg->type = ErrorWarning;
	msg->length = htons(length + 1);
	msg->subtype = subtype;
	memcpy(msg->message, message,length);

	write(socket,msg,sizeof(errorMsg) + length);

	free(msg);

}

/*
 * void rfcSendListMessage(int socket[], playerListElement list[], int numberOfPlayers)
 *
 * This function sends a list message to all active players.
 * This list message contains the name and the corresponding
 * score of all players.
 *
 * Parameter:
 * int socket[] = list with the socket descriptors of all active players
 * playerListElement list[] = list of all active players (name + score)
 * int numberOfPlayers = number of active players
 *
 */
void rfcSendListMessage(int socket[], playerListElement list[], int numberOfPlayers)
{
	listMsg *msg;
	msg = malloc(sizeof(listMsg) + numberOfPlayers *37);

	msg->type = PlayerList;
	msg->length = htons(numberOfPlayers * 37);

	memcpy(msg->list,list,numberOfPlayers * 37);

	for(int i=0;i<numberOfPlayers;i++)
	{
		msg->list[i].score = htonl(msg->list[i].score);
	}

	for(int i=0; i<numberOfPlayers; i++)
	{
		write(socket[i],msg,sizeof(listMsg) + 37 * numberOfPlayers);
	}

	free(msg);
}

/*
 * void rfcSendCatalogResponse(int socket)
 *
 * This function sends a list of all availiable catalog names to a
 * client.
 *
 * Parameter:
 * int socket = socket descriptor of client
 *
 */
void rfcSendCatalogResponse(int socket)
{
	catalogResponse *msg = NULL;
	char *catalogList = catalogGetList();		//get a list of all availiable catalogs
	int i = 0;
	int j = 0;

	while(catalogList[i] != '\0')				//send the first n - 1 catalog names to client
	{
		if(catalogList[i] == '\n')
		{
			msg = malloc(sizeof(catalogResponse) + (i - j));
			msg->type = CatalogResponse;
			msg->length = htons(i-j);
			memcpy(msg->filename, catalogList + j, i-j);
			write(socket,msg, sizeof(catalogResponse) + (i-j));
			j = i+1;
			free(msg);
		}
		i++;
	}

	msg = malloc(sizeof(catalogResponse) + (i-j) + 1 );		//send the last catalog name to client
	msg->type = CatalogResponse;
	msg->length = htons(i-j);
	memcpy(msg->filename, catalogList + j, i-j);
	write(socket,msg, sizeof(catalogResponse) + (i-j));
	free(msg);

	msg = malloc(sizeof(catalogResponse));					//send an empty catalog response to indicate end of list
	msg->type = CatalogResponse;
	msg->length = htons(0);
	write(socket,msg,sizeof(catalogResponse));
	free(msg);

}

/*
 * void rfcSendErrorBroadcast(char message[])
 *
 * This function sends an error broadcast as soon as a fatal error
 * has occured. This happens for example when the gamemaster leaves
 * the game or a player leaves the game phase resulting so that
 * only one player is left,
 *
 * Parameter:
 * char message[] = message to be send
 *
 */
void rfcSendErrorBroadcast(char message[])
{
	errorMsg *msg;

	int length = strlen(message);
	msg = malloc( sizeof(errorMsg) + length);

	msg->type = ErrorWarning;
	msg->length = htons(length + 1);
	msg->subtype = fatal;
	memcpy(msg->message, message,length);

	userLockData();
	for(int i = 0; i < 4; i++)			//send the error message to all active players
	{
		if(userGetPlayer(i).playerInUse == 1)
		{
			write(userGetPlayer(i).playerSocket, msg, sizeof(errorMsg) + length + 1);
			close(userGetPlayer(i).playerSocket);
		}
	}
	userUnlockData();
	free(msg);
	mainCleanup();
}
/*
 * catalogChange *rfcReadCatalaogChange(int socket, header head)
 *
 * This function reads an incoming catalog change message from a
 * client. The received message (from gamemaster) is afterwards
 * broadcastetd to all active clients.
 *
 * Parameter:
 * int socket = socket descriptor from client
 * header head = the header already read from socket
 *
 *Return value:
 *pointer to the received message
 */
catalogChange *rfcReadCatalaogChange(int socket, header head)
{
	catalogChange *cch;
	int length = ntohs(head.length);
	char *name;
	cch = malloc(sizeof(catalogChange) + length);
	name = malloc(sizeof(char) * length);

	read(socket,name,length);			//read the remaining part from the socket (catalog name)

	cch->type = CatalogChange;
	cch->length = htons(length);
	memcpy(cch->filename, name, length);
	free(name);

	return cch;
}

/*
 * startGame *rfcReadStartGame(int socket, header head)
 *
 * This function reads an incoming startgame  message from
 * gamemaster. The received filename is used for the loader
 * process which will load the corrosponding file into
 * a shared memory.
 *
 * Parameter:
 * int socket = socket descriptor from client
 * header head = the header already read from socket
 *
 *Return value:
 *pointer to the received message
 */
startGame *rfcReadStartGame(int socket, header head)
{
	startGame *stg;
	int length = ntohs(head.length);
	char *name;

	stg = malloc(sizeof(startGame) + length);
	name = malloc(sizeof(char) * length);

	read(socket,name,length);			//read the remaining part from socket
	stg->type = StartGame;
	stg->length = htons(length);
	memcpy(stg->filename, name,length);
	free(name);
	return stg;
}

/*
 * questionAnswered rfcReadQuestionAnswered(int socket, header head)
 *
 * This function reads an incoming question answered  message from
 * a client.
 *
 * Parameter:
 * int socket = socket descriptor from client
 * header head = the header already read from socket
 *
 *Return value:
 *the answered question
 */
questionAnswered rfcReadQuestionAnswered(int socket, header head)
{
	questionAnswered msg;
	int length = ntohs(head.length);

	msg.type = head.type;
	msg.length = length;

	read(socket,&msg.answer, sizeof(msg.answer));		// read remaining part from socket

	return msg;
}

/*
 * void rfcSendCatalogChangeBroadcast(catalogChange *cch)
 *
 * This function sends a catalog change broadcast to all active clients
 * (except gamemaster) to indicate that the gamemaster has clicked to
 * a catalog name.
 *
 * Parameter:
 *catalogChange *cch = pointer to the catalog change message
 */
void rfcSendCatalogChangeBroadcast(catalogChange *cch)
{
	userLockData();
	for(int i = 1; i < 4; i++)
	{
		if(userGetPlayer(i).playerInUse == 1)
		{
			write(userGetPlayer(i).playerSocket, cch, sizeof(catalogChange) + ntohs(cch->length));
		}
	}
	userUnlockData();
}

/*
 * void rfcSendNoQuestionsleft(int socket)
 *
 * This function sends a message to a client after all questions
 * are answered. This is indicated by a length of 0,
 *
 * Parameter:
 *int socket = socket descriptor of client
 */
void rfcSendNoQuestionsleft(int socket)
{
	questionMsg msg;

	msg.type = QuestionResponse;
	msg.length = 0;

	write(socket,&msg, sizeof(msg) - sizeof(msg.message));
}

/*
 * void rfcSendStartGameBroadcast(void)
 *
 * This function sends a start game broadcast to all clients after
 * the gamemaster has started the game.
 *
 */
void rfcSendStartGameBroadcast(void)
{
	startGame stg;
	stg.type = StartGame;
	stg.length = 0;
	userLockData();
	for(int i = 0; i < 4; i++)
	{
		if(userGetPlayer(i).playerInUse == 1)
		{
			write(userGetPlayer(i).playerSocket, &stg, sizeof(startGame));
		}
	}
	userUnlockData();
}

/*
 * void rfcSendQuestion(Question ques, int socket)
 *
 * This function sends a question to a client.
 *
 *Parameter:
 *Question ques = question to be send
 *int socket = socket descriptor of client
 *
 */
void rfcSendQuestion(Question ques, int socket)
{
	questionMsg msg;

	msg.type = QuestionResponse;
	msg.length = htons(769);

	memcpy(&msg.message, &ques, sizeof(msg.message));

	write(socket, &msg, sizeof(msg));
}

/*
 * void rfcSendQuestionResult(int socket, uint8_t correct, int timeout)
 *
 * This function sends a the result of a answered or unanswered question
 * (timeout) to a client.
 *
 *Parameter:
 *int socket = socket descriptor of client
 *int uint8_t correct = bitmask of correct answer
 *int timeout = timeout expired
 *
 */
void rfcSendQuestionResult(int socket, uint8_t correct, int timeout)
{
	questionResult msg;
	uint8_t mask = 128;

	if(timeout == 1)
	{
		msg.type = QuestionResult;
		msg.length = htons(1);
		msg.correct = correct;

		msg.correct = msg.correct | mask;		//set time expired bit
	}
	else
	{
		msg.type = QuestionResult;
		msg.length = htons(1);
		msg.correct = correct;
	}

	write(socket, &msg, sizeof(questionResult));
}

void rfcSendGameOverMessage(int socket, int userId)
{
	gameOver msg;

	msg.type = GameOver;
	msg.length = htons(5);

	userLockData();
	msg.rank = userGetPlayer(userId).rank;
	msg.score = htonl(userGetPlayer(userId).playerScore);
	userUnlockData();

	write(socket,&msg,sizeof(gameOver));
}

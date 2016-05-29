/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * login.c: Implementierung des Logins
 */

#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include "rfc.h"

#include "rfc.h"
#include "user.h"
#include "login.h"
#include "clientthread.h"
#include "main.h"


void *login(void *arg)
{
	int loginSocket = mainGetLoginSocket();
	int newSocket;
	struct sockaddr_in clientInfo;

	sem_t *sem = mainGetSemaphor();
	pthread_t gameThread[4];
	socklen_t length = sizeof(clientInfo);
	loginRqst msg;


	while(1)
	{
		newSocket = accept(loginSocket,(struct sockaddr *)&clientInfo,&length);		//wait for a new connection
		if(newSocket < 0)
		{
			perror("Error accepting login request : ");
			return NULL;
		}
		//Connection sucessfully established => read loginrequest
		if ( (read(newSocket, &msg, sizeof(msg)) <= 0) )
		{
			perror("Error reading login socket");
			close(newSocket);
		}

		msg.length = ntohs(msg.length);
		if(msg.length > 32 || msg.version != 8)
		{
			rfcSendErrorMessage(newSocket, "Fehler bei name oder Version", fatal);
		}
		else
		{
			msg.name[msg.length - 1] = '\0';
			userLockData();						//get mutex
			int slot = userGetFreeSlot();
			if(slot != -1)						//number of players < 4
			{
				if(userGetGameState() == 0)		//game not started
				{
					if(userIsNameValid(msg.name) == 1)
					{
						userAddPlayer(slot,msg.name,newSocket);
						//==>Start the Gamethread
						if( (pthread_create(&gameThread[slot],NULL,&clientThread,(void *)&slot) != 0) )
						{
							perror("Failed to create game thread :");
							return NULL;

						}
						userUnlockData();		//release mutex
						sem_post(sem);			//activate the score agent => publish new player
						rfcSendLoginResOk(newSocket,slot);
					}
					else	//name already taken
					{
						rfcSendErrorMessage(newSocket, "Name vergeben", fatal);
						close(newSocket);
					}
				}
				else		//game already started
				{
					rfcSendErrorMessage(newSocket, "Spiel bereits gestartet", fatal);
					close(newSocket);
				}
			}

			else		//number of players = 4
			{
				rfcSendErrorMessage(newSocket, "Maximale Spielerzahl erreicht", fatal);
				close(newSocket);
			}
		}

		userUnlockData();
	}

	return NULL;
}




/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * score.h: Implementierung des Score-Agents
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "score.h"
#include "rfc.h"
#include "user.h"
#include "main.h"

/*
 * Score agent thread gets active as soon as one of the following events take place:
 * 	- successful login of a new user
 * 	- lougout of a user
 * 	- the score of a user has changed
 *
 * The agent uses the qsort function to sort the list of users before they are broadcasted
 * to all active users.
 */
void *scoreAgent(void *arg)
{
	sem_t *sem = mainGetSemaphor();
	playerListElement listElements[4];
	int activeSockets[4];
	int numberOfPlayers = 0;

	while(1)
	{
		sem_wait(sem);
		userLockData();

		for(int i=0;i<4;i++)	//copy playerlist
		{
			if( (userGetPlayer(i).playerInUse == 1) )		//create list of active players
			{
				listElements[numberOfPlayers].id = userGetPlayer(i).playerId;															//copy player id
				strncpy(listElements[numberOfPlayers].name, userGetPlayer(i).playerName, sizeof(listElements[numberOfPlayers].name));	//copy playername
				listElements[numberOfPlayers].score = userGetPlayer(i).playerScore;														// copy player score
				activeSockets[numberOfPlayers] = userGetPlayer(i).playerSocket;		//add active sockets to list
				numberOfPlayers++;
			}
		}
		userUnlockData();

		qsort(listElements, numberOfPlayers, sizeof(playerListElement), compare);		//sort the list elements before they are send to the users
		rfcSendListMessage(activeSockets,listElements,numberOfPlayers);

		userLockData();
		for(int i=0; i < numberOfPlayers; i++)
		{
			userSetRankById(listElements[i].id, i + 1);
		}
		userUnlockData();
		numberOfPlayers = 0;

	}

	return NULL;
}

/*
 * int compare(const void *e1, const void *e2)
 *
 * This function is used by the qsort function to sort the
 * array of listElemnts before they are send to the users.
 *
 * Parameter:
 * const void *e1 = pointer to fist element
 * const void *e2 = pointer to second element
 */
int compare(const void *e1, const void *e2)
{
	 playerListElement *p1 = (playerListElement *)e1;
	 playerListElement *p2 = (playerListElement *)e2;

	 if(p1->score == p2->score)
	 {
		 return 0;
	 }
	 if(p1->score > p2->score)
	 {
		 return -1;
	 }
	 if(p1->score < p2->score)
	 {
		 return 1;
	 }

	 return -1;		//never reached
}

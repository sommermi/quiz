/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * user.c: Implementierung der User-Verwaltung
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "rfc.h"
#include "main.h"
#include "user.h"


static player playerData[4];
static int gameState = 0;		//1 indicates game started => refuse further login requests etc. 2 indicates game finished
static pthread_mutex_t mutex;

void userSetUserFinished(int userId)
{
	userLockData();
	playerData[userId].playerFinished = 1;
	userUnlockData();
}

/*
 * This function returns the number of loged in players.
 *
 * Return value:
 * number of active players
 */
int userGetActivePlayers(void)
{
	int numberOfPlayers = 0;
	userLockData();
	for(int i = 0; i<4; i++)
	{
		if(playerData[i].playerInUse == 1)
		{
			numberOfPlayers++;
		}
	}
	userUnlockData();

	return numberOfPlayers;
}

int userGetFinishedPlayers(void)
{
	int numberOfPlayers = 0;
	userLockData();
	for(int i = 0; i<4; i++)
	{
		if(playerData[i].playerFinished)
		{
			numberOfPlayers++;
		}
	}
	userUnlockData();

	return numberOfPlayers;
}

/*
 * This function returns a single player identified by his index
 * in the playerData[] array.
 *
 * Return value:
 * data set of player
 */
player userGetPlayer(int index)
{
	return playerData[index];
}

/*
 * This function checks whether a player name is already taken by
 * another player or not.
 *
 *Parameter:
 *char name[] =  the name to be checked
 *
 * Return value:
 * 1 = name not taken
 * -1 = name taken
 */
int userIsNameValid(char name[])
{
	for(int i=0; i<4;i++)
	{
		if( (strcmp(name,playerData[i].playerName) == 0) && (playerData[i].playerInUse == 1) )		//duplicate found
		{
			return -1;
		}
	}
	return 1;
}

/*
 * This function sets the gamestate to 1 as soon as the game has
 * been started.
 */
void userSetGameState(int state)
{
	gameState = state;
}

/*
 * Return value:
 * gamestat = 1 => game started
 * gamestate = 0 => game not started
 */
int userGetGameState(void)
{
	return gameState;
}

/*
 * This function removes a player from the game. And checks
 * whether the removed player was the gamemaster or not.
 * As soon as the game has started it is aslso cheked whether
 * enough players are left or not.
 *
 * Parameter:
 * int userId = id of the player who gets removed
 */
void userRemovePlayer(int userId)
{
	if(userId != 0)		//regular player has left the game
	{
		userLockData();
		close(playerData[userId].playerSocket);
		memset(&playerData[userId], 0 , sizeof(playerData[userId]));
		userUnlockData();
	}
	else	//gamemaster has left the game
	{
		rfcSendErrorBroadcast("Spielleiter hat das Spiel verlassen");
		mainCleanup();
	}

	if(userGetActivePlayers() < 2 && gameState == 1)		//not enough players during game phase
	{
		rfcSendErrorBroadcast("Nicht genug Spieler! Spiel wird beendet");
		mainCleanup();
	}
}

/*
 * This function sets the score of a player after a question was answered
 *
 * Parameter:
 * int userId = self explaining
 * unsigned long score = new score of player
 */
void userSetScore(int userId, unsigned long score)
{
	userLockData();
	playerData[userId].playerScore = playerData[userId].playerScore + score;
	userUnlockData();
}

void userAddPlayer(int slot, char name[], int socket)
{
	strncpy(playerData[slot].playerName,name,sizeof(playerData[slot].playerName));
	if(slot == 0)
	{
		playerData[slot].playerIsGameMaster = 1;
	}
	playerData[slot].playerSocket = socket;
	playerData[slot].playerInUse = 1;
}
/*
 * int userGetFreeSlot(void)
 *
 * This function returns the number of a free slot in the
 * playerData array. If no slot is availiable (4 players
 * are logged in) the function will -1.
 *
 * Return values:
 * 0 - 3 slot avaliable
 * -1 all slots are reserved
 */

int userGetFreeSlot(void)
{
	for(int i=0;i<4;i++)
	{
		if( playerData[i].playerInUse == 0 )
		{
			return i;
		}
	}
	return -1;
}
int userNameAvailiable(char name[])
{
	return 0;
}
/*
 * void userLockData(void)
 *
 * This function locks the mutex in order to ensure, that
 * only one thread at a time is able to perform operations
 * on the user data.
 *
 */
void userLockData(void)
{
	pthread_mutex_lock(&mutex);
}

/*
 * void userUnlockData(void)
 *
 * This function unlocks the mutex after an operation on the
 * user data has finished.
 *
 */
void userUnlockData(void)
{
	pthread_mutex_unlock(&mutex);
}

/*
 * void userInit(void)
 *
 * This function inits the user data. All values are set to
 * 0. The mutex, which was created by the main function is
 * also stored in a modul global variable for further use.
 *
 */
void userInit(void)
{
	for(int i=0; i<4;i++)
	{
		playerData[i].playerId = i;
		playerData[i].playerInUse = 0;
		if(i == 0)
		{
			playerData[i].playerIsGameMaster = 1;
		}
		else
		{
			playerData[i].playerIsGameMaster = 0;
		}
		playerData[i].playerScore = 0;
		playerData[i].playerSocket = 0;
		playerData[i].playerFinished = 0;
		playerData[i].rank = 0;
	}
	mutex =  mainGetMutex();
}

/*
 * void userSetRankById(int userId, uint8_t rank)
 *
 * This function sets the players rank during the game. The user
 * gets identified by its id.
 *
 * Parameters:
 * int userId = the user id where we want to set the rank
 * uint8_t = the current rank of the user in the game
 *
 */
void userSetRankById(int userId, uint8_t rank)
{
	for(int i= 0; i<4; i++)
	{
		if(playerData[i].playerId == userId)
		{
			playerData[i].rank = rank;
		}
	}
}


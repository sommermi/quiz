/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * user.h: Header für die User-Verwaltung
 */

#ifndef USER_H
#define USER_H

typedef struct
{
	char playerName[32];
	int playerId;
	int playerSocket;
	unsigned long playerScore;
	int playerInUse;
	int playerIsGameMaster;
	int playerFinished;
	uint8_t rank;
}player;

void userInit(void);
void userLockData(void);
void userUnlockData(void);
int userGetFreeSlot(void);
void userAddPlayer(int slot, char name[], int socket);
int userGetGameState(void);
void userSetGameState(int state);
int userIsNameValid(char name[]);
int userGetActivePlayers(void);
int userGetFinishedPlayers(void);
void userRemovePlayer(int userId);
player userGetPlayer(int index);
void userSetUserFinished(int userId);
void userSetScore(int userId, unsigned long score);
void userSetRankById(int userId, uint8_t rank);

#endif

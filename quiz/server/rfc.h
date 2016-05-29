/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 *
 * rfc.h: Definitionen für das Netzwerkprotokoll gemäß dem RFC
 */

#ifndef RFC_H
#define RFC_H

#include "common/question.h"


enum errors
{
	warning,
	fatal
};

enum messages
{
	LoginRequest = 1,
	LoginResponseOk,
	CatalogRequest,
	CatalogResponse,
	CatalogChange,
	PlayerList,
	StartGame,
	QuestionRequest,
	QuestionResponse,			//colliding types in common.h
	QuestionAnswered,
	QuestionResult,
	GameOver,
	ErrorWarning = 255
};


#pragma pack(push,1)		//avoid padding

typedef struct
{
	uint8_t type;
	uint16_t length;
}header;

typedef struct
{
	uint8_t type;
	uint16_t length;
	uint8_t version;
	char name[32];

}loginRqst;

typedef struct
{
	uint8_t  type;
	uint16_t length;
	uint8_t version;
	uint8_t clientId;

}loginResOk;

typedef struct
{
	uint8_t type;
	uint16_t length;
	uint8_t subtype;
	char message[];
}errorMsg;

typedef struct
{
	char name[32];
	uint32_t score;
	uint8_t id;
}playerListElement;


typedef struct
{
	uint8_t type;
	uint16_t length;
	playerListElement list[];
}listMsg;

typedef struct
{
	uint8_t type;
	uint16_t length;
	char filename[];
}catalogResponse;

typedef struct
{
	uint8_t type;
	uint16_t length;
	char filename[];
}catalogChange;


typedef struct
{
	uint8_t type;
	uint16_t length;
	char filename[];
}startGame;

typedef struct
{
	uint8_t type;
	uint16_t length;
	QuestionMessage message;
}questionMsg;

typedef struct
{
	uint8_t type;
	uint16_t length;
	uint8_t answer;
}questionAnswered;

typedef struct
{
	uint8_t type;
	uint16_t length;
	uint8_t correct;
}questionResult;

typedef struct
{
	uint8_t type;
	uint16_t length;
	uint8_t rank;
	uint32_t score;
}gameOver;

#pragma pack(pop)

void rfcSendLoginResOk(int socket, int slot);
void rfcSendErrorMessage(int socket, char message[],int subType);
void rfcSendListMessage(int socketList[], playerListElement[], int numberOfPlayers);
void rfcSendCatalogResponse(int socket);
void rfcSendCatalogChangeBroadcast(catalogChange *cch);
void rfcSendStartGameBroadcast(void);
void rfcSendNoQuestionsleft(int socket);
void rfcSendQuestion(Question ques, int socket);
void *rfcGetMessage(header head, int socket);
void rfcSendErrorBroadcast(char message[]);
void rfcSendQuestionResult(int socket, uint8_t correct, int timeout);
void rfcSendGameOverMessage(int socket, int userId);
catalogChange *rfcReadCatalaogChange(int socket, header head);
startGame *rfcReadStartGame(int socket, header head);
questionAnswered rfcReadQuestionAnswered(int socket, header head);

#endif

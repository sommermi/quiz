/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * catalog.h: Header für die Katalogbehandlung und Loader-Steuerung
 */

#ifndef CATALOG_H
#define CATALOG_H

#include "common/question.h"

typedef struct
{
	int pid;
	char *filelist;
	int numberOfQuestions;
	int stdinPipe[2];
	int stdoutPipe[2];
	int initialised;
	int fdSharedMemory;
	Question *shmData;
}catalogParameters;

int catalogInit(void);
char *catalogGetList(void);
int catalogLoad(char filename[]);
int catalogBindSharedMemory(void);
void catalogUnlinkSharedMemory(void);
Question catalogReadQuestion(int questionNumber);
catalogParameters catalogGetParamesters(void);
#endif

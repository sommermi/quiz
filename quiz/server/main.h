/*
 * main.h
 *
 *  Created on: 18.04.2016
 *      Author: sommer
 */

#ifndef SERVER_MAIN_H_
#define SERVER_MAIN_H_

#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

#define STDLOADERPATH "./loader";
#define SEMKEY 123456L

pthread_mutex_t mainGetMutex(void);
int mainCreateMutex(void);
sem_t *mainGetSemaphor(void);
int mainCreateSemaphor(void);
int mainGetLoginSocket(void);
int mainCreateLockfile(void);
int mainCreateLoginSocket(int port);
void mainDeleteLockfile(void);
int mainInitSighandler(struct sigaction action);
void mainSignalHandler(int sig);
void mainCleanup(void);
void mainShowHelp(void);


#endif /* SERVER_MAIN_H_ */

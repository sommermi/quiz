/*
 * main.h
 *
 *  Created on: 18.04.2016
 *      Author: sommer
 */

#ifndef SERVER_MAIN_H_
#define SERVER_MAIN_H_


static char lockFilePath[] = "/tmp/programRunning";
static struct sigaction action;

int mainCreateLockfile(void);
void mainDeleteLockfile(void);
int mainInitSighandler(void);
void mainSignalHandler(int sig);
void mainCleanup(void);


#endif /* SERVER_MAIN_H_ */

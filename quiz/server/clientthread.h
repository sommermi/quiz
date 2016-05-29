/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * clientthread.h: Header für den Client-Thread
 */

#ifndef CLIENTTHREAD_H
#define CLIENTTHREAD_H

void *clientThread(void *arg);
int clientGamemaster(void);
int clientRegularPlayer(int id);
int clientGamePhase(int id, int socket);
int clientGameEndPhase(int socket, int id);
unsigned long scoreForTimeLeft(unsigned long timeLeft,unsigned long timeout);
#endif

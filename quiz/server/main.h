/*
 * main.h
 *
 *  Created on: 18.04.2016
 *      Author: sommer
 */

#ifndef SERVER_MAIN_H_
#define SERVER_MAIN_H_


static char lockFilePath[] = "/tmp/programRunning";	//Test

int mainCreateLockfile(void);
int mainDeleteLockfile(void);


#endif /* SERVER_MAIN_H_ */

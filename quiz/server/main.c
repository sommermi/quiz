/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * main.c: Hauptprogramm des Servers
 */

#include "common/util.h"
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "main.h"

int main(int argc, char **argv)
{
	switch(mainCreateLockfile())
	{
		case 0:	 perror("Error: Another instance of the server is already running!");
				 return -1;
		case 1:  printf("\nStarting server...");
				 break;
		case -1: perror("Error: Problems while creating lock file");
				 return -1;
	}


	if(mainDeleteLockfile() != 0)
	{
		perror("Error: Deleting lockfile failed!");
	}

	return 0;
}

/**
 * int mainCreateLockfile(void)
 *
 * This function creates a lock file to ensure that only one instance
 * of the server is running on the machine.
 * The default path to the lockfile is /tmp/programRunning. This path
 * is specified in main.h.
 *
 * Return values:
 * 0 indicates that there is already one instance running
 * 1 indicates that no instnace is running => file was sucessfully created
 * -1 indicates that there was an eeror creating the lock file (lack of memory etc.)
 */
int mainCreateLockfile(void)
{
	int fd = open(lockFilePath, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	if(fd < 0)
	{
		if(errno == EEXIST)		//file already exists => Instance of server already running
		{
			return 0;
		}
		else	//problems with open()
		{
			return -1;
		}
	}
	else	//file was created sucessfully => no instance was running
	{
		close(fd);
		return 1;
	}
}

/**
 * int mainDeleteLockfie(void)
 *
 * This function deletes the crated lock file that ensures that only one instance
 * of the server is running.
 * The function is called at the end of a game as part of the cleanup routine to
 * free all resources that were occupied by the server process.
 *
 * Return values:
 * !=0 indicates that there an error has occured while deleting the lock file
 */
int mainDeleteLockfile(void)
{
	return unlink(lockFilePath);
}

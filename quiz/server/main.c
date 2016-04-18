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
#include <signal.h>
#include "main.h"

#include <stdio.h>


int main(int argc, char **argv)
{

	if(mainInitSighandler() < 0)
	{
		perror("Error: Unable to regsiter custom sigHandler");
		return -1;
	}

	switch(mainCreateLockfile())
	{
		case 0:	 perror("Error: Another instance of the server is already running!");
				 return -1;
		case 1:  printf("Starting server...\n");
				 break;
		case -1: perror("Error: Problems while creating lock file");
				 return -1;
	}

	sleep(15);

	mainCleanup();


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
 * This function deletes the created lock file that ensures that only one instance
 * of the server is running.
 * The function is called at the end of a game as part of the cleanup routine to
 * free all resources that were occupied by the server process.
 *
 * Return values:
 * != 0 indicates that there an error has occured while deleting the lock file
 */
void mainDeleteLockfile(void)
{
	if(unlink(lockFilePath) != 0)
	{
		perror("Error: Deleting lockfile");
	}

}

/*
 * void mainSignalHandler(int sig)
 *
 * This function implements the custom signal handler for the server process. In case
 * a SIGTERM or SIGINT signal is received this function will call the mainCleanup
 * function which will release all the resources.
 *
 * Parameters:
 * int sig = signal number (specified in signal.h)
 */
void mainSignalHandler(int sig)
{
	switch(sig)
	{
		case SIGINT  : printf("Received SIGINT starting cleanup\n");
					   mainCleanup();
					   break;
		case SIGTERM : printf("Recieved SIGTERM starting cleanup\n");
					   mainCleanup();
					   break;
	}
}

/*
 * int mainInitSignalhandler(void)
 *
 * This function redirects the needed signals to the custom signal handler routine
 * mainSignalHandler().
 *
 * Return values:
 * -1 indicates an error has occured => terminate process
 * 0 indicates no error
 */
int mainInitSighandler(void)
{
	action.sa_handler = mainSignalHandler;		//register custom sigHandler function
	sigemptyset(&action.sa_mask);				//don't block other signals while sigHandler runs
	action.sa_flags = 0;						//no special behavior required

	//redirect the required signals to custom sigHandler
	if( (sigaction(SIGINT, &action, NULL) || sigaction(SIGTERM,&action,NULL) ) < 0)
	{
		return -1;
	}

	return 0;
}

/*
 * void mainCleanup(void)
 *
 * This function releases all the acquired resources before the termination of the process.
 */
void mainCleanup(void)
{
	mainDeleteLockfile();
}

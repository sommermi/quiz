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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "login.h"
#include "main.h"
#include "user.h"
#include "score.h"



static char lockFilePath[] = "/tmp/programRunning";
static int loginSocket;
static sem_t sem;
static pthread_mutex_t mutex;
static pthread_t loginThread;
static pthread_t scoreAgentThread;


int main(int argc, char **argv)
{
	struct sigaction action;

	int port = 0;
	int i;
	char pathLoader[256];
	char pathCatalog[256];

	while( (i = getopt(argc, argv,"p:c:l:h")) != -1)
	{
		switch(i)
	    {
	    	case 'p': port = atoi(optarg);
	                  break;
	        case 'c': strncpy(pathCatalog,optarg,256);
	                  break;
	        case 'l': strncpy(pathLoader,optarg,256);
	                  break;
	        case 'h': mainShowHelp();
	                  return 0;
	        default : mainShowHelp();
	        		  return 0;
	        }
	    }

	if(mainInitSighandler(action) < 0)
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

	if( (mainCreateLoginSocket(port) < 0) )
	{
		mainCleanup();
		return -1;
	}

	if( (mainCreateSemaphor()) < 0)
	{
		mainCleanup();
		return -1;
	}

	if( (mainCreateMutex()) < 0)
	{
		mainCleanup();
		return -1;
	}

	userInit();

	if( (pthread_create(&loginThread,NULL,&login,NULL) != 0) )
	{
		perror("Failed to create login thread :");
		mainCleanup();
		return -1;
	}

	if( (pthread_create(&scoreAgentThread,NULL,&scoreAgent,NULL) != 0) )
	{
		perror("Failed to create score agent thread : ");
		mainCleanup();
		return -1;
	}

	pthread_join(loginThread,NULL);
	pthread_exit(&scoreAgentThread);
	mainCleanup();
	return 0;
}

pthread_mutex_t mainGetMutex(void)
{
	return mutex;
}

int mainCreateMutex(void)
{
	if( (pthread_mutex_init(&mutex,NULL)) < 0)
	{
		perror("Failed to create mutex: ");
		return -1;
	}

	return 0;
}

/*
 * sem_t mainGetSemaphor(void)
 *
 * This function returns the semaphore used by the client threads
 * and the score agent thread.
 *
 * Return value:
 * semaphore
 */
sem_t *mainGetSemaphor(void)
{
	return &sem;
}

/*
 * int mainCreateSemaphor(void)
 *
 * This function creates the semaphor which is used to trigger the
 * score agent thread.
 *
 * Return values:
 * -1 indicates that an error has occurred => unable to init semaphore
 * 0 indicates no problems
 */
int mainCreateSemaphor(void)
{
	if( (sem_init(&sem,0,0)) < 0 )
	{
		perror("Failed to create semaphor: ");
		return -1;
	}

	return 0;
}


/*
 * int mainGetLoginSocket(void)
 *
 * This function returns the loginSocket which is used by the login
 * thread.
 *
 * Return values:
 * Socket descriptor loginSocket
 */
int mainGetLoginSocket(void)
{
	return loginSocket;
}
/*
 * int mainCreateLoginSocket(int port)
 *
 * This function creates the socket which the login thread is later using
 * to accept incoming client connestions.
 *
 * Parameter:
 * int port = the port on which the server is waiting for
 * connestions. If port = 0 the standard port 54321 is used.
 *
 * Return values:
 * -1 indicates that an error has occured
 * 0 indicates everything worked
 */
int mainCreateLoginSocket(int port)
{
	struct sockaddr_in sa;
	if( (loginSocket = socket(AF_INET,SOCK_STREAM,0)) < 0)		//create a ipv4 tcp socket
	{
		perror("Failed to create login socket: ");
		return -1;
	}

	//initialise the socket address structure
	memset(&sa,0,sizeof(sa));
	sa.sin_family = AF_INET;
	if(port == 0)				//no port was specified => use STDPORT
	{
		sa.sin_port = htons(54321);
		printf("Using std port\n");
	}
	else
	{
		sa.sin_port = htons(port);
		printf("Using port: %d\n", port);
	}
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	if( (bind (loginSocket, (struct sockaddr *)&sa,sizeof(sa)) < 0))
	{
		perror("Failed to bind the login socket: ");
		return -1;
	}

	if( (listen(loginSocket,4) < 0))
	{
		perror("Failed to listen login socket: ");
		return -1;
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
int mainInitSighandler(struct sigaction action)
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
	printf("\nCleanup\n");
	mainDeleteLockfile();
	close(loginSocket);
	sem_destroy(&sem);
	pthread_cancel(loginThread);
	pthread_cancel(scoreAgentThread);
	pthread_mutex_destroy(&mutex);
}

/*
 * void mainShowHelp(void)
 *
 * This funtion shows all the possible command options to the user.
 */
void mainShowHelp(void)
{
	printf("Aufruf: -c KATALOGE [-p PORT] [-l LOADER] [-h HELP]\n");
	printf("-c KATALOGE Pfade zu den Fragekatalogen\n");
	printf("-p PORT zu verwendender Port Default: 54321\n");
	printf("-l LOADER Pfad zum Loader Default ./loader\n");
	printf("-h HELP Hilfe anzeigen\n");
}

/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * catalog.c: Implementierung der Fragekatalog-Behandlung und Loader-Steuerung
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "common/question.h"
#include "common/server_loader_protocol.h"
#include "common/util.h"
#include "catalog.h"

static catalogParameters param;

/*
 * catalogParameters catalogGetParameters(void)
 *
 * This function returns the catalogParameters structure. This function
 * is for example used inside the cleanup method to unlink the shared
 * memory if it was initialised.
 *
 * Return value:
 * the catalogParameter structure
 */
catalogParameters catalogGetParameters(void)
{
	return param;
}


/*
 * int catalogLoad(char filename[])
 *
 * This function sends the chosen catalog (identified by name) to the loader
 * process via the established pipe connection. The chosen catalog is then
 * loaded into the shared memory area.
 *
 * Return value:
 * 1 indicates catalog suceessful loaded
 * -1 indicates an error hasoccured
 *
 * Parameters:
 * char filename[] = the name of the desired catalog
 */
int catalogLoad(char filename[])
{
	char loadCmd[256];
	char *readBuffer;
	char numberOfQuestions[4];

	int len = sprintf(loadCmd, "LOAD %s\n", filename);
	write(param.stdinPipe[1], loadCmd, len);

	while((readBuffer = readLine(param.stdoutPipe[0])) != NULL)
	{
		if(!strncmp(readBuffer, LOAD_SUCCESS_PREFIX, sizeof(LOAD_SUCCESS_PREFIX)-1))
		{
			int j = 0;
			for(int i = 0; i < strlen(readBuffer); i++)
			{
				if(isdigit(readBuffer[i]))			//get the number of questions inside the loaded catalog
				{
					numberOfQuestions[j] = readBuffer[i];
					j++;
				}
			}
			numberOfQuestions[j] = '\0';
			param.numberOfQuestions = atoi(numberOfQuestions);
			param.initialised = 1;		//indicates that shared memory exists (used for cleanup)
			free(readBuffer);
			return 1;
		}
		else if(!strncmp(readBuffer, LOAD_ERROR_INVALID, sizeof(LOAD_ERROR_INVALID)-1))
		{
			free(readBuffer);
			return -1;
		}
		else if(!strncmp(readBuffer, LOAD_ERROR_PREFIX, sizeof(LOAD_ERROR_PREFIX)-1))
		{
			free(readBuffer);
			return -1;
		}
		else if(!strncmp(readBuffer, LOAD_ERROR_CANNOT_OPEN, sizeof(LOAD_ERROR_CANNOT_OPEN)-1))
		{
			free(readBuffer);
			return -1;
		}
		else if(!strncmp(readBuffer, LOAD_ERROR_CANNOT_READ	, sizeof(LOAD_ERROR_CANNOT_READ)-1))
		{
			free(readBuffer);
			return -1;
		}
		else if(!strncmp(readBuffer, LOAD_ERROR_SHMEM	, sizeof(LOAD_ERROR_SHMEM)-1))
		{
			free(readBuffer);
			return -1;
		}
		else if(!strncmp(readBuffer, LOAD_ERROR_OOM	, sizeof(LOAD_ERROR_OOM)-1))
		{
			free(readBuffer);
			return -1;
		}
	}

	return 1;
}

/*
 * char *catalogGetList(void)
 *
 * This function returns the list of all availibale catalogs. This is used
 * to send the list to a client
 *
 * Return value:
 * char* pointer to catalogList
 *
 */
char *catalogGetList(void)
{
	return param.filelist;
}

/*
 * int catalogBindSharedMemory(void)
 *
 * This function binds a created shared memory into the address space of the
 * server process, so that it can be accessed for reading the questions inside
 * the loaded catalog.
 *
 * Return value:
 * 1 indicates no error
 * -1 indicates error while mapping
 *
 */
int catalogBindSharedMemory(void)
{
	param.fdSharedMemory = shm_open(SHMEM_NAME, O_RDONLY, (mode_t)0);
	if(param.fdSharedMemory < 0)
	{
		perror("error opening shared memory");
		return -1;
	}
	//startaddress, length, memoryprotection, MAP_SHARED, filedescriptor, offset
	param.shmData = mmap(NULL,param.numberOfQuestions * sizeof(Question), PROT_READ, MAP_SHARED, param.fdSharedMemory,0);
	if(param.shmData == MAP_FAILED)
	{
		perror("error mapping shared memory");
		return -1;
	}

	return 1;
}

/*
 * Question catalogReadQuestion(int questionNumber)
 *
 * This function reds a question out of the shared memory area. The question is
 * identified by it's number.
 *
 * Return value:
 * Question = the desired question
 *
 * Parameter:
 * int questionNumber = number of the desired question
 *
 */
Question catalogReadQuestion(int questionNumber)
{
	Question ques;

	memcpy(&ques, param.shmData + questionNumber, sizeof(Question));	//copy the question out of the shared memory

	return ques;
}

/*
 * void catalogUnlinkSharedMemory(void)
 *
 * This function simply unlinks (deletes) the shared memory after a game has
 * ended.
 *
 */
void catalogUnlinkSharedMemory(void)
{
	shm_unlink(SHMEM_NAME);
}

/*
 * int catalogInit(void)
 *
 * This function inits the catalog process by first forking from server
 * process and afterwards excecuting the provided loader process with
 * the respective parameters. Afterwards commands can be send and received
 * via the established pipe connections.
 *
 * Return value:
 * 1 indicates no error
 * -1 indicates an error
 *
 */
int catalogInit(void)
{
	if( (pipe(param.stdinPipe) || pipe(param.stdoutPipe) == -1) )	//create pipes
	{
		perror("Error creating pipes for loader process: ");
		return -1;
	}

	if( (param.pid = fork()) < 0)		//saves PID of loader process for further use (cleanup)
	{
		perror("error forking loader process: ");
		return -1;
	}
	if(param.pid == 0)	//inside the child process
	{
		if( (dup2(param.stdinPipe[0], STDIN_FILENO)) == -1)		//use stdinPipe[0] as new stdin for loader process
		{
			perror("error redirecting stdin ");
			return -1;
		}

		if( (dup2(param.stdoutPipe[1], STDOUT_FILENO)) == -1)	//use stdoutPipe[1] as new stfout for loader process
		{
			perror("error redirecting stdout ");
			return -1;
		}
		execl("/bin/loader", "loader","-d" ,"/home/sommer/catalogs" , NULL);
		return 1;
	}
	else
	{
		int bufferGrow = 128;
		int bufferSize = 128;
		int readPos = 0;
		char *newBuffer;
		char browseCmd[] = "BROWSE\n";

		close(param.stdinPipe[0]);
		close(param.stdoutPipe[1]);
		write(param.stdinPipe[1], browseCmd, sizeof(browseCmd) - 1);	//send Browse command to loader
		param.filelist = malloc(bufferSize);

		for(;;)		//read the availiable catalogs from pipe
		{
			if(readPos >= bufferSize)
			{
				printf("\nReallocating\n");
				bufferSize = bufferSize + bufferGrow;
				newBuffer = realloc(param.filelist,bufferSize);
				if(newBuffer == NULL)
				{
					free(param.filelist);
					perror("error reallocating memory for filelist: ");
					return -1;
				}
				param.filelist = newBuffer;
			}
			read(param.stdoutPipe[0],&param.filelist[readPos],1);		//read sign by sign from pipe
			if(param.filelist[readPos] == '\n')							//new line signe reached
			{
				readPos++;
				read(param.stdoutPipe[0],&param.filelist[readPos],1);	//read next sign from pipe
				if(param.filelist[readPos] == '\n')						//if sign also new line sign => end of filelist reached
				{
					param.filelist[readPos - 1] = '\0';					//add termination to filelist
					return 1;
				}

				readPos++;		//end of file list not reached
			}
			else						//continue reading
			{
				readPos++;
			}
		}
	}
	return 1;
}

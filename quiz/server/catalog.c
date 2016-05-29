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

catalogParameters catalogGetParamesters(void)
{
	return param;
}
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
				if(isdigit(readBuffer[i]))
				{
					numberOfQuestions[j] = readBuffer[i];
					j++;
				}
			}
			numberOfQuestions[j] = '\0';
			param.numberOfQuestions = atoi(numberOfQuestions);
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

char *catalogGetList(void)
{
	return param.filelist;
}

int catalogBindSharedMemory(void)
{
	param.fdSharedMemory = shm_open(SHMEM_NAME, O_RDONLY,0400);
	if(param.fdSharedMemory < 0)
	{
		perror("error opening shared memory");
		return -1;
	}
	printf("\nNumberrrr: %d\n", param.numberOfQuestions);
	param.shmData = mmap(NULL,param.numberOfQuestions * sizeof(Question), PROT_READ, MAP_SHARED, param.fdSharedMemory,0);
	if(param.shmData == MAP_FAILED)
	{
		perror("error mapping shared memory");
	}

	return 0;
}

Question catalogReadQuestion(int questionNumber)
{
	Question ques;

	memcpy(&ques, param.shmData + questionNumber, sizeof(Question));

	return ques;
}

void catalogUnlinkSharedMemory(void)
{

}

int catalogInit(void)
{
	if( (pipe(param.stdinPipe) || pipe(param.stdoutPipe) == -1) )
	{
		perror("Error creating pipes for loader process: ");
		return -1;
	}

	if( (param.pid = fork()) < 0)
	{
		perror("error forking loader process: ");
		return -1;
	}
	if(param.pid == 0)	//child process
	{
		if( (dup2(param.stdinPipe[0], STDIN_FILENO)) == -1)
		{
			perror("error redirecting stdin ");
			return -1;
		}

		if( (dup2(param.stdoutPipe[1], STDOUT_FILENO)) == -1)
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
		write(param.stdinPipe[1], browseCmd, sizeof(browseCmd) - 1);
		param.filelist = malloc(bufferSize);

		for(;;)
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
			read(param.stdoutPipe[0],&param.filelist[readPos],1);
			if(param.filelist[readPos] == '\n')
			{
				readPos++;
				read(param.stdoutPipe[0],&param.filelist[readPos],1);
				if(param.filelist[readPos] == '\n')
				{
					param.filelist[readPos - 1] = '\0';
					param.initialised = 1;
					return 1;
				}

				readPos++;
			}
			else
			{
				readPos++;
			}
		}
	}
	return 1;
}

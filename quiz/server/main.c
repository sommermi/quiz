/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * main.c: Hauptprogramm des Servers
 */

#include "common/util.h"
#include <stdio.h>

int main(int argc, char **argv)
{
	setProgName(argv[0]);
	/* debugEnable() */

	infoPrint("Server Gruppe xy");
	printf("\nDas ist ein Test");		//Test commit

	return 0;
}

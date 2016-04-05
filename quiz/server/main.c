/*
 * Systemprogrammierung
 * Multiplayer-Quiz
 *
 * Server
 * 
 * main.c: Hauptprogramm des Servers
 */

#include "common/util.h"

int main(int argc, char **argv)
{
	setProgName(argv[0]);
	/* debugEnable() */

	infoPrint("Server Gruppe xy");

	return 0;
}

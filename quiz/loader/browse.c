/**
 * \file	loader/browse.c
 * \author	Stefan Gast
 *
 * \brief	Implementierung des Verzeichnislisters
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include "common/util.h"
#include "browse.h"
#include "util.h"

/**
 * \brief	Einen Verzeichniseintrag ausgeben
 *
 * Gibt den angegebenen Verzeichniseintrag auf der Standardausgabe mit einem
 * einzigen write-Aufruf aus.
 */
static void printDirent(struct dirent *entry		/**< Der auszugebende Verzeichniseintrag */
		       )
{
	char *end = strchr(entry->d_name, '\0');	/* Ende der Zeichenkette suchen */

	*end = '\n';		/* Nullterminierung durch Zeilenumbruch ersetzen */
	write2stdout(entry->d_name, end-entry->d_name+1, NULL);
	*end = '\0';		/* Ersetzung rückgängig machen (für evtl. weitere Verarbeitung) */
}


/**
 * \brief	Den Inhalt eines Verzeichnisses auf der Standardausgabe ausgeben
 *
 * Gibt den Inhalt eines Verzeichnisses auf der Standardausgabe zeilenweise aus.
 * Es werden keine versteckten Dateien (deren Name mit einem Punkt beginnt) aufgelistet.
 * Eine Leerzeile signalisiert das Ende der Ausgabe.
 * Fehlermeldungen erscheinen auf der Standardfehlerausgabe.
 */
void browse(const char *directory_name		/**< Der Name des aufzulistenden Verzeichnisses */
	   )
{
	DIR *dirp = opendir(directory_name);
	struct dirent *entry;

	if(dirp == NULL)
	{
		errorPrint("Kann Verzeichnis %s nicht auflisten: %s",
			   directory_name, strerror(errno));
		write2stdout("\n", 1, NULL);
		return;
	}

	errno = 0;
	entry = readdir(dirp);
	while(entry != NULL && errno == 0)
	{
		if(entry->d_name[0] != '.')		/* versteckte Dateien ignorieren... */
			printDirent(entry);		/* ...und nur die anderen ausgeben */

		entry = readdir(dirp);
	}

	if(errno != 0)			/* Falls ein Fehler aufgetreten ist... */
	{
		errorPrint("Fehler beim Auflisten des Verzeichnisses %s: %s",		/* ...Meldung ausgeben */
			   directory_name, strerror(errno));
	}

	write2stdout("\n", 1, NULL);
}

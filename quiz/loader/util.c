/**
 * \file	loader/util.c
 * \author	Stefan Gast
 *
 * \brief	Hilfsfunktionen des Loaders
 */

#include <stdlib.h>
#include "util.h"
#include "common/util.h"

/**
 * \brief	Auf die Standardausgabe schreiben und bei Fehler Programm beenden
 *
 * Schreibt den übergebenen Puffer auf die Standardausgabe. Falls nicht alle
 * Bytes geschrieben werden können, wird eine Fehlermeldung auf der
 * Standardfehlerausgabe geschrieben, eine Cleanup-Funktion ausgeführt und das
 * Programm beendet.
 */
void write2stdout(const void *buf,		/**< Der zu schreibende Puffer */
		  size_t n,			/**< Anzahl der zu schreibenden Bytes */
		  void (*cleanup_fn)(void)	/**< Cleanup-Funktion, oder NULL */
		 )
{
	const ssize_t ret = write(STDOUT_FILENO, buf, n);

	if(ret == (ssize_t)-1)
	{
		errnoPrint("Kann nicht auf Standardausgabe schreiben");
		if(cleanup_fn)
			cleanup_fn();
		exit(EXIT_FAILURE);
	}

	if((size_t)ret != n)
	{
		errorPrint("Kann nicht den ganzen Puffer auf Standardausgabe schreiben!");
		if(cleanup_fn)
			cleanup_fn();
		exit(EXIT_FAILURE);
	}
}

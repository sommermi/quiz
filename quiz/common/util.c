/**
 * \file	common/util.c
 * \author	Stefan Gast
 *
 * \brief	Implementierung diverser Hilfsfunktionen für Ein-/Ausgabe, Fehlersuche usw.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include "util.h"


/**
 * \brief	Enum für Ausgabestile
 */
typedef enum
{
	STYLE_NORMAL,		/**< Normaler Ausgabestil ohne besondere Formatierung */
	STYLE_INFO,		/**< Informationsmeldung */
	STYLE_ERROR,		/**< Fehlermeldung */
	STYLE_DEBUG,		/**< Debug-Meldung */
	STYLE_HEXDUMP		/**< Hex-Dump */
} OutputStyle;


static const char *prog_name = "<unknown>";	/**< Aufrufname des Programms (argv[0]) */
static int debug_enabled = 0;			/**< Debug-Ausgaben eingeschaltet? */
static int style_enabled = 1;			/**< Ausgabehervorhebungen aktiviert? */


/**
 * \brief	Einen FILE-Stream für andere Threads blockieren und Thread-Abbruch
 *		verhindern.
 *
 * Bei Ausgaben, die mehrere Ausgabefunktionen (wie fprintf, fputc...) hintereinander
 * ausführen und zusammenhängend erscheinen müssen, muss der FILE-Stream für andere
 * Threads gesperrt werden, damit die Ausgabe nicht durch diese unterbrochen wird.
 * Dies wird über die Funktion flockfile realisiert. Während ein Thread die Sperre
 * für den FILE-Stream hält, darf dieser nicht abgebrochen werden, ansonsten geht
 * die Sperre verloren und nachfolgende Ausgaben resultieren in einem Deadlock.
 *
 * \return	PTHREAD_CANCEL_ENABLE, falls Thread-Abbruch zuvor erlaubt war, sonst
 *		PTHREAD_CANCEL_DISABLE
 *
 * \see		unlockFile
 */
static int lockFile(FILE *file		/**< FILE-Stream, der gesperrt werden soll */
		   )
{
	int oldState;
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldState);
	flockfile(file);
	return oldState;
}


/**
 * \brief	Einen gesperrten FILE-Stream wieder freigeben und Thread-Abbruch
 *		ggf. wieder ermöglichen.
 *
 * Gibt einen zuvor mit lockFile gesperrten FILE-Stream wieder für andere Threads
 * frei und stellt den Cancel-Status des Threads wieder her.
 *
 * \see		lockFile
 */
static void unlockFile(FILE *file,		/**< Zuvor gesperrter FILE-Stream */
		      int savedCancelState	/**< Im vorherigen Aufruf von lockFile zurückgegebener Cancel-Status */
		     )
{
	funlockfile(file);
	pthread_setcancelstate(savedCancelState, NULL);
}


/**
 * \brief	Ausgabestil umschalten
 *
 * Schaltet den Ausgabestil des übergebenen FILE-Streams um, vorausgesetzt dieser
 * ist mit einer Konsole verbunden. Dazu werden ANSI Escape Codes verwendet.
 *
 * \see		OutputStyle
 */
static void setStyle(FILE *file,		/**< FILE-Stream, für den der Stil geändert werden soll */
		     OutputStyle style		/**< Gewünschter Ausgabestil */
		    )
{
	if(style_enabled && isatty(fileno(file)))
	{
		switch(style)
		{
			case STYLE_NORMAL:
				fprintf(file, "\033[0;39;49m");	/* Attribute und Vordergrundfarbe zurücksetzen */
				break;
			case STYLE_INFO:
				fprintf(file, "\033[1;39;49m");	/* Fettdruck, Standardfarben */
				break;
			case STYLE_ERROR:
				fprintf(file, "\033[1;31;49m");	/* Fettdruck, rot */
				break;
			case STYLE_DEBUG:
				fprintf(file, "\033[0;33;49m");	/* normal, gelb */
				break;
			case STYLE_HEXDUMP:
				fprintf(file, "\033[0;32;49m");	/* normal, grün */
				break;
		}
	}
}


/**
 * \brief	Den Programmnamen setzen
 *
 * Setzt den Namen des Programms (normalerweise argv[0] von main).
 * Es wird kein Speicher reserviert, sondern die gleiche Adresse wie für
 * das Argument verwendet!
 *
 * \see		getProgName
 */
void setProgName(const char *argv0	/**< argv[0] von main */
		)
{
	prog_name = argv0;
}


/**
 * \brief	Den Programmnamen abfragen
 *
 * Fragt den Aufrufnamen des Programms ab, der zuvor mit setProgName
 * gesetzt wurde.
 *
 * \return	Der zuvor mit setProgName gesetzte Name, oder "<unknown>"
 *		falls setProgName noch nicht aufgerufen wurde.
 *
 * \see		setProgName
 */
const char *getProgName(void)
{
	return prog_name;
}


/**
 * \brief	Debug-Ausgaben einschalten
 *
 * Schaltet die Debug-Ausgaben ein.
 */
void debugEnable(void)
{
	debug_enabled = 1;
}


/**
 * \brief	Prüfen, ob Debug-Meldungen aktiv sind
 *
 * \retval	0 Debug-Meldungen derzeit ausgeschaltet
 * \retval	1 Debug-Meldungen aktiv
 */
int debugEnabled(void)
{
	return debug_enabled;
}


/**
 * \brief	Debug-Ausgaben ausschalten
 *
 * Schaltet die Debug-Ausgaben aus.
 */
void debugDisable(void)
{
	debug_enabled = 0;
}


/**
 * \brief	Ausgabestile für Meldungen aktivieren
 *
 * Aktiviert unterschiedliche Ausgabestile für die verschiedenen Typen von Meldungen.
 *
 * \note	Hat nur eine Wirkung, wenn der entsprechende Ausgabekanal (stdout, stderr) ein
 *		Terminal ist.
 */
void styleEnable(void)
{
	style_enabled = 1;
}


/**
 * \brief	Prüfen, ob unterschiedliche Ausgabestile aktiv sind
 *
 * \retval	0 Ausgabestile derzeit ausgeschaltet
 * \retval	1 Ausgabestile aktiv
 */
int styleEnabled(void)
{
	return style_enabled;
}


/**
 * \brief	Ausgabestile für Meldungen deaktivieren
 *
 * Deaktiviert unterschiedliche Ausgabestile für die verschiedenen Meldungstypen.
 */
void styleDisable(void)
{
	style_enabled = 0;
}


/**
 * \brief	Debug-Meldung ausgeben
 *
 * Gibt eine Meldung auf der Standardfehlerausgabe aus, falls Debug-Ausgaben aktiviert sind.
 * Der Meldung wird der Name des Programms, gefolgt von ": " vorangestellt. Jede Ausgabe wird
 * durch einen Zeilenumbruch abgeschlossen.
 *
 * \see		debugEnable, debugDisable, setProgName
 */
void debugPrint(const char *fmt,	/**< printf-Formatstring */
		...			/**< zusätzliche Argumente */
	       )
{
	va_list args;
	int savedCancelState;

	if(debug_enabled)
	{
		va_start(args, fmt);
		savedCancelState = lockFile(stderr);	/* Stream sperren, sodass die Zeile immer komplett und zusammenhängend ausgegeben wird */
		setStyle(stderr, STYLE_DEBUG);
		fprintf(stderr, "%s: ", getProgName());
		vfprintf(stderr, fmt, args);
		putc_unlocked('\n', stderr);
		setStyle(stderr, STYLE_NORMAL);
		unlockFile(stderr, savedCancelState);	/* Sperre freigeben */
		va_end(args);
	}
}


/**
 * \brief	Informations-Meldung ausgeben
 *
 * Gibt eine Meldung auf der Standardfehlerausgabe aus.
 * Der Meldung wird der Name des Programms, gefolgt von ": " vorangestellt. Jede Ausgabe wird
 * durch einen Zeilenumbruch abgeschlossen.
 *
 * \see		setProgName
 */
void infoPrint(const char *fmt,		/**< printf-Formatstring */
		...			/**< zusätzliche Argumente */
	       )
{
	va_list args;
	int savedCancelState;

	va_start(args, fmt);
	savedCancelState = lockFile(stderr);	/* Stream sperren, sodass die Zeile immer komplett und zusammenhängend ausgegeben wird */
	setStyle(stderr, STYLE_INFO);
	fprintf(stderr, "%s: ", getProgName());
	vfprintf(stderr, fmt, args);
	putc_unlocked('\n', stderr);
	setStyle(stderr, STYLE_NORMAL);
	unlockFile(stderr, savedCancelState);	/* Sperre freigeben */
	va_end(args);
}


/**
 * \brief	Fehlermeldung ausgeben
 *
 * Gibt eine Meldung auf der Standardfehlerausgabe aus.
 * Der Meldung wird der Name des Programms, gefolgt von ": " vorangestellt. Jede Ausgabe wird
 * durch einen Zeilenumbruch abgeschlossen.
 *
 * \see		setProgName
 */
void errorPrint(const char *fmt,	/**< printf-Formatstring */
		...			/**< zusätzliche Argumente */
	       )
{
	va_list args;
	int savedCancelState;

	va_start(args, fmt);
	savedCancelState = lockFile(stderr);	/* Stream sperren, sodass die Zeile immer komplett und zusammenhängend ausgegeben wird */
	setStyle(stderr, STYLE_ERROR);
	fprintf(stderr, "%s: ", getProgName());
	vfprintf(stderr, fmt, args);
	putc_unlocked('\n', stderr);
	setStyle(stderr, STYLE_NORMAL);
	unlockFile(stderr, savedCancelState);	/* Sperre freigeben */
	va_end(args);
}


/**
 * \brief	Fehlermeldung in Abhängigkeit zu errno ausgeben
 *
 * Gibt eine Fehlermeldung nach dem Muster
 * "Programmname: prefix: strerror(errno)" auf der Standardfehlerausgabe aus.
 * Im Gegensatz zu strerror ist diese Funktion threadsicher.
 *
 * \see		setProgName
 */
void errnoPrint(const char *prefix	/**< Text, der der Fehlerbeschreibung vorangestellt wird */
	       )
{
	int savedCancelState;

	savedCancelState = lockFile(stderr);	/* Stream sperren, sodass die Zeile immer komplett und zusammenhängend ausgegeben wird */
	setStyle(stderr, STYLE_ERROR);
	fprintf(stderr, "%s: ", getProgName());
	perror(prefix);
	setStyle(stderr, STYLE_NORMAL);
	unlockFile(stderr, savedCancelState);	/* Sperre freigeben */
}


/**
 * \brief	Inhalt eines Puffers als Hexdump ausgeben, falls Debug-Ausgaben aktiviert sind
 *
 * Gibt den Inhalt des übergebenen Puffers als Hexdump auf der
 * Standardfehlerausgabe aus, falls Debug-Ausgaben aktiviert sind.
 * Jede Zeile hat dabei folgenden Aufbau:
 * Programmname: Präfix: Hexdaten
 * Das Präfix wird an dieser Stelle wie bei printf zusammengesetzt.
 *
 * \see		vhexdump, hexdump, setProgName, debugEnable, debugDisable
 */
void debugHexdump(const void *ptr,		/**< Zeiger auf die auszugebenden Daten */
		  size_t n,			/**< Anzahl der auszugebenden Bytes */
		  const char *fmt,		/**< Formatstring für das Präfix */
		  ...				/**< Zusätzliche Parameter, analog zu printf */
		 )
{
	va_list args;

	if(debug_enabled)
	{
		va_start(args, fmt);
		vhexdump(ptr, n, fmt, args);
		va_end(args);
	}
}


/**
 * \brief	Inhalt eines Puffers als Hexdump ausgeben
 *
 * Gibt den Inhalt des übergebenen Puffers als Hexdump auf der
 * Standardfehlerausgabe aus. Jede Zeile hat dabei folgenden Aufbau:
 * Programmname: Präfix: Hexdaten
 * Das Präfix wird an dieser Stelle wie bei printf zusammengesetzt.
 *
 * \see		vhexdump, debugHexdump, setProgName, debugEnable, debugDisable
 */
void hexdump(const void *ptr,		/**< Zeiger auf die auszugebenden Daten */
	     size_t n,			/**< Anzahl der auszugebenden Bytes */
	     const char *fmt,		/**< Formatstring für das Präfix */
	     ...			/**< Zusätzliche Parameter, analog zu printf */
	    )
{
	va_list args;

	va_start(args, fmt);
	vhexdump(ptr, n, fmt, args);
	va_end(args);
}


/**
 * \brief	Inhalt eines Puffers als Hexdump ausgeben
 *
 * Gibt den Inhalt des übergebenen Puffers als Hexdump auf der
 * Standardfehlerausgabe aus. Jede Zeile hat dabei folgenden Aufbau:
 * Programmname: Präfix: Hexdaten
 * Das Präfix wird an dieser Stelle wie bei vprintf zusammengesetzt.
 *
 * \see		debugHexdump, hexdump, setProgName, debugEnable, debugDisable
 */
void vhexdump(const void *ptr,		/**< Zeiger auf die auszugebenden Daten */
	      size_t n,			/**< Anzahl der auszugebenden Bytes */
	      const char *fmt,		/**< Formatstring, analog zu printf */
	      va_list args		/**< Argumentliste, passend zum Formatstring */
	     )
{
	const size_t charsPerLine = 16U;
	const size_t fullLines = n/charsPerLine;
	const size_t incompleteLine = n%charsPerLine;
	const unsigned char *array = (const unsigned char *)ptr;
	char byte;
	size_t line;
	size_t column;
	va_list a;
	int savedCancelState;

	savedCancelState = lockFile(stderr);	/* Stream sperren, sodass der Dump immer komplett und zusammenhängend ausgegeben wird */
	setStyle(stderr, STYLE_HEXDUMP);

	/* komplette Zeilen mit 16 Bytes ausgeben */
	for(line=0; line<fullLines; ++line)
	{
		/* Programmname voranstellen */
		fprintf(stderr, "%s: ", getProgName());

		/* nun das Präfix */
		va_copy(a, args);	/* Argumente kopieren, denn nach vfprintf wäre args ungültig (benötigt aber C99-Unterstützung) */
		vfprintf(stderr, fmt, a);
		va_end(a);
		fprintf(stderr, ": ");

		/* Bytes als Hexwerte ausgeben */
		for(column=0; column<charsPerLine; ++column)
			fprintf(stderr, "%02x ", (unsigned)array[line*charsPerLine + column]);

		/* Abstand einfügen */
		fprintf(stderr, "%7s", "");

		/* Bytes als ASCII-Zeichen anzeigen */
		for(column=0; column<charsPerLine; ++column)
		{
			byte = array[line*charsPerLine + column];
			fprintf(stderr, "%c ", isgraph(byte) ? byte : '.');
		}

		/* Zeile abschließen */
		fputc('\n', stderr);
	}

	/* letzte, unvollständige Zeile ausgeben */
	if(incompleteLine)
	{
		/* Programmname voranstellen */
		fprintf(stderr, "%s: ", getProgName());

		/* nun das Präfix */
		va_copy(a, args);	/* Argumente kopieren, denn nach vfprintf wäre args ungültig (benötigt aber C99-Unterstützung) */
		vfprintf(stderr, fmt, a);
		va_end(a);
		fprintf(stderr, ": ");

		/* Bytes als Hexwerte ausgeben */
		for(column=0; column<incompleteLine; ++column)
			fprintf(stderr, "%02x ", (unsigned)array[line*charsPerLine + column]);

		/* leere Hexstellen auffüllen */
		while(column++ < charsPerLine)
			fprintf(stderr, "   ");

		/* Abstand einfügen */
		fprintf(stderr, "%7s", "");

		/* Bytes als ASCII-Zeichen anzeigen */
		for(column=0; column<incompleteLine; ++column)
		{
			byte = array[line*charsPerLine + column];
			fprintf(stderr, "%c ", isgraph(byte) ? byte : '.');
		}

		/* Zeile abschließen */
		fputc('\n', stderr);
	}

	setStyle(stderr, STYLE_NORMAL);
	/* Sperre für Standardfehlerausgabe wieder freigeben */
	unlockFile(stderr, savedCancelState);	/* Sperre freigeben */
}


/**
 * \brief	Eine Zeile aus einem Eingabekanal lesen
 *
 * Liest eine Zeile aus einem Eingabekanal. Der Speicher dafür wird dynamisch mit malloc
 * reserviert. Der abschließende Zeilenumbruch wird durch ein Nullbyte ersetzt.
 *
 * \attention	Die Funktion ist nicht cancel-safe, da beim Abbruch der mit malloc reservierte
 *		Speicher verloren geht!
 *
 * \return	Die gelesene Zeile, oder NULL bei Fehler (errno wird entsprechend gesetzt)
 */
char *readLine(int fd		/**< Der Dateideskriptor, von dem gelesen werden soll */
	      )
{
	static const size_t bufferGrow = 512;
	size_t bufferSize = 0;
	size_t bufferPos = 0;
	char *buffer = NULL;
	char *newBuffer = NULL;

	for(;;)
	{
		/* mehr Speicher reservieren, falls Puffer voll */
		if(bufferPos >= bufferSize)
		{
			bufferSize += bufferGrow;
			newBuffer = realloc(buffer, bufferSize);
			if(newBuffer == NULL)
			{
				free(buffer);
				return NULL;
			}
			buffer = newBuffer;
		}

		/* Byte lesen */
		errno = 0;
		if(read(fd, &buffer[bufferPos], 1) < 1)
		{
			free(buffer);
			return NULL;
		}

		/* Zeilenumbruch gelesen? */
		if(buffer[bufferPos] == '\n')
		{
			buffer[bufferPos] = '\0';	/* Zeilenumbruch durch Nullbyte ersetzen */
			return buffer;	/* Puffer zurückgeben und beenden */
		}

		++bufferPos;
	}

	return NULL;	/* wird nie erreicht */
}

/**
 * \file	loader/util.h
 * \author	Stefan Gast
 *
 * \brief	Header für Hilfsfunktionen des Loaders
 */

#ifndef LOADER_UTIL_H
#define LOADER_UTIL_H

#include <unistd.h>

void write2stdout(const void *buf, size_t n, void (*cleanup_fn)(void));

#endif

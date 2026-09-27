/* Parser module for the SimpCalc language. */
#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

/* Parses a file and reports if it is a valid program. */
int parse(FILE *source, FILE *out, const char *filename);

#endif

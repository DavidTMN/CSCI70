/* Scanner module for the SimpCalc language. It turns source code into tokens. */
#ifndef SCANNER_H
#define SCANNER_H

#include <stdio.h>

#define MAX_LEXEME 256

/* All token types the scanner can return. */
typedef enum {
    T_IDENTIFIER,
    T_NUMBER,
    T_STRING,
    T_ASSIGN,
    T_SEMICOLON,
    T_COLON,
    T_COMMA,
    T_LEFTPAREN,
    T_RIGHTPAREN,
    T_PLUS,
    T_MINUS,
    T_MULTIPLY,
    T_DIVIDE,
    T_RAISE,
    T_LESSTHAN,
    T_EQUAL,
    T_GREATERTHAN,
    T_LTEQUAL,
    T_GTEQUAL,
    T_NOTEQUAL,
    T_ENDOFFILE,
    T_PRINT,
    T_IF,
    T_ELSE,
    T_ENDIF,
    T_SQRT,
    T_AND,
    T_OR,
    T_NOT,
    T_ERROR
} TokenType;

/* One token with its type, text, and line number. */
typedef struct {
    TokenType type;
    char lexeme[MAX_LEXEME];
    int line;
    const char *error;
} Token;

/* Prepares the scanner to read a new file. */
void scanner_init(FILE *source);

/* Returns the next token from the input. */
Token gettoken(void);

/* Returns the printed name of a token type. */
const char *token_name(TokenType type);

/* Prints one token or error line to the output file. */
void print_token(FILE *out, const Token *t);

#endif

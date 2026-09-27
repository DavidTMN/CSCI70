/* Scanner for SimpCalc. It follows our DFA to group characters into tokens. */
#include <ctype.h>
#include <string.h>
#include "scanner.h"

static FILE *src;
static int line;

/* Token names in the same order as the TokenType list. */
static const char *names[] = {
    "Identifier", "Number", "String", "Assign", "Semicolon", "Colon",
    "Comma", "LeftParen", "RightParen", "Plus", "Minus", "Multiply",
    "Divide", "Raise", "LessThan", "Equal", "GreaterThan", "LTEqual",
    "GTEqual", "NotEqual", "EndofFile", "Print", "If", "Else", "Endif",
    "Sqrt", "And", "Or", "Not", "Error"
};

/* Keywords and the token type of each one. */
static const struct {
    const char *word;
    TokenType type;
} keywords[] = {
    {"PRINT", T_PRINT}, {"IF", T_IF},     {"ELSE", T_ELSE}, {"ENDIF", T_ENDIF},
    {"SQRT", T_SQRT},   {"AND", T_AND},   {"OR", T_OR},     {"NOT", T_NOT}
};

/* Returns the printed name of a token type. */
const char *token_name(TokenType type)
{
    return names[type];
}

/* Sets the file to read and resets the line count. */
void scanner_init(FILE *source)
{
    src = source;
    line = 1;
}

/* Reads one character and counts new lines. */
static int next_char(void)
{
    int c = fgetc(src);
    if (c == '\n')
        line++;
    return c;
}

/* Puts a character back so the next token can read it. */
static void push_back(int c)
{
    if (c == EOF)
        return;
    if (c == '\n')
        line--;
    ungetc(c, src);
}

/* Adds a character to the end of the lexeme. */
static void add(Token *t, int c)
{
    size_t len = strlen(t->lexeme);
    if (len < MAX_LEXEME - 1) {
        t->lexeme[len] = (char)c;
        t->lexeme[len + 1] = '\0';
    }
}

/* Marks the token as a lexical error with a reason. */
static Token error(Token *t, const char *reason)
{
    t->type = T_ERROR;
    t->error = reason;
    return *t;
}

/* Reads an identifier and checks if it is a keyword. */
static Token scan_word(Token *t, int c)
{
    size_t i;

    do {
        add(t, c);
        c = next_char();
    } while (c != EOF && (isalnum(c) || c == '_'));
    push_back(c);

    t->type = T_IDENTIFIER;
    for (i = 0; i < sizeof keywords / sizeof keywords[0]; i++)
        if (strcmp(t->lexeme, keywords[i].word) == 0)
            t->type = keywords[i].type;
    return *t;
}

/* Reports a bad number and keeps the wrong character. */
static Token bad_number(Token *t, int c)
{
    if (c != EOF && isprint(c))
        add(t, c);
    return error(t, "Invalid number format");
}

/* Reads a number with optional decimal and exponent parts. */
static Token scan_number(Token *t, int c)
{
    do {
        add(t, c);
        c = next_char();
    } while (isdigit(c));

    if (c == '.') {
        add(t, c);
        c = next_char();
        if (!isdigit(c))
            return bad_number(t, c);
        do {
            add(t, c);
            c = next_char();
        } while (isdigit(c));
    }

    if (c == 'e' || c == 'E') {
        add(t, c);
        c = next_char();
        if (c == '+' || c == '-') {
            add(t, c);
            c = next_char();
        }
        if (!isdigit(c))
            return bad_number(t, c);
        do {
            add(t, c);
            c = next_char();
        } while (isdigit(c));
    }

    if (c == '.')
        return bad_number(t, c);

    push_back(c);
    t->type = T_NUMBER;
    return *t;
}

/* Reads a string that must close on the same line. */
static Token scan_string(Token *t)
{
    int c;

    add(t, '"');
    for (;;) {
        c = next_char();
        if (c == '"') {
            add(t, c);
            t->type = T_STRING;
            return *t;
        }
        if (c == '\n' || c == '\r' || c == EOF)
            return error(t, "Unterminated string");
        add(t, c);
    }
}

/* Reads an operator that may have one or two characters. */
static Token one_or_two(Token *t, int second, TokenType two, TokenType one)
{
    int c = next_char();
    if (c == second) {
        add(t, c);
        t->type = two;
    } else {
        push_back(c);
        t->type = one;
    }
    return *t;
}

/* Skips spaces and comments, then reads the next token. */
Token gettoken(void)
{
    Token t;
    int c;

    for (;;) {
        c = next_char();
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v')
            continue;
        if (c == '/') {
            int d = next_char();
            if (d == '/') {
                do
                    c = next_char();
                while (c != '\n' && c != EOF);
                continue;
            }
            push_back(d);
        }
        break;
    }

    memset(&t, 0, sizeof t);
    t.line = line;

    if (c == EOF) {
        t.type = T_ENDOFFILE;
        add(&t, ' ');
        return t;
    }
    if (isalpha(c) || c == '_')
        return scan_word(&t, c);
    if (isdigit(c))
        return scan_number(&t, c);
    if (c == '"')
        return scan_string(&t);

    add(&t, c);
    switch (c) {
    case ';': t.type = T_SEMICOLON;   return t;
    case ',': t.type = T_COMMA;       return t;
    case '(': t.type = T_LEFTPAREN;   return t;
    case ')': t.type = T_RIGHTPAREN;  return t;
    case '+': t.type = T_PLUS;        return t;
    case '-': t.type = T_MINUS;       return t;
    case '/': t.type = T_DIVIDE;      return t;
    case '=': t.type = T_EQUAL;       return t;
    case ':': return one_or_two(&t, '=', T_ASSIGN, T_COLON);
    case '*': return one_or_two(&t, '*', T_RAISE, T_MULTIPLY);
    case '<': return one_or_two(&t, '=', T_LTEQUAL, T_LESSTHAN);
    case '>': return one_or_two(&t, '=', T_GTEQUAL, T_GREATERTHAN);
    case '!':
        c = next_char();
        if (c == '=') {
            add(&t, c);
            t.type = T_NOTEQUAL;
            return t;
        }
        push_back(c);
        return error(&t, "Illegal character/character sequence");
    default:
        return error(&t, "Illegal character/character sequence");
    }
}

/* Prints a token or a lexical error message. */
void print_token(FILE *out, const Token *t)
{
    if (t->type == T_ERROR)
        fprintf(out, "Lexical Error: %s \"%s\" on line %d\n", t->error, t->lexeme, t->line);
    else
        fprintf(out, "%-30s %s\n", token_name(t->type), t->lexeme);
}

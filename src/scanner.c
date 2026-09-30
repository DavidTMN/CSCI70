/* Scanner for SimpCalc. */
#include <ctype.h>
#include <string.h>
#include "scanner.h"

static FILE *src;
static int line;
static TokenType last_token_type;

static const char *names[] = {
    "Identifier", "Number", "String", "Assign", "Semicolon", "Colon",
    "Comma", "LeftParen", "RightParen", "Plus", "Minus", "Multiply",
    "Divide", "Raise", "LessThan", "Equal", "GreaterThan", "LTEqual",
    "GTEqual", "NotEqual", "EndofFile", "Print", "If", "Else", "Endif",
    "Sqrt", "And", "Or", "Not", "Error"
};

static const struct {
    const char *word;
    TokenType type;
} keywords[] = {
    {"PRINT", T_PRINT}, {"IF", T_IF},
    {"ELSE", T_ELSE},   {"ENDIF", T_ENDIF},
    {"SQRT", T_SQRT},   {"AND", T_AND},
    {"OR", T_OR},       {"NOT", T_NOT}
};

const char *token_name(TokenType type)
{
    return names[type];
}

void scanner_init(FILE *source)
{
    src = source;
    line = 1;
    last_token_type = T_ERROR;
}

static int next_char(void)
{
    int c = fgetc(src);
    if (c == '\n')
        line++;
    return c;
}

static void push_back(int c)
{
    if (c == EOF)
        return;
    if (c == '\n')
        line--;
    ungetc(c, src);
}

static void add(Token *t, int c)
{
    size_t len = strlen(t->lexeme);
    if (len < MAX_LEXEME - 1) {
        t->lexeme[len] = (char)c;
        t->lexeme[len + 1] = '\0';
    }
}

static Token error(Token *t, const char *reason)
{
    t->type = T_ERROR;
    t->error = reason;
    return *t;
}

static Token scan_word(Token *t, int c)
{
    size_t i;
    do {
        add(t, c);
        c = next_char();
    } while (c != EOF && (isalnum(c) || c == '_'));
    push_back(c);
    t->type = T_IDENTIFIER;
    for (i = 0; i < sizeof keywords / sizeof keywords[0]; i++) {
        if (strcmp(t->lexeme, keywords[i].word) == 0)
            t->type = keywords[i].type;
    }
    return *t;
}

static Token bad_number(Token *t, int c)
{
    if (c == '\n')
        push_back(c);
    return error(t, "NUMBER");
}

static Token scan_number(Token *t, int c)
{
    do {
        add(t, c);
        c = next_char();
    } while (isdigit(c));

    if (c == '.') {
        int d = next_char();
        if (!isdigit(d)) {
            push_back(d);
            return bad_number(t, c);
        }
        add(t, c);
        c = d;
        do {
            add(t, c);
            c = next_char();
        } while (isdigit(c));
    }

    if (c == 'e' || c == 'E') {
        int d = next_char();
        if (d == '+' || d == '-') {
            int e = next_char();
            if (!isdigit(e)) {
                add(t, c);
                add(t, d);
                if (e != EOF && isprint(e))
                    add(t, e);
                return bad_number(t, e);
            }
            add(t, c);
            add(t, d);
            c = e;
        } else if (!isdigit(d)) {
            add(t, c);
            if (d != EOF && isprint(d))
                add(t, d);
            return bad_number(t, d);
        } else {
            add(t, c);
            c = d;
        }
        do {
            add(t, c);
            c = next_char();
        } while (isdigit(c));
    }

    push_back(c);
    t->type = T_NUMBER;
    return *t;
}

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
            return error(t, "STRING");
        add(t, c);
    }
}

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

Token gettoken(void)
{
    Token t;
    int c;
    int skipped_whitespace = 0;

    for (;;) {
        c = next_char();
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v') {
            skipped_whitespace = 1;
            continue;
        }
        if (c == '/') {
            int d = next_char();
            if (d == '/') {
                skipped_whitespace = 1;
                do {
                    c = next_char();
                } while (c != '\n' && c != EOF);
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
        last_token_type = t.type;
        return t;
    }

    if (isalpha(c) || c == '_') {
        Token res = scan_word(&t, c);
        last_token_type = res.type;
        return res;
    }

    if (isdigit(c)) {
        Token res = scan_number(&t, c);
        last_token_type = res.type;
        return res;
    }

    if (c == '"') {
        Token res = scan_string(&t);
        last_token_type = res.type;
        return res;
    }

    add(&t, c);
    switch (c) {
    case '.':
        if (!skipped_whitespace && last_token_type == T_NUMBER) {
            last_token_type = T_ERROR;
            return error(&t, "NUMBER");
        }
        last_token_type = T_ERROR;
        return error(&t, "ILLEGAL");
    case ';': t.type = T_SEMICOLON;  last_token_type = t.type; return t;
    case ',': t.type = T_COMMA;      last_token_type = t.type; return t;
    case '(': t.type = T_LEFTPAREN;  last_token_type = t.type; return t;
    case ')': t.type = T_RIGHTPAREN; last_token_type = t.type; return t;
    case '+': t.type = T_PLUS;       last_token_type = t.type; return t;
    case '-': t.type = T_MINUS;      last_token_type = t.type; return t;
    case '/': t.type = T_DIVIDE;     last_token_type = t.type; return t;
    case '=': t.type = T_EQUAL;      last_token_type = t.type; return t;
    case ':': {
        Token res = one_or_two(&t, '=', T_ASSIGN, T_COLON);
        last_token_type = res.type;
        return res;
    }
    case '*': {
        Token res = one_or_two(&t, '*', T_RAISE, T_MULTIPLY);
        last_token_type = res.type;
        return res;
    }
    case '<': {
        Token res = one_or_two(&t, '=', T_LTEQUAL, T_LESSTHAN);
        last_token_type = res.type;
        return res;
    }
    case '>': {
        Token res = one_or_two(&t, '=', T_GTEQUAL, T_GREATERTHAN);
        last_token_type = res.type;
        return res;
    }
    case '!':
        c = next_char();
        if (c == '=') {
            add(&t, c);
            t.type = T_NOTEQUAL;
            last_token_type = t.type;
            return t;
        }
        last_token_type = T_ERROR;
        return error(&t, "EXCLAMATION");
    default:
        last_token_type = T_ERROR;
        return error(&t, "ILLEGAL");
    }
}

void print_token(FILE *out, const Token *t)
{
    if (t->type == T_ERROR) {
        if (strcmp(t->error, "EXCLAMATION") == 0) {
            fprintf(out, "Lexical Error reading character ! on line %d\n", t->line);
            fprintf(out, "Error  on line %d\n", t->line);
        } else if (strcmp(t->error, "NUMBER") == 0) {
            fprintf(out, "Lexical Error: Invalid number format   on line %d\n", t->line);
            fprintf(out, "Error   on line %d\n", t->line);
        } else if (strcmp(t->error, "STRING") == 0) {
            fprintf(out, "Lexical Error: Unterminated  on line %d\n", t->line);
            fprintf(out, "Error   on line %d\n", t->line);
        } else {
            fprintf(out, "Lexical Error: Illegal character/character sequence   on line %d\n", t->line);
            fprintf(out, "Error  on line %d\n", t->line);
        }
    } else {
        fprintf(out, "%-30s %s\n", token_name(t->type), t->lexeme);
    }
}
/* Recursive descent parser for SimpCalc. Each grammar rule has its own function. */
#include "parser.h"
#include "scanner.h"

static Token tok;
static FILE *out;
static int incomplete_reported;
static int invalid_reported;

static int Blk(void);
static int Exp(void);

/* Writes one message line to the parse output. */
static void emit(const char *message)
{
    fprintf(out, "%s\n", message);
}

/* Moves to the next token. */
static void advance(void)
{
    tok = gettoken();
}

/* Checks that the current token is the expected one. */
static int match(TokenType expected)
{
    if (tok.type != expected) {
        emit("Symbol expected");
        return 0;
    }
    advance();
    return 1;
}

/* A program is a block followed by the end of file. */
static int Prg(void)
{
    return Blk() && match(T_ENDOFFILE);
}

/* Reads more print arguments separated by commas. */
static int Arg(void);
static int Argfollow(void)
{
    if (tok.type == T_COMMA)
        return match(T_COMMA) && Arg() && Argfollow();
    return 1;
}

/* An argument is a string or an expression. */
static int Arg(void)
{
    if (tok.type == T_STRING)
        return match(T_STRING);
    return Exp();
}

/* Ends an if statement with ENDIF or with an ELSE block. */
static int Iffollow(void)
{
    int ok;

    if (tok.type == T_ENDIF)
        return match(T_ENDIF) && match(T_SEMICOLON);
    if (tok.type == T_ELSE) {
        ok = match(T_ELSE) && Blk() && match(T_ENDIF) && match(T_SEMICOLON);
        if (!ok && !incomplete_reported) {
            emit("Incomplete if Statement");
            incomplete_reported = 1;
        }
        return ok;
    }
    emit("Symbol expected");
    return 0;
}

/* Accepts one relational operator. */
static int Rel(void)
{
    switch (tok.type) {
    case T_LESSTHAN:
    case T_EQUAL:
    case T_GREATERTHAN:
    case T_LTEQUAL:
    case T_NOTEQUAL:
    case T_GTEQUAL:
        advance();
        return 1;
    default:
        emit("Missing relational operator");
        return 0;
    }
}

/* A condition compares two expressions. */
static int Cnd(void)
{
    return Exp() && Rel() && Exp();
}

/* Parses an assignment, print, or if statement. */
static int Stm(void)
{
    int ok = 0;

    switch (tok.type) {
    case T_IDENTIFIER:
        ok = match(T_IDENTIFIER) && match(T_ASSIGN) && Exp() && match(T_SEMICOLON);
        if (ok)
            emit("Assignment Statement Recognized");
        break;
    case T_PRINT:
        ok = match(T_PRINT) && match(T_LEFTPAREN) && Arg() && Argfollow()
             && match(T_RIGHTPAREN) && match(T_SEMICOLON);
        if (ok)
            emit("Print Statement Recognized");
        break;
    case T_IF:
        emit("If Statement Begins");
        ok = match(T_IF) && Cnd() && match(T_COLON) && Blk() && Iffollow();
        if (ok)
            emit("If Statement Ends");
        break;
    default:
        break;
    }

    if (!ok && !invalid_reported) {
        emit("Invalid Statement");
        invalid_reported = 1;
    }
    return ok;
}

/* A block is zero or more statements. */
static int Blk(void)
{
    if (tok.type == T_IDENTIFIER || tok.type == T_PRINT || tok.type == T_IF)
        return Stm() && Blk();
    return 1;
}

/* A value is an identifier, a number, a SQRT call, or a grouped expression. */
static int Val(void)
{
    switch (tok.type) {
    case T_IDENTIFIER:
        return match(T_IDENTIFIER);
    case T_NUMBER:
        return match(T_NUMBER);
    case T_SQRT:
        return match(T_SQRT) && match(T_LEFTPAREN) && Exp() && match(T_RIGHTPAREN);
    default:
        return match(T_LEFTPAREN) && Exp() && match(T_RIGHTPAREN);
    }
}

/* A literal is a value with an optional minus sign. */
static int Lit(void)
{
    if (tok.type == T_MINUS)
        return match(T_MINUS) && Val();
    return Val();
}

/* Handles exponent operators. */
static int Litfollow(void)
{
    if (tok.type == T_RAISE)
        return match(T_RAISE) && Lit() && Litfollow();
    return 1;
}

/* A factor is a literal with its exponents. */
static int Fac(void)
{
    return Lit() && Litfollow();
}

/* Handles multiply and divide. */
static int Facfollow(void)
{
    if (tok.type == T_MULTIPLY)
        return match(T_MULTIPLY) && Fac() && Facfollow();
    if (tok.type == T_DIVIDE)
        return match(T_DIVIDE) && Fac() && Facfollow();
    return 1;
}

/* A term is a factor with its multiply and divide parts. */
static int Trm(void)
{
    return Fac() && Facfollow();
}

/* Handles plus and minus. */
static int Trmfollow(void)
{
    if (tok.type == T_PLUS)
        return match(T_PLUS) && Trm() && Trmfollow();
    if (tok.type == T_MINUS)
        return match(T_MINUS) && Trm() && Trmfollow();
    return 1;
}

/* An expression is a term with its plus and minus parts. */
static int Exp(void)
{
    return Trm() && Trmfollow();
}

/* Parses a whole file and prints if it is valid. */
int parse(FILE *source, FILE *output, const char *filename)
{
    int valid;

    out = output;
    incomplete_reported = 0;
    invalid_reported = 0;
    scanner_init(source);
    advance();

    valid = Prg();
    fprintf(out, "%s is %sa valid SimpCalc program\n", filename, valid ? "" : "not ");
    return valid;
}

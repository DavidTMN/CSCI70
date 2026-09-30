/* Recursive descent parser for SimpCalc. */
#include "parser.h"
#include "scanner.h"

static Token tok;
static FILE *out;
static int parse_error;

static int Blk(void);
static int Exp(void);

static void emit(const char *message)
{
    fprintf(out, "%s\n", message);
}

static void advance(void)
{
    tok = gettoken();
}

static int match(TokenType expected)
{
    if (parse_error)
        return 0;
    if (tok.type != expected) {
        fprintf(out, "Parse Error on line %d: %s Expected.\n", tok.line, token_name(expected));
        parse_error = 1;
        return 0;
    }
    advance();
    return 1;
}

static int Prg(void)
{
    return Blk() && match(T_ENDOFFILE);
}

static int Arg(void);
static int Argfollow(void)
{
    if (parse_error) return 0;
    if (tok.type == T_COMMA)
        return match(T_COMMA) && Arg() && Argfollow();
    return 1;
}

static int Arg(void)
{
    if (parse_error) return 0;
    if (tok.type == T_STRING)
        return match(T_STRING);
    return Exp();
}

static int Iffollow(void)
{
    if (parse_error) return 0;
    if (tok.type == T_ENDIF)
        return match(T_ENDIF) && match(T_SEMICOLON);
    if (tok.type == T_ELSE)
        return match(T_ELSE) && Blk() && match(T_ENDIF) && match(T_SEMICOLON);

    match(T_ENDIF);
    return 0;
}

static int Rel(void)
{
    if (parse_error) return 0;
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
        fprintf(out, "Missing relational operator\n");
        parse_error = 1;
        return 0;
    }
}

static int Cnd(void)
{
    if (parse_error) return 0;
    return Exp() && Rel() && Exp();
}

static int Stm(void)
{
    int ok = 0;
    if (parse_error) return 0;

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
    return ok;
}

static int Blk(void)
{
    if (parse_error) return 0;
    if (tok.type == T_IDENTIFIER || tok.type == T_PRINT || tok.type == T_IF)
        return Stm() && Blk();
    return 1;
}

static int Val(void)
{
    if (parse_error) return 0;
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

static int Lit(void)
{
    if (parse_error) return 0;
    if (tok.type == T_MINUS)
        return match(T_MINUS) && Val();
    return Val();
}

static int Litfollow(void)
{
    if (parse_error) return 0;
    if (tok.type == T_RAISE)
        return match(T_RAISE) && Lit() && Litfollow();
    return 1;
}

static int Fac(void)
{
    if (parse_error) return 0;
    return Lit() && Litfollow();
}

static int Facfollow(void)
{
    if (parse_error) return 0;
    if (tok.type == T_MULTIPLY)
        return match(T_MULTIPLY) && Fac() && Facfollow();
    if (tok.type == T_DIVIDE)
        return match(T_DIVIDE) && Fac() && Facfollow();
    return 1;
}

static int Trm(void)
{
    if (parse_error) return 0;
    return Fac() && Facfollow();
}

static int Trmfollow(void)
{
    if (parse_error) return 0;
    if (tok.type == T_PLUS)
        return match(T_PLUS) && Trm() && Trmfollow();
    if (tok.type == T_MINUS)
        return match(T_MINUS) && Trm() && Trmfollow();
    return 1;
}

static int Exp(void)
{
    if (parse_error) return 0;
    return Trm() && Trmfollow();
}

int parse(FILE *source, FILE *output, const char *filename)
{
    int valid;
    out = output;
    parse_error = 0;
    scanner_init(source);
    advance();
    valid = Prg();
    if (valid && !parse_error) {
        fprintf(out, "%s is a valid SimpCalc program\n", filename);
        return 1;
    }
    return 0;
}
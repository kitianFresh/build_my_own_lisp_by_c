#include<stdio.h>
#include<stdlib.h>
#include "mpc.h"

/* If we compiling on Windows compile these functions */
#ifdef _WIN32
#include<string.h>

static char buffer[2048];

/* Fake readline function */
char *readline(char *prompt) {
    fputs("lispy> ", stdout);
    fgets(buffer, 2048, stdin);
    char *cpy = (char*)malloc(strlen(buffer) + 1);
    strcpy(cpy, buffer);
    //cpy[strlen(cpy)-1] = '\0';
    return cpy;
}

/* Fake add_history function */
void add_history(char* unused) {}

#else
#include<editline/readline.h>
#include<editline/history.h>
#endif


int main(int argc, char **argv) {
    /* Create Some Parsers */
    mpc_parser_t* Decimal   = mpc_new("decimal");
    mpc_parser_t* Integer   = mpc_new("integer");
    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Operator  = mpc_new("operator"); 
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* Lispy     = mpc_new("lispy");

    /*Define them with the following Language */
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                                                               \
         decimal    : /-?[0-9]+/;                                                                 \
         integer    : /-?[0-9]+/;                                                                       \
         number     : <integer> | <decimal>;                                                            \
         operator   : '+' | '-' | '*' | '/' | '%' | \"add\" | \"sub\" | \"mul\" | \"div\";              \
         expr       : <number> | '(' <operator> <expr>+ ')';                                            \
         lispy      : /^/ <operator> <expr>+ /$/;                                                       \
        ",
        Decimal, Integer, Number, Operator, Expr, Lispy);

    puts("Lispy version 0.0.0.0.2");
    puts("Press Ctrl+c to Exit\n");

    while (1) {
        /* Output our prompt and get input */
        char *input = readline("lispy> ");

        /* Add input to history */
        add_history(input);
        
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            /* On Success Print the AST */
            mpc_ast_print(r.output);
            mpc_ast_delete(r.output);
        }
        else {
            /* Otherwise Print the Error */
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    /* Undefine and Delete our Parser */
    mpc_cleanup(6, Decimal, Integer, Number, Operator, Expr, Lispy);

    return 0;
}

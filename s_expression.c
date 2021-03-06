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
    //这句其实可以不要，但是为了保险，再一次添加string terminator '\0'
    cpy[strlen(cpy)-1] = '\0';
    return cpy;
}

/* Fake add_history function */
void add_history(char* unused) {}

#else
#include<editline/readline.h>
#include<editline/history.h>
#endif


/* Create Enumeration of Possible lval Types */
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };

/* Declare New lval Struct */
typedef struct lval {
    int type;
    long num;
    /* Error and Symbol types have some string data */
    char* err;
    char* sym;

    /* Count and pointer to a list of 'lval*' ,因此要用二级指针 */
    int count;
    struct lval** cell;
} lval;

/* Construct a pointer to a new Number lval */
lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

/* Construct a pointer to a new Error lval */
lval* lval_err(char* m) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
  return v;
}

/* Construct a pointer to a new Symbol lval */
lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

/* A pointer to a new empty Sexpr lval */
lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

void lval_del(lval* v) {

    switch (v->type) {
        /* Do nothing sepcial for number type */
        case LVAL_NUM: break;

        /* For Err or Sym free the string data */
        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;

        /* If Sexpr then delete all elements inside */
        case LVAL_SEXPR:
            for (int i=0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            /* Also free the memory allocated to contain the pointers */
            free(v->cell);
        break;
    }

    /* Free the memory allocated for the 'lval' struct itself */
    free(v);
}

lval* lval_read_num(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno !=ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval* lval_add(lval* v, lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}

lval* lval_read(mpc_ast_t* t) {

    /* If Symbol or Number return conversion to that type */
    if (strstr(t->tag, "number")) { return lval_read_num(t); }
    if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }
    //printf("lval_read debug\n");
    /* If root (>) or sexpr then create empty list */
    lval* x = NULL;
    if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
    if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }

    /* Fill this list with any valid expression contained within */
    for (int i=0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
        if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}


void lval_print(lval* v);

void lval_expr_print(lval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {

    /* Print Value contained within */
    lval_print(v->cell[i]);

    /* Don't print trailing space if last element */
    if (i != (v->count-1)) {
        putchar(' ');
    }
  }
  putchar(close);
}

void lval_print(lval* v) {
    switch (v->type) {
        case LVAL_NUM:   printf("%li", v->num); break;
        case LVAL_ERR:   printf("Error: %s", v->err); break;
        case LVAL_SYM:   printf("%s", v->sym); break;
        case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
  }
}

/* Print an "lval" followed by a newline */
void lval_println(lval* v) { lval_print(v); putchar('\n'); }

int main(int argc, char **argv) {
    /* Create Some Parsers */
    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Symbol    = mpc_new("symbol");
    mpc_parser_t* Sexpr     = mpc_new("sexpr"); 
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* Lispy     = mpc_new("lispy");

    /*Define them with the following Language */
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                                                               \
         number     : /-?[0-9]+/;                                                                       \
         symbol     : '+' | '-' | '*' | '/' | '%' | '^';                                                \
         sexpr      : '(' <expr>* ')';                                                                  \
         expr       : <number> | <symbol> | <sexpr>;                                                    \
         lispy      : /^/ <expr>* /$/;                                                                  \
        ",
        Number, Symbol, Sexpr, Expr, Lispy);

    puts("Lispy version 0.0.0.0.5");
    puts("Press Ctrl+c to Exit\n");

    while (1) {
        /* Output our prompt and get input */
        char *input = readline("lispy> ");

        /* Add input to history */
        add_history(input);
        
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            
            lval* x = lval_read(r.output);
            lval_println(x);
            lval_del(x);
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
    mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);

    return 0;
}

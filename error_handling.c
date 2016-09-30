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

/* Create Enumeration of Possible Error Types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* Create Enumeration of Possible lval Types */
enum { LVAL_NUM, LVAL_ERR };

/* Declare New lval Struct */
typedef struct {
    int type;
    long num;
    int err;
} lval;

/* Create a new number type lval */
lval lval_num(long x) {
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}

/* Create a new error type lval */
lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
}

/* Print an "lval" */
void lval_print(lval v) {
    switch (v.type) {
        /* In the case the type is a number print it */
        /* Then 'break' out of the switch. */
        case LVAL_NUM: printf("%li", v.num); break;
        
        /* In the case the type is an error */
        case LVAL_ERR:
            /* Check what type of error it is and print it */
            if (v.err == LERR_DIV_ZERO) {
                printf("Error: Division By Zero!");
            }
            if (v.err == LERR_BAD_OP)   {
                printf("Error: Invalid Operator!");
            }
            if (v.err == LERR_BAD_NUM)  {
                printf("Error: Invalid Number!");
            }
        break;
    }
}

/* Print an "lval" followed by a newline */
void lval_println(lval v) { lval_print(v); putchar('\n'); }

int number_of_nodes(mpc_ast_t* t) {
    if (t->children_num == 0) { return 1; }
    if (t->children_num >= 1) {
        int total = 1;
        for (int i=0; i < t->children_num; i++) {
            total += number_of_nodes(t->children[i]);
        }
        return total;
    }
    return 0;
}

int number_of_leaves(mpc_ast_t* t) {
    if (t->children_num == 0) { return 1; }
    if (t->children_num >= 1) {
        int leaves = 0;
        for (int i=0; i < t->children_num; i++) {
            leaves += number_of_leaves(t->children[i]);
        }
        return leaves;
    }
    return 1;
}

int number_of_branches(mpc_ast_t* t) {
    if (t->children_num == 0) { return 0; }
    if (t->children_num >= 1) {
        int branches = t->children_num;
        for (int i=0; i < t->children_num; i++) {
            branches += number_of_branches(t->children[i]);
        }
        return branches;
    }
    return 0;
}

int most_number_of_children(mpc_ast_t* t) {
    if (t->children_num == 0) { return 0; }
    if (t->children_num >= 1) {
        int max = t->children_num;
        for (int i=0; i < t->children_num; i++) {
            int childrens = most_number_of_children(t->children[i]); 
            if (childrens > max) {
                max = childrens;
            }
        }
        return max;
    }
    return 0;
}


/* Use operator string to see which operation to perform */
lval eval_op(lval x, char* op, lval y) {
    /* If either value is an error return it */
    if (x.type == LVAL_ERR) { return x; }
    if (y.type == LVAL_ERR) { return y; }

    /* Otherwise do maths on the number values */
    if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
    if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
    if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
    if (strcmp(op, "/") == 0) {
        /* If second operand is zero return error */
        return y.num == 0
            ? lval_err(LERR_DIV_ZERO)
            : lval_num(x.num / y.num);
    }
    if (strcmp(op, "%") == 0) {
        return y.num == 0
            ? lval_err(LERR_DIV_ZERO)
            : lval_num(x.num % y.num);
    }
    if (strcmp(op, "^") == 0) { return lval_num(pow(x.num, y.num)); }
    return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {
    /* If tagged as number return it directly. */
    if (strstr(t->tag, "number")) {
        /* Check if there is some error in conversion */
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    /* The operator is always second child because the first child is always '(' and final child is always ')' */
    char* op = t->children[1]->contents;

    /* We store the third child in `x` */
    lval x = eval(t->children[2]);

    /* Iterate the remaining children and combining. */
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

int main(int argc, char **argv) {
    /* Create Some Parsers */
    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Operator  = mpc_new("operator"); 
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* Lispy     = mpc_new("lispy");

    /*Define them with the following Language */
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                                                               \
         number     : /-?[0-9]+/;                                                                       \
         operator   : '+' | '-' | '*' | '/' | '%' | '^';                                                \
         expr       : <number> | '(' <operator> <expr>+ ')';                                            \
         lispy      : /^/ <operator> <expr>+ /$/;                                                       \
        ",
        Number, Operator, Expr, Lispy);

    puts("Lispy version 0.0.0.0.3");
    puts("Press Ctrl+c to Exit\n");

    while (1) {
        /* Output our prompt and get input */
        char *input = readline("lispy> ");

        /* Add input to history */
        add_history(input);
        
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            
            lval result = eval(r.output);
            lval_println(result);
            //mpc_ast_print(r.output);
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
    mpc_cleanup(4, Number, Operator, Expr, Lispy);

    return 0;
}

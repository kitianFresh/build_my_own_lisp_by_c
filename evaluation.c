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
long eval_op(long x, char* op, long y) {
    if (strcmp(op, "+") == 0) { return x + y; }
    if (strcmp(op, "-") == 0) { return x - y; }
    if (strcmp(op, "*") == 0) { return x * y; }
    if (strcmp(op, "/") == 0) { return x / y; }
    if (strcmp(op, "%") == 0) { return x % y; }
    if (strcmp(op, "^") == 0) { return pow(x, y); }
    return 0;
}

long eval(mpc_ast_t* t) {
    /* If tagged as number return it directly. */
    if (strstr(t->tag, "number")) {
        return atoi(t->contents);
    }

    /* The operator is always second child because the first child is always '(' and final child is always ')' */
    char* op = t->children[1]->contents;

    /* We store the third child in `x` */
    long x = eval(t->children[2]);

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
            
            long result = eval(r.output);
            printf("%li\n", result);
            mpc_ast_print(r.output);
           // pirntf("%li\n", number_of_nodes(r.output), number_of_leaves(r.output),
             //       number_of_branches(r.ouput), most_number_of_children(r.output));
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

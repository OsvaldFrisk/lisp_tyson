#include "mpc.h"
#include <editline/readline.h>


enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };
enum { LVAL_NUM, LVAL_ERR };

typedef struct {
  int type;
  long num;
  int err;
} lval;

lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

int number_of_nodes(mpc_ast_t* t){
  if(t->children_num == 0) { return 1; }
  else if(t->children_num >= 1) {
    int total = 1;
    for (int i = 0; i < t->children_num; i++){
      total = total + number_of_nodes(t->children[i]);
    }
    return total;
  }
  return 0;
}

long eval_op(long x, char* op, long y){
  if (strcmp(op, "+") == 0) { return x + y; }
  if (strcmp(op, "-") == 0) { return x - y; }
  if (strcmp(op, "*") == 0) { return x * y; }
  if (strcmp(op, "/") == 0) { return x / y; }
  return 0;
}

long eval(mpc_ast_t* t){
  // If tagged as number we can return.
  if(strstr(t->tag, "number")) {
    return atoi(t->contents);
  }

  // The operator is always the second child.
  char* op = t->children[1]->contents;

  // We store the third child in x.
  long x = eval(t->children[2]);

  // Iterate the remaining children then combining.
  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }
  return x;
}


int main(int argc, char** argv) {
  
  /* Create Some Parsers */
  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Tyson = mpc_new("tyson");
  
  /* Define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                     \
      number   : /-?[0-9]+/ ;                             \
      operator : '+' | '-' | '*' | '/' ;                  \
      expr     : <number> | '(' <operator> <expr>+ ')' ;  \
      tyson    : /^/ <operator> <expr>+ /$/ ;             \
    ",
    Number, Operator, Expr, Tyson);
  
  puts("Tyson Version 0.0.0.0.2");
  puts("Press Ctrl+c to Exit\n");
  
  while (1) {
  
    char* input = readline("tyson> ");
    add_history(input);
    
    /* Attempt to parse the user input */
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Tyson, &r)) {
      /* On success print and delete the AST */
      //mpc_ast_print(r.output);
      long result = eval(r.output);
      printf("%li\n", result);
      mpc_ast_delete(r.output);


      /*
      mpc_ast_t* output = r.output;
      printf("Tag : %s\n", output->tag);
      printf("Contents : %s\n", output->contents);
      printf("Number of children : %i\n", output->children_num);

      mpc_ast_t* c0 = output->children[0];
      printf("First Child Tag: %s\n", c0->tag);
      printf("First Child Contents: %s\n", c0->contents);
      printf("First Child Number of children: %i\n", c0->children_num);
      */
      


    } else {
      /* Otherwise print and delete the Error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
    
    free(input);
  }
  
  /* Undefine and delete our parsers */
  mpc_cleanup(4, Number, Operator, Expr, Tyson);
  
  return 0;
}

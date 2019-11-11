#include "mpc.h"
#include <editline/readline.h>


// enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR };

typedef struct lval {
  int type;
  long num;

  // Error and Symbol types
  char* err;
  char* sym;

  // Count and Pointer to list of lval points
  int count;
  struct lval** cell;
} lval;

// Prototypes
void lval_print(lval* v);
lval* lval_eval(lval* v);
lval* lval_eval_sexpr(lval* v);



lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

lval* lval_err(char* m) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
  return v;
}

lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}


void lval_del(lval* v) {
  switch (v->type)
  {
  // Nothing special for numbers 
  case LVAL_NUM: break;

  // Err or Sym free string data
  case LVAL_ERR: free(v->err); break;
  case LVAL_SYM: free(v->sym); break;

  // Sexpr delete all elements inside
  case LVAL_SEXPR:
    for (int i = 0; i < v->count; i++)
    {
      lval_del(v->cell[i]);
    }
    // free momeory for pointers
    free(v->cell);
  break;
  }

  // Free v's memory
  free(v);
}


lval* lval_read_num(mpc_ast_t* t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  if (errno != ERANGE) {
    return lval_num(x);
  }
  else {
    return lval_err("invalid number");
  }
}

lval* lval_add(lval* v, lval* x) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = x;
  return v;
}

lval* lval_read(mpc_ast_t* t) {

  // Symbol or Number conversion to type
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

  // if root (>) or Sexpr, create empty list
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
  if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }

  // Fill list with valid experssion constained within
  for (int i = 0; i < t->children_num; i++)
  {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
    x = lval_add(x, lval_read(t->children[i]));
  }
  return x;
}

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; i++)
  {
    // Print value within
    lval_print(v->cell[i]);

    // Don't print tailing space if last element
    if (i != (v->count-1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

void lval_print(lval* v) {
  switch(v->type) {
    case LVAL_NUM:    printf("%li", v->num);  break;
    case LVAL_ERR:    printf("Error: %s", v->err);  break;
    case LVAL_SYM:    printf("%s", v->sym);  break;
    case LVAL_SEXPR:  lval_expr_print(v, '(', ')');  break;
  }
}

void lval_println(lval* v) { lval_print(v); putchar('\n'); }


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


lval* lval_pop(lval* v, int i) { 
  // Find the item at i
  lval* x = v->cell[i];

  // Shift memory after "i"
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));

  // Update list count
  v->count--;

  // Reallocate the memory used
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval* builtin_op(lval* a, char* op) {
  // Check all arguments for type number
  for (int i = 0; i < a->count; i++)
  {
    if (a->cell[i]->type != LVAL_NUM){
      lval_del(a);
      return lval_err("Cannot operate on non-number!");
    }
  }

  // Pop first element
  lval* x = lval_pop(a, 0);

  // If no arguments and sub then perform unary negation
  if ((strcmp(op, "-") == 0) && a->count == 0) {
    x->num = -x->num;
  }

  // While there are elements remaining
  while (a->count > 0) {
    
    lval* y = lval_pop(a, 0);

    if (strcmp(op, "+") == 0) { x->num += y->num; }
    if (strcmp(op, "-") == 0) { x->num -= y->num; }
    if (strcmp(op, "*") == 0) { x->num *= y->num; }
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        lval_del(x);
        lval_del(y);
        x = lval_err("Division by zero!");
        break;
      }
      x->num /= y->num;
     }
    lval_del(y);
  }
  lval_del(a);
  return x;
}


lval* lval_eval(lval* v) {
  // Evaluate S-expression
  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }
  // Other types remain the same
  return v;
}

lval* lval_eval_sexpr(lval* v) {
  // Evaluating children
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
  }

  // Error checking
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
  }

  // Empty and Single expression
  if (v->count == 0) { return v; }
  if (v->count == 1) { return lval_take(v, 0); }

  // Ensure first element is symbol
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f);
    lval_del(v);
    return lval_err("S-expression does not start with symbol!");
  }

  // Call builtin with operator
  lval* result = builtin_op(v, f->sym);
  lval_del(f);
  return result;
}

// lval eval_op(lval x, char* op, lval y){

//   // returning value on value error
//   if (x.type == LVAL_ERR) { return x; }
//   if (y.type == LVAL_ERR) { return y; }


//   if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
//   if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
//   if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
//   if (strcmp(op, "/") == 0) {
//     if (y.num == 0) { return lval_err(LERR_DIV_ZERO); }
//     else            { return lval_num(x.num / y.num); }
//   }

//   return lval_err(LERR_BAD_OP);
// }

// lval eval(mpc_ast_t* t){
//   // If tagged as number we can return.
//   if(strstr(t->tag, "number")) {
//     errno = 0;
//     long x = strtol(t->contents, NULL, 10);
//     if (errno != ERANGE) {
//       return lval_num(x);
//     }
//     else {
//       return lval_err(LERR_BAD_NUM);
//     }
//     // return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
//   }

//   // The operator is always the second child.
//   char* op = t->children[1]->contents;

//   // We store the third child in x.
//   lval x = eval(t->children[2]);

//   // Iterate the remaining children then combining.
//   int i = 3;
//   while (strstr(t->children[i]->tag, "expr")) {
//     x = eval_op(x, op, eval(t->children[i]));
//     i++;
//   }
//   return x;
// }


int main(int argc, char** argv) {
  
  /* Create Some Parsers */
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr  = mpc_new("sexpr");
  mpc_parser_t* Expr   = mpc_new("expr");
  mpc_parser_t* Tyson  = mpc_new("tyson");
  
  /* Define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                          \
      number : /-?[0-9]+/ ;                    \
      symbol : '+' | '-' | '*' | '/' ;         \
      sexpr  : '(' <expr>* ')' ;               \
      expr   : <number> | <symbol> | <sexpr> ; \
      tyson  : /^/ <expr>* /$/ ;               \
    ",
    Number, Symbol, Sexpr, Expr, Tyson);
  
  puts("Tyson Version 0.0.0.0.6");
  puts("Press Ctrl+c to Exit\n");
  
  while (1) {
  
    char* input = readline("tyson> ");
    add_history(input);
    
    /* Attempt to parse the user input */
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Tyson, &r)) {
      /* On success print and delete the AST */
      //mpc_ast_print(r.output);
      // lval result = eval(r.output);
      // lval* x = lval_read(r.output);
      lval* x = lval_eval(lval_read(r.output));
      lval_println(x);
      lval_del(x);
      // mpc_ast_delete(r.output);


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
  mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Tyson);
  
  return 0;
}

#include "mpc.h"
#include <editline/readline.h>

#define LASSERT(args, cond, err) \
  if (!(cond)) { lval_del(args); return lval_err(err); }

// Forward Declarations

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

// Lisp value
enum { LVAL_ERR, LVAL_NUM,    LVAL_FUN,
       LVAL_SYM, LVAL_SEXPR,  LVAL_QEXPR };

typedef lval* (*lbuiltin)(lenv*, lval*);

struct lval {
  int type;
  long num;

  // Error and Symbol types
  char* err;
  char* sym;

  // Function ponter
  lbuiltin fun;

  // Count and Pointer to list of lval points
  int count;
  struct lval** cell;
};

// maps the relationship between variable names and values
struct lenv {
  int count;
  char** syms;
  lval** vals;
};

void lval_print(lval* v);
lval* lval_eval(lenv* e, lval* v);
lval* lval_eval_sexpr(lenv* e, lval* v);
lval* lval_join(lval* x, lval* y);
lval* lval_copy(lval* v);



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

// A pointer to a new empty Qexpr lval
lval* lval_qexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

// lval function constructor
lval* lval_fun(lbuiltin func) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->fun = func;
  return v;
}

lenv* lenv_new(void) {
  lenv* e = malloc(sizeof(lenv));
  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  return e;
}

lval* lenv_get(lenv* e, lval* k) {
  // Iterate over all items in enviornment
  for (int i = 0; i < e->count; i++)
  {
    // Check for matching variable
    if (strcmp(e->syms[i], k->sym) == 0) {
      return lval_copy(e->vals[i]);
    }
  }
  
  // If no matching variable return error
  return lval_err("unbound symbol!");
}



void lval_del(lval* v) {
  switch (v->type)
  {
  // Nothing special for numbers and functions
  case LVAL_NUM: break;
  case LVAL_FUN: break;

  // Err or Sym free string data
  case LVAL_ERR: free(v->err); break;
  case LVAL_SYM: free(v->sym); break;

  // Sexpr and Qexpr delete all elements inside
  case LVAL_QEXPR:
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

void lenv_put(lenv* e, lval* k, lval* v) {
  // Iterate over all variables in environment
  // to check if variable already exists
  for (int i = 0; i < e->count; i++) {
    // If variable is found, replace the value with new
    if (strcmp(e->syms[i], k->sym) == 0) {
      lval_del(e->vals[i]);
      e->vals[i] = lval_copy(v);
      return;
    }
  }
  // If no existing variable was found allocate space for new
  e->count++;
  e->vals = realloc(e->vals, sizeof(lval*) * e->count);
  e->syms = realloc(e->syms, sizeof(char*) * e->count);

  // Copy content of lval and symbol string into new location
  e->vals[e->count-1] = lval_copy(v);
  e->syms[e->count-1] = malloc(strlen(k->sym)+1);
  strcpy(e->syms[e->count-1], k->sym);
}

void lenv_del(lenv* e) {
  for (int i = 0; i < e->count; i++) {
    free(e->syms[i]);
    lval_del(e->vals[i]);
  }
  free(e->syms);
  free(e->vals);
  free(e);
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
  if (strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }

  // Fill list with valid experssion constained within
  for (int i = 0; i < t->children_num; i++)
  {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
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
    case LVAL_ERR:    printf("Error: %s", v->err);  break;
    case LVAL_NUM:    printf("%li", v->num);  break;
    case LVAL_FUN:    printf("<function>"); break;
    case LVAL_SYM:    printf("%s", v->sym);  break;
    case LVAL_SEXPR:  lval_expr_print(v, '(', ')');  break;
    case LVAL_QEXPR:  lval_expr_print(v, '{', '}');  break;
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


lval* builtin_op(lenv* e, lval* a, char* op) {
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

lval* builtin_add(lenv* e, lval* a) {
  return builtin_op(e, a, "+");
}

lval* builtin_sub(lenv* e, lval* a) {
  return builtin_op(e, a, "-");
}

lval* builtin_mul(lenv* e, lval* a) {
  return builtin_op(e, a, "*");
}

lval* builtin_div(lenv* e, lval* a) {
  return builtin_op(e, a, "/");
}

lval* builtin_head(lenv* e, lval* a) {
  // Check Error conditions 
  LASSERT(a, a->count == 1, "Function 'head' passed too many arguments!")
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'head' passed incorrect types!")
  LASSERT(a, a->cell[0]->count != 0, "Function 'head' passed {}!")

  // Take first argument
  lval* v = lval_take(a, 0);

  // Delete all elements that are not head and return
  while (v->count > 1) {
    lval_del(lval_pop(v, 1));
  }

  return v;
}

lval* builtin_tail(lenv* e, lval* a) {
  // Check Error conditions 
  LASSERT(a, a->count == 1, "Function 'tail' passed too many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'tail' passed incorrect types!");
  LASSERT(a, a->cell[0]->count != 0, "Function 'tail' passed {}!");

  // Take first argument
  lval* v = lval_take(a, 0);

  // Delete first element and return rest
  lval_del(lval_pop(v, 0));
  return v;
}

lval* builtin_list(lenv* e, lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval* builtin_eval(lenv* e, lval* a) {
  LASSERT(a, a->count == 1, "Function 'eval' passed to many arguments!");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'eval' passed incorrect type!")

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(e, x);
}

lval* builtin_join(lenv* e, lval* a) {

  for (int i = 0; i < a->count; i++)
  {
    LASSERT(a, a->cell[i]->type == LVAL_QEXPR, "Function 'join' passed incorrect type!");
  }

  lval* x = lval_pop(a, 0);

  while (a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);
  return x;
}

lval* builtin_def(lenv* e, lval* a) {
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR, "Function 'def' passed incorrect type!")

  // First argument is symbol list
  lval* syms = a->cell[0];

  // Check list for only symbols
  for (int i = 0; i < syms->count; i++) {
    LASSERT(a, syms->cell[i]->type == LVAL_SYM, "Function 'def' cannot define non-symbol");
  }
    // Check valid number of symbols and values
    LASSERT(a, syms->count == a->count-1, "Function 'def' cannot define incorrect number of values to symbols")

    // Assign copies of values to symbols
    for (int i = 0; i < syms->count; i++) {
      lenv_put(e, syms->cell[i], a->cell[i+1]);
    }

  lval_del(a);
  return lval_sexpr();
}

lval* builtin(lenv* e, lval* a, char* func) {
  if (strcmp("list", func) == 0) { return builtin_list(e, a); }
  if (strcmp("head", func) == 0) { return builtin_head(e, a); }
  if (strcmp("tail", func) == 0) { return builtin_tail(e, a); }
  if (strcmp("join", func) == 0) { return builtin_join(e, a); }
  if (strcmp("eval", func) == 0) { return builtin_eval(e, a); }
  if (strstr("+-/*", func)) { return builtin_op(e, a, func); }
  lval_del(a);
  return lval_err("Unknown Function!");
}

lval* lval_join(lval* x, lval* y) {
  // For each cell in y, add it to x
  while (y->count) {
    x = lval_add(x, lval_pop(y, 0));
  }

  // Delete the empty y and return x
  lval_del(y);
  return x;
}

lval* lval_copy(lval* v) {
  lval* x = malloc(sizeof(lval));
  x->type = v->type;

  switch (v->type) {

    // Copy functions and numbers
    case LVAL_NUM: x->num = v->num; break;
    case LVAL_FUN: x->fun = v->fun; break;

    // Copy error strings
    case LVAL_ERR:
      x->err = malloc(strlen(v->err) + 1);
      strcpy(x->err, v->err); break;

    // Copy symbol string
    case LVAL_SYM:
      x->sym = malloc(strlen(v->sym) + 1);
      strcpy(x->sym, v->sym); break;

    case LVAL_SEXPR:
    case LVAL_QEXPR:
      x->count = v->count;
      x->cell = malloc(sizeof(lval*) * x->count);
      for (int i = 0; i < x->count; i++) {
        x->cell[i] = lval_copy(v->cell[i]);
      }
    break;
  }
  return x;
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
  lval* k = lval_sym(name);
  lval* v = lval_fun(func);
  lenv_put(e, k, v);
  lval_del(k);
  lval_del(v);
}

void lenv_add_builtins(lenv* e) {
  // List functions
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);

  // Mathematical functions
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);

  // Variable function
  lenv_add_builtin(e, "def", builtin_def);
}


lval* lval_eval(lenv* e, lval* v) {
  if (v->type == LVAL_SYM) {
    lval* x = lenv_get(e, v);
    lval_del(v);
    return x;
  }
  // Evaluate S-expression
  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }
  // Other types remain the same
  return v;
}

lval* lval_eval_sexpr(lenv* e, lval* v) {
  // Evaluating children
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(e, v->cell[i]);
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
  if (f->type != LVAL_FUN) {
    lval_del(v);
    lval_del(f);
    return lval_err("First element is not a function!");
  }

  // Call builtin with operator
  lval* result = f->fun(e, v);
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
  mpc_parser_t* Qexpr  = mpc_new("qexpr");
  mpc_parser_t* Expr   = mpc_new("expr");
  mpc_parser_t* Tyson  = mpc_new("tyson");
  
  /* Define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT,
  "                                                     \
    number : /-?[0-9]+/ ;                               \
    symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;         \
    sexpr  : '(' <expr>* ')' ;                          \
    qexpr  : '{' <expr>* '}' ;                          \
    expr   : <number> | <symbol> | <sexpr> | <qexpr> ;  \
    tyson  : /^/ <expr>* /$/ ;                          \
  ",
  Number, Symbol, Sexpr, Qexpr, Expr, Tyson);
  
  puts("Tyson Version 0.0.0.0.5");
  puts("Press Ctrl+c to Exit\n");
  
  lenv* e = lenv_new();
  lenv_add_builtins(e);
  
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
      lval* x = lval_eval(e, lval_read(r.output));
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

  lenv_del(e);
  
  /* Undefine and delete our parsers */
  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Tyson);
  
  return 0;
}

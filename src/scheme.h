#ifndef SCHEME_H_
#define SCHEME_H_


#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <err.h>

#define UNUSED __attribute__((unused))

typedef struct obj *(*scm_proc)(struct obj *, struct obj **);

typedef struct obj {
  enum obj_tag {
    SCHEME_INTEGER, // 0
    SCHEME_TRUE, // 1
    SCHEME_FALSE, // 2
    SCHEME_NIL, // 3
    SCHEME_CHARACTER, // 4
    SCHEME_STRING, // 5
    SCHEME_CONS, // 6
    SCHEME_SYMBOL, // 7
    SCHEME_CLOSURE, // 8
    SCHEME_PROC, // 9
    SCHEME_KNOT // 10
  } tag;

  union {
    int32_t integer_value;
    char char_value;
    char *sym_value;
    struct {
      char *buffer;
      size_t length;
    };
    struct {
      struct obj *car, *cdr;
    };
    struct {
      struct obj *env, *expr;
    };
    scm_proc procedure;
    struct obj *fwd;
  };
} scm_object;

#define CAR(X) (X)->car
#define CDR(X) (X)->cdr
#define CADR(X)  CAR(CDR(X))
#define CAAR(X)  CAR(CAR(X))
#define CDAR(X)  CDR(CAR(X))
#define CDDR(X)  CDR(CDR(X))
#define CADDR(X) CAR(CDDR(X))
#define CAADR(X) CAR(CADR(X))
#define CDDDR(X) CDR(CDDR(X))
#define CADAR(X) CAR(CDAR(X))
#define CADDDR(X) CAR(CDDDR(X))

#define SCM_BOOL(x) ((x) ? scm_t : scm_f)

/* global constants */
scm_object *scm_t, *scm_f, *scm_nil;

/* interpreter data structures */
scm_object *symbol_table, *environment;

/* built-in symbols */
scm_object *quote_sym, *define_sym, *lambda_sym, *if_sym, *expand_sym, *eof_sym, *quasiquote_sym, *unquote_sym;

/* object tag to string */
const char *tag_str(enum obj_tag tag);

/* allocation functions */
scm_object *new(enum obj_tag);
scm_object *new_integer(int);
scm_object *new_char(char);
scm_object *new_string(char *, int);
scm_object *cons(scm_object *car, scm_object *cdr);
scm_object *make_symbol(char *sym);
scm_object *new_closure(scm_object *env, scm_object *expr);

/* interpreter entry points */
scm_object *eval(scm_object *, scm_object **);
scm_object *user_interact(scm_object *, scm_object **);

#endif /* SCHEME_H_ */

// vim: ft=c

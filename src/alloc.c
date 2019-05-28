#include "scheme.h"

scm_object *new(enum obj_tag tag) {
  scm_object *o = malloc(sizeof(scm_object));
  if (!o) {
    err(1, "failed to allocate memory for object of tag %s", tag_str(tag));
  }
  o->tag = tag;
  return o;
}

scm_object *new_integer(int num) {
  scm_object *o = new(SCHEME_INTEGER);
  o->integer_value = num;
  return o;
}

scm_object *new_char(char c) {
  scm_object *o = new(SCHEME_CHARACTER);
  o->char_value = c;
  return o;
}

scm_object *new_string(char *buf, int size) {
  scm_object *o = new(SCHEME_STRING);
  o->buffer = buf;
  o->length = size;
  return o;
}

scm_object *cons(scm_object *car, scm_object *cdr) {
  scm_object *o = new(SCHEME_CONS);
  o->car = car;
  o->cdr = cdr;
  return o;
}

scm_object *make_symbol(char *sym) {
  scm_object *elem = symbol_table;

  while (elem->tag != SCHEME_NIL) {
    assert(elem->tag == SCHEME_CONS);
    assert(CAR(elem)->tag == SCHEME_SYMBOL);
    if (strcmp(CAR(elem)->sym_value, sym) == 0) {
      return CAR(elem);
    }
    elem = CDR(elem);
  }

  scm_object *obj = new(SCHEME_SYMBOL);
  obj->sym_value = strdup(sym);
  symbol_table = cons(obj, symbol_table);

  return obj;
}

scm_object *new_closure(scm_object *env, scm_object *expr) {
  scm_object *o = new(SCHEME_CLOSURE);
  o->env = env;
  o->expr = expr;
  return o;
}



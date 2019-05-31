#include "scheme.h"
#include "reader.h"
#include "lib.h"

const char *tag_str(enum obj_tag tag) {
  switch (tag) {
    case SCHEME_INTEGER: return "integer";
    case SCHEME_TRUE: return "true";
    case SCHEME_FALSE: return "false";
    case SCHEME_NIL: return "nil";
    case SCHEME_CHARACTER: return "char";
    case SCHEME_STRING: return "string";
    case SCHEME_CONS: return "pair";
    case SCHEME_SYMBOL: return "symbol";
    case SCHEME_CLOSURE: return "closure";
    case SCHEME_PROC: return "procedure";
    case SCHEME_KNOT: return "knot";
    default: errx(1, "unknown object tag %d", tag);
  }
}

int is_self_eval(scm_object *obj) {
  int d = obj->tag;
  return d == SCHEME_TRUE ||
    d == SCHEME_FALSE ||
    d == SCHEME_CHARACTER ||
    d == SCHEME_STRING ||
    d == SCHEME_INTEGER ||
    d == SCHEME_CLOSURE ||
    d == SCHEME_PROC ||
    d == SCHEME_NIL;
}

int is_special(scm_object *o, scm_object *tag) {
  return o->tag == SCHEME_CONS && CAR(o) == tag;
}

scm_object *eval(scm_object *obj, scm_object **env) {
tailcall:

  if (is_self_eval(obj)) {
    return obj;
  } else if (is_special(obj, quote_sym)) {
    return CADR(obj);
  } else if (is_special(obj, define_sym)) {
    scm_object *name = CADR(obj), *expr = CADDR(obj);
    if (name->tag == SCHEME_CONS) {
      expr = cons(lambda_sym, cons(CDR(name), CDDR(obj))), name = CAR(name);
    } else if (name->tag != SCHEME_SYMBOL) {
      errx(1, "can't define %s", tag_str(name->tag));
    }
    scm_object *hole = new(SCHEME_KNOT);

    scm_object *new_env =
      cons(cons(name, hole), *env);

    hole->fwd = eval(expr, &new_env);
    *env = cons(cons(name, hole->fwd), *env);

    return hole->fwd;
  } else if (is_special(obj, lambda_sym)) {
    switch (CADR(obj)->tag) {
      case SCHEME_CONS:
      case SCHEME_NIL:
      case SCHEME_SYMBOL:
        break;
      default:
        errx(1, "parameter of lambda must be a list or symbol, got %s", tag_str(CADR(obj)->tag));
    }

    return new_closure(*env, obj);
  } else if (is_special(obj, if_sym)) {
    scm_object *cond = CADR(obj), *if_body = CADDR(obj);
    scm_object *else_body = scm_nil;

    if (CDDDR(obj)->tag == SCHEME_CONS) {
      else_body = CADDDR(obj);
    }

    switch (eval(cond, env)->tag) {
      case SCHEME_FALSE:
        obj = else_body;
        goto tailcall;
      default:
        obj = if_body;
        goto tailcall;
    }
  } else if (obj->tag == SCHEME_SYMBOL) {
    scm_object *elem = *env;

    while (elem->tag != SCHEME_NIL) {
      assert(elem->tag == SCHEME_CONS);
      assert(CAR(elem)->tag == SCHEME_CONS);
      assert(CAAR(elem)->tag == SCHEME_SYMBOL);

      if (CAAR(elem) == obj) {
        return CDAR(elem);
      }
      elem = CDR(elem);
    }

    errx(1, "no binding for symbol %s", obj->sym_value);
  } else if (obj->tag == SCHEME_CONS) {
    scm_object *fun = eval(CAR(obj), env);

    switch (fun->tag) {
      case SCHEME_CLOSURE: ;
        scm_object *closure_env = fun->env,
                   *closure_body = CDDR(fun->expr),
                   *closure_args = CADR(fun->expr),
                   *params_zipped;

        switch (closure_args->tag) {
          case SCHEME_NIL:
          case SCHEME_CONS:
            params_zipped = zip_eval(closure_args, CDR(obj), env);
            break;
          case SCHEME_SYMBOL:
            params_zipped = cons(cons(closure_args, map_eval(CDR(obj), env)), scm_nil);
            break;
          default:
            errx(1, "unsupported object %s as arguments of closure", tag_str(closure_args->tag));
        }

        scm_object *new_env = append(params_zipped, closure_env);
        env = &new_env;

        while (CDR(closure_body)->tag != SCHEME_NIL) {
          eval(CAR(closure_body), env);
          closure_body = CDR(closure_body);
        }

        obj = CAR(closure_body);
        goto tailcall;

      case SCHEME_PROC: ;
        scm_object *args = scm_nil;
        obj = CDR(obj);
        while (obj->tag != SCHEME_NIL) {
          args = cons(eval(CAR(obj), env), args);
          obj = CDR(obj);
        }
        return fun->procedure(append(args, scm_nil), env);

      case SCHEME_KNOT:
        obj = cons(fun->fwd, CDR(obj));
        goto tailcall;

      default: errx(1, "can't apply obj of type %s", tag_str(fun->tag));
    }
  } else if (obj->tag == SCHEME_KNOT) {
    obj = obj->fwd;
    goto tailcall;
  } else {
    errx(1, "can't eval obj with type %d", obj->tag);
  }
}

scm_object *user_interact(scm_object *obj, scm_object **env) {
  return eval(eval(cons(expand_sym, cons(cons(quote_sym, cons(obj, scm_nil)), scm_nil)), env), env);
}

int main(int argc, char *argv[]) {
  scm_init();
  int linum = 0, colnum = 0;

  scheme_input = stdin;

  for (int i = 1; i < argc; i++) {
    pscm_load(cons(new_string(argv[i], strlen(argv[i])), scm_nil), &environment);
  }

  for (;; linum++) {
    printf("> ");
    if (peek() == EOF) {
      exit(0);
    }
    scm_write(user_interact(scm_read(&linum, &colnum), &environment));
    putchar('\n');
  }
  exit(0);
}

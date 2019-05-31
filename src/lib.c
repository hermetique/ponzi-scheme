#include "reader.h"
#include "lib.h"

#include <unistd.h>
#include <fcntl.h>

#define P(TYPE, DISCRIMINANT) \
  static scm_object *pscm_is_ ## TYPE (scm_object *a, UNUSED scm_object **env) { \
    assert(scm_len(a) == 1); \
    return SCM_BOOL(CAR(a)->tag == SCHEME_ ## DISCRIMINANT); \
  }

#define O(NAME, OP) \
  static scm_object *pscm_op_ ## NAME (scm_object *a, UNUSED scm_object **env) { \
    assert(scm_len(a) == 2); \
    assert(CAR(a)->tag == SCHEME_INTEGER); \
    assert(CADR(a)->tag == SCHEME_INTEGER); \
    return new_integer(CAR(a)->integer_value OP CADR(a)->integer_value); \
  }

#define C(NAME, OP) \
  static scm_object *pscm_cmp_ ## NAME (scm_object *a, UNUSED scm_object **env) { \
    assert(scm_len(a) == 2); \
    if (CAR(a)->tag == SCHEME_INTEGER && CADR(a)->tag == SCHEME_INTEGER) { \
      return SCM_BOOL(CAR(a)->integer_value OP CADR(a)->integer_value); \
    } else if (CAR(a)->tag == SCHEME_CHARACTER && CADR(a)->tag == SCHEME_CHARACTER) { \
      return SCM_BOOL(CAR(a)->char_value OP CADR(a)->char_value); \
    } else { \
      errx(1, "invalid comparison between types %s and %s", tag_str(CAR(a)->tag), tag_str(CADR(a)->tag)); \
    } \
  }

P(cons, CONS)
P(null, NIL)
P(string, STRING)
P(function, CLOSURE)
P(procedure, PROC)
P(char, CHARACTER)
P(integer, INTEGER)
P(symbol, SYMBOL)

O(plus, +)
O(minus, -)
O(times, *)
O(over, /)
O(rem, %)

C(lt, <)
C(gt, >)
C(lte, <=)
C(gte, >=)

#undef P
#undef O
#undef C

int scm_len(scm_object *n) {
  assert(n->tag == SCHEME_CONS || n->tag == SCHEME_NIL);
  int len = 0;
  while (n->tag == SCHEME_CONS) {
    n = CDR(n);
    len++;
  }
  return len;
}

scm_object *add_procedure(const char *name, scm_proc procedure) {
  scm_object *sym = make_symbol((char *) name);
  scm_object *proc = new(SCHEME_PROC);
  proc->procedure = procedure;
  environment = cons(cons(sym, proc), environment);
  return proc;
}

scm_object *pscm_cons(scm_object *args, UNUSED scm_object **env) {
  assert(scm_len(args) == 2);

  return cons(CAR(args), CADR(args));
}

scm_object *pscm_is_bool(scm_object *args, UNUSED scm_object **env) {
  assert(scm_len(args) == 1);

  return SCM_BOOL(CAR(args)->tag == SCHEME_TRUE || CAR(args)->tag == SCHEME_FALSE); 
}

scm_object *pscm_car(scm_object *args, UNUSED scm_object **env) {
  assert(scm_len(args) == 1);
  if (CAR(args)->tag != SCHEME_CONS) {
    scm_write(CAR(args));
    errx(1, "bad argument to car: object %s", tag_str(CAR(args)->tag));
  }

  return CAAR(args);
}

scm_object *pscm_cdr(scm_object *args, UNUSED scm_object **env) {
  assert(scm_len(args) == 1);
  assert(CAR(args)->tag == SCHEME_CONS);
  if (CAR(args)->tag != SCHEME_CONS) {
    scm_write(CAR(args));
    errx(1, "bad argument to cdr: object %s", tag_str(CAR(args)->tag));
  }

  return CDAR(args);
}

scm_object *pscm_setcar(scm_object *args, UNUSED scm_object **env) {
  assert(scm_len(args) == 2);

  scm_object *addr = CAR(args);
  scm_object *val = CADR(args);
  if (CAR(args)->tag != SCHEME_CONS) {
    return scm_f;
  }
  CAR(addr) = val;

  return scm_t;
}

scm_object *pscm_setcdr(scm_object *args, UNUSED scm_object **env) {
  assert(scm_len(args) == 2);

  scm_object *addr = CAR(args);
  scm_object *val = CADR(args);
  if (addr->tag != SCHEME_CONS) {
    return scm_f;
  }
  CDR(addr) = val;

  return scm_t;
}

scm_object *pscm_list(scm_object *args, UNUSED scm_object **env) {
  return args;
}

scm_object *pscm_id(scm_object *args, UNUSED scm_object **env) {
  return CAR(args);
}

scm_object *pscm_load(scm_object *args, scm_object **env) {
  assert(scm_len(args) == 1);

  scm_object *path = CAR(args);
  assert(path->tag == SCHEME_STRING);
  int linum = 0, colnum = 0;

  if ((scheme_input = fopen(path->buffer, "r")) != NULL) {
    for(;;) {
      if (peek() == EOF) {
        break;
      }
      user_interact(scm_read(&linum, &colnum), env);
    }
    scheme_input = stdin;
    return scm_t;
  } else {
    err(1, "failed to open file %s for reading", path->buffer);
  }
}

scm_object *pscm_write(scm_object *args, UNUSED scm_object **env) {
  int n = 0;
  while (args->tag != SCHEME_NIL) {
    switch (CAR(args)->tag) {
      case SCHEME_STRING:
        printf("%s", CAR(args)->buffer);
        break;
      case SCHEME_CHARACTER:
        printf("%c", CAR(args)->char_value);
        break;
      default: scm_write(CAR(args));
    }
    args = CDR(args);
    n++;
  }
  return new_integer(n);
}

scm_object *pscm_error(scm_object *args, UNUSED scm_object **env) {
  assert(scm_len(args) == 1);
  assert(CAR(args)->tag == SCHEME_STRING);

  CAR(args)->buffer[CAR(args)->length] = 0;
  errx(1, "%s", CAR(args)->buffer);
}

scm_object *pscm_equal(scm_object *args, UNUSED scm_object **env) {
  assert(scm_len(args) == 2);

  scm_object *a = CAR(args), *b = CADR(args);

  if (a->tag != b->tag) return scm_f;
  switch (a->tag) {
    case SCHEME_INTEGER:
      return SCM_BOOL(a->integer_value == b->integer_value);
    case SCHEME_TRUE: return scm_t;
    case SCHEME_FALSE: return scm_t;
    case SCHEME_NIL: return scm_t;
    case SCHEME_CHARACTER:
      return SCM_BOOL(a->char_value == b->char_value);
    case SCHEME_STRING:
      if (a->length != b->length)
        return scm_f;
      return SCM_BOOL(!strncmp(a->buffer, b->buffer, a->length));
    case SCHEME_CONS:
      if (pscm_equal(cons(CAR(a), CAR(b)), env) != scm_t)
        return scm_f;
      return pscm_equal(cons(CDR(a), CDR(b)), env);
    default: return SCM_BOOL(a == b);
  }
}

scm_object *pscm_string_ref(scm_object *args, UNUSED scm_object **env) {
  assert(scm_len(args) == 2);
  assert(CAR(args)->tag == SCHEME_STRING);
  assert(CADR(args)->tag == SCHEME_INTEGER);

  scm_object *str = CAR(args);
  size_t idx = CADR(args)->integer_value;

  if (idx > str->length) {
    errx(1, "string-ref: index %zu out of bounds", idx);
  }
  return new_char(str->buffer[idx]);
}

scm_object *pscm_string_set(scm_object *args, UNUSED scm_object **env) {
  assert(scm_len(args) == 3);
  assert(CAR(args)->tag == SCHEME_STRING);
  assert(CADR(args)->tag == SCHEME_INTEGER);
  assert(CADDR(args)->tag == SCHEME_CHARACTER);

  scm_object *str = CAR(args), *chr = CADDR(args);
  size_t idx = CADR(args)->integer_value;

  if (idx > str->length) {
    errx(1, "string-set!: index %zu out of bounds", idx);
  }

  str->buffer[idx] = chr->char_value;
  return scm_t;
}

scm_object *pscm_string_len(scm_object *args, UNUSED scm_object **env) {
  assert(scm_len(args) == 1);
  assert(CAR(args)->tag == SCHEME_STRING);

  return new_integer(CAR(args)->length);
}

scm_object *pscm_env(scm_object *args, UNUSED scm_object **env) {
  assert(scm_len(args) == 0);
  return *env;
}

scm_object *pscm_eval(scm_object *args, UNUSED scm_object **env) {
  assert(scm_len(args) == 2);

  return eval(CAR(args), &CADR(args));
}

scm_object *pscm_gensym(scm_object *args, UNUSED scm_object **env) {
  assert(scm_len(args) == 0);
  static int gensym_counter = 0;

  char buf[128];
  if (!sprintf(buf, "#%d", gensym_counter++))
    err(1, "failed to print symbol %d", gensym_counter);

  return make_symbol(buf);
}

scm_object *pscm_read_char(scm_object *args, UNUSED scm_object **env) {
  assert(scm_len(args) == 0 || scm_len(args) == 1);

  if (scm_len(args) == 1 && CAR(args)->tag == SCHEME_INTEGER) {
    char buf;
    switch (read(CAR(args)->integer_value, &buf, 1)) {
      case 0:
        return scm_nil;
      case 1:
        return new_char(buf);
      default: return scm_f;
    }
  } else {
    int ch = getc(scheme_input);
    if (ch == EOF) {
      return scm_nil;
    } else {
      return new_char((char) ch);
    }
  }
}

scm_object *pscm_write_char(scm_object *args, UNUSED scm_object **env) {
  assert(scm_len(args) == 1 || scm_len(args) == 2);
  assert(CAR(args)->tag == SCHEME_CHARACTER);

  if (scm_len(args) == 2 && CADR(args)->tag == SCHEME_INTEGER) {
    char buf = CAR(args)->char_value;
    if (write(CADR(args)->integer_value, &buf, 1) != 1) {
      return scm_f;
    }
    return scm_t;
  } else {
    if (!putc((int) CAR(args)->char_value, stdout)) {
      return scm_f;
    }
    return scm_t;
  }
}

scm_object *pscm_open(scm_object *args, UNUSED scm_object **env) {
  assert(scm_len(args) == 2 && CAR(args)->tag == SCHEME_STRING && CADR(args)->tag == SCHEME_CHARACTER);
  int fd;
  switch (CADR(args)->char_value) {
    case 'r':
      if ((fd = open(CAR(args)->buffer, O_RDONLY)) == -1) {
        return scm_f;
      }
      return new_integer(fd);
    case 'w':
      if ((fd = open(CAR(args)->buffer, O_WRONLY | O_CREAT, 0644)) == -1) {
        return scm_f;
      }
      return new_integer(fd);
    case '+':
      if ((fd = open(CAR(args)->buffer, O_RDWR | O_CREAT, 0644)) == -1) {
        return scm_f;
      }
      return new_integer(fd);
    default:
      return scm_f;
  }
}

scm_object *pscm_close(scm_object *args, UNUSED scm_object **env) {
  assert(scm_len(args) == 1);
  close(CAR(args)->integer_value);
  return scm_t;
}

void scm_init() {
  scm_t = new(SCHEME_TRUE);
  scm_f = new(SCHEME_FALSE);
  scm_nil = new(SCHEME_NIL);
  symbol_table = new(SCHEME_NIL);
  environment = new(SCHEME_NIL);

  quote_sym = make_symbol("quote");
  define_sym = make_symbol("define");
  lambda_sym = make_symbol("lambda");
  if_sym = make_symbol("if");
  expand_sym = make_symbol("expand");
  eof_sym = make_symbol("EOF");
  quasiquote_sym = make_symbol("quasiquote");
  unquote_sym = make_symbol("unquote");

  add_procedure("cons", pscm_cons);
  add_procedure("car", pscm_car);
  add_procedure("cdr", pscm_cdr);
  add_procedure("set-car!", pscm_setcar);
  add_procedure("set-cdr!", pscm_setcdr);
  add_procedure("list", pscm_list);

  add_procedure("eq?", pscm_equal);

  add_procedure("pair?", pscm_is_cons);
  add_procedure("null?", pscm_is_null);
  add_procedure("bool?", pscm_is_bool);
  add_procedure("string?", pscm_is_string);
  add_procedure("char?", pscm_is_char);
  add_procedure("procedure?", pscm_is_procedure);
  add_procedure("function?", pscm_is_function);
  add_procedure("integer?", pscm_is_integer);
  add_procedure("symbol?", pscm_is_symbol);

  add_procedure("+", pscm_op_plus);
  add_procedure("-", pscm_op_minus);
  add_procedure("*", pscm_op_times);
  add_procedure("/", pscm_op_over);
  add_procedure("%", pscm_op_rem);

  add_procedure("<", pscm_cmp_lt);
  add_procedure(">", pscm_cmp_gt);
  add_procedure("<=", pscm_cmp_lte);
  add_procedure(">=", pscm_cmp_gte);

  add_procedure("string-ref", pscm_string_ref);
  add_procedure("string-len", pscm_string_len);
  add_procedure("string-set!", pscm_string_set);

  add_procedure("open-file", pscm_open);
  add_procedure("close-file", pscm_close);
  add_procedure("read-char", pscm_read_char);
  add_procedure("write-char", pscm_write_char);

  add_procedure("gensym", pscm_gensym);
  add_procedure("expand", pscm_id);
  add_procedure("load", pscm_load);
  add_procedure("write", pscm_write);
  add_procedure("eval", pscm_eval);
  add_procedure("environment", pscm_env);
  add_procedure("error", pscm_error);
}

scm_object *zip_eval(scm_object *names, scm_object *args, scm_object **env) {
  scm_object *result = scm_nil;
  while (names->tag != SCHEME_NIL && args->tag != SCHEME_NIL) {
    scm_object *arg = eval(CAR(args), env);
    result = cons(cons(CAR(names), arg), result);
    names = CDR(names), args = CDR(args);
  }
  return result;
}

scm_object *append(scm_object *a, scm_object *b) {
  scm_object *result = b;
  while (a->tag != SCHEME_NIL) {
    result = cons(CAR(a), result);
    a = CDR(a);
  }
  return result;
}

// implementation due to nortti (@JuEeHa) and vi
// <vi@forbidden.technology>
scm_object *map_eval(scm_object *args, scm_object **env) {
  scm_object *head = scm_nil, **tail_ptr = &head;

  while (args->tag != SCHEME_NIL) {
    scm_object *current = cons(eval(CAR(args), env), scm_nil);
    *tail_ptr = current;
    tail_ptr = &CDR(current);
    args = CDR(args);
  }

  return head;
}

int scm_write(scm_object *obj) {
  switch (obj->tag) {
    case SCHEME_INTEGER:
      printf("%d", obj->integer_value);
      break;
    case SCHEME_TRUE:
      printf("#t");
      break;
    case SCHEME_FALSE:
      printf("#f");
      break;
    case SCHEME_NIL:
      printf("()");
      break;
    case SCHEME_CHARACTER:
      printf("#\\");
      switch (obj->char_value) {
        case '\n':
          printf("newline"); break;
        case '\t':
          printf("tab"); break;
        case ' ':
          printf("space"); break;
        default:
          printf("%c", obj->char_value); break;
      }
      break;
    case SCHEME_STRING:
      putchar('"');
      char c;
      for (size_t i = 0; i < obj->length; i++) {
        switch (c = obj->buffer[i]) {
          case '\n':
            printf("\\n");
            break;
          case '\t':
            printf("\\t");
            break;
          case '"':
            printf("\\\"");
            break;
          case '\\':
            printf("\\\\");
            break;
          default:
            putchar(c);
        }
      }
      putchar('"');
      break;
    case SCHEME_CONS:
      putchar('(');
      scm_object *car = CAR(obj), *cdr = CDR(obj);

print_pair:
      scm_write(car);
      if (cdr->tag == SCHEME_CONS) {
        putchar(' ');
        obj = cdr;
        car = CAR(obj);
        cdr = CDR(obj);
        goto print_pair;
      } else if (cdr->tag == SCHEME_NIL) {
      } else {
        printf(" . ");
        scm_write(cdr);
      }
      goto end;

end:
        putchar(')');
        break;
    case SCHEME_SYMBOL:
      printf("%s", obj->sym_value);
      break;
    case SCHEME_CLOSURE:
      printf("#<closure %#.zx>", (size_t) obj);
      break;
    case SCHEME_PROC:
      printf("#<procedure %#.zx>", (size_t) obj->procedure);
      break;
    case SCHEME_KNOT:
      printf("#<knot: ");
      scm_write(obj->fwd);
      putchar('>');
      break;
  }
  fflush(stdout);
  return 0;
}

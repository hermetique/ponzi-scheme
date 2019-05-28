#include "reader.h"

int is_delim(char ch) {
  return isspace(ch) || (ch == '(') || (ch == ')') || (ch == '\n') || (ch == ';') || ch == EOF || ch == '"';
}

int is_initial(int c) {
    return isalpha(c) || c == '*' || c == '/' || c == '>' || c == '<' || c == '=' || c == '?' || c == '!';
}

int peek() {
  int ch = getc(scheme_input);
  if (ch == EOF) {
    return EOF;
  }
  ungetc(ch, scheme_input);

  return ch;
}

int getch(int *linum, int *colnum) {
  int c = getc(scheme_input);
  if (c == '\n') {
    *colnum = 0;
    (*linum)++;
  } else { (*colnum)++; }
  return c;
}

void skip_spaces(int *linum, int *colnum) {
  int c;
  while ((c = getch(linum, colnum)) != EOF) {
    if (isspace(c)) {
      continue;
    } else if (c == ';') {
      while ((c = getch(linum, colnum)) != EOF && c != '\n') {}
      continue;
    }
    ungetc(c, scheme_input);
    return;
  }
}

void expect_delim(int linum, int colnum, const char *what) {
  if (!is_delim(peek())) {
    errx(1, "%s not followed by delimiter at %d:%d", what, linum, colnum);
  }
}

void eat_string(int *linum, int *colnum, const char *str) {
  int c;
  while (*str != '\0') {
    if ((c = getch(linum, colnum)) != *str) {
      errx(1, "unexpected '%c' at %d:%d, expecting '%c'", c, *linum, *colnum, *str);
    }
    str++;
  }
}

scm_object *read_scm_char(int *linum, int *colnum) {
  int c = getch(linum, colnum);
  switch (c) {
    case EOF:
      errx(1, "incomplete character literal at %d:%d", *linum, *colnum);
    case 's':
      if (peek() == 'p') {
        eat_string(linum, colnum, "pace");
        expect_delim(*linum, *colnum, "character");
        return new_char(' ');
      }
      break;
    case 'n':
      if (peek() == 'e') {
        eat_string(linum, colnum, "ewline");
        expect_delim(*linum, *colnum, "character");
        return new_char('\n');
      }
      break;
    case 't':
      if (peek() == 'a') {
        eat_string(linum, colnum, "ab");
        expect_delim(*linum, *colnum, "character");
        return new_char('\t');
      }
      break;
  }
  expect_delim(*linum, *colnum, "character");
  return new_char(c);
}

scm_object *read(int *linum, int *colnum);

scm_object *read_scm_string(int *linum, int *colnum) {
  size_t size = 0, cap = 256;
  int ch;
  char *buffer = malloc(cap);
  char value;

  while ((ch = getch(linum, colnum)) != '"') {
    switch(ch) {
      case '\\':
        switch (ch = getch(linum, colnum)) {
          case 'n':
            value = '\n';
            break;
          case 't':
            value = '\t';
            break;
          case '"':
            value = '"';
            break;
          case '\\':
            value = '\\';
            break;
          case EOF:
            errx(1, "EOF while reading string at %d:%d", *linum, *colnum);
          default:
            errx(1, "unrecognised escape character '%c' at %d:%d", ch, *linum, *colnum);
        }
        break;
      case EOF:
        errx(1, "unterminated string at %d:%d", *linum, *colnum);
      default:
        value = ch;
        break;
    }

    if (size + 1 > cap) {
      buffer = realloc(buffer, cap + 64);
    }
    buffer[size++] = value;
  }

  expect_delim(*linum, *colnum, "string literal");
  return new_string(buffer, size);
}

scm_object *read_scm_list(int *linum, int *colnum) {
  scm_object *car = read(linum, colnum);
  skip_spaces(linum, colnum);

  int ch;
  switch (ch = peek()) {
    case '.':
      getch(linum, colnum);
      scm_object *cdr = read(linum, colnum);
      skip_spaces(linum, colnum);
      if (getch(linum, colnum) != ')') {
        errx(1, "expected closing ')' at %d:%d", *linum, *colnum);
      }
      return cons(car, cdr);
    case ')':
      getch(linum, colnum);
      return cons(car, scm_nil);
    default:
      cdr = read_scm_list(linum, colnum);
      return cons(car, cdr);
  }
}

scm_object *read_scm_integer(char c, int *linum, int *colnum) {
  short sign = 1;
  int num = 0;

  if (c == '-') { sign = -1; } else { ungetc(c, scheme_input); }
  while (isdigit(c = getch(linum, colnum))) {
    num = (num * 10) + (c - '0');
  }
  num *= sign;
  if (is_delim(c)) {
    ungetc(c, scheme_input);
    return new_integer(num);
  } else {
    errx(1, "expecting delimiter at %d:%d, got '%c'", *linum, *colnum, c);
  }
}

scm_object *read_scm_symbol(char c, int *linum, int *colnum) {
  size_t i = 0, cap = 256;
  char *buf = malloc(cap);
  while (is_initial(c) || isdigit(c) || c == '+' || c == '-') {
    if (i + 1 < cap) {
      buf[i++] = c;
    } else {
      buf[i] = '\0';
      errx(1, "symbol '%s' too long at %d:%d", buf, *linum, *colnum);
    }
    c = getch(linum, colnum);
  }
  if (is_delim(c)) {
    buf[i] = '\0';
    ungetc(c, scheme_input);
    scm_object *x = make_symbol(buf);
    free(buf);
    return x;
  } else {
    errx(1, "expecting delimiter after symbol at %d:%d", *linum, *colnum);
  }
}

scm_object *read(int *linum, int *colnum) {
  int c;

  skip_spaces(linum, colnum);

  c = getch(linum, colnum);
  if (isdigit(c) || (c == '-' && isdigit(peek()))) {
    return read_scm_integer(c, linum, colnum);
  } else if (c == '#') {
    switch(getch(linum, colnum)) {
      case 't':
        return scm_t;
      case 'f':
        return scm_f;
      case '\\':
        return read_scm_char(linum, colnum);
      default:
        errx(1, "expecting boolean at %d:%d (#t/#f), got '%c'", *linum, *colnum, c);
    }
  } else if (c == '"') {
    return read_scm_string(linum, colnum);
  } else if (c == '(') {
    switch(peek()) {
      case ')':
        getch(linum, colnum);
        return scm_nil;
      default:
        return read_scm_list(linum, colnum);
    }
  } else if (is_initial(c) || ((c == '+' || c == '-') && is_delim(peek()))) {
    return read_scm_symbol(c, linum, colnum);
  } else if (c == '\'') {
    return cons(quote_sym, cons(read(linum, colnum), scm_nil));
  } else if (c == '`') {
    return cons(quasiquote_sym, cons(read(linum, colnum), scm_nil));
  } else if (c == ',') {
    return cons(unquote_sym, cons(read(linum, colnum), scm_nil));
  } else if (c == EOF) {
    return cons(quote_sym, cons(eof_sym, scm_nil));
  } else {
    errx(1, "unexpected '%c' at %d:%d\n", c, *linum, *colnum);
  }
}


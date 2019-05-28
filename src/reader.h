#ifndef READER_H_
#define READER_H_

#include "scheme.h"

FILE *scheme_input;

int peek();
scm_object *read(int *, int *);

#endif /* READER_H_ */

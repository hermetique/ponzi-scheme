#ifndef READER_H_
#define READER_H_

#include "scheme.h"

FILE *scheme_input;

int peek();
scm_object *scm_read(int *, int *);

#endif /* READER_H_ */

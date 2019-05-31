#ifndef SCHEME_LIB_H_
#define SCHEME_LIB_H_

#include "scheme.h"

scm_object *zip_eval(scm_object *, scm_object *, scm_object **);
scm_object *append(scm_object *, scm_object *);
scm_object *map_eval(scm_object *, scm_object **);

int scm_write(scm_object *);
int scm_len(scm_object *);

scm_object *add_procedure(const char *, scm_proc);


void scm_init();
scm_object *pscm_load(scm_object *, scm_object **);

#endif /* SCHEME_LIB_H_ */

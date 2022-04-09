#ifndef HZBOOL_H
#define HZBOOL_H

/* ensure we have a 'bool' type, and 'true'/'false' keywords */
#ifdef __cplusplus
  /* C++ has these built in, so do nothing */
#elif __STDC_VERSION__ >= 199901L
  /* C99 defines these in a header, so use that */
  #include <stdbool.h>
#elif !defined(__bool_true_false_are_defined)
  /* use insigned int for bool
     this is a tradeoff to support use as a bit field struct member */
  typedef unsigned int bool;
  /* use an enum for true/false for now */
  enum { false = 0, true = 1 };
  #define __bool_true_false_are_defined 1
#endif

#endif

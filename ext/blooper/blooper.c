#include 
#include "ruby.h"
  
void Init_test(void);
VALUE ju_match_count();

void Init_test() {
  VALUE cJottoUtil = rb_define_class("Test", rb_cObject);
  rb_define_method(cJottoUtil, "match_count", ju_match_count, 2);
}

VALUE ju_match_count(VALUE jh, VALUE word1, VALUE word2) {
  char *w1 = RSTRING_PTR(word1);
  char *w2 = RSTRING_PTR(word2);
  int i;
  int n = 0;

  for(i = 0; i < RSTRING_LEN(word1); i++) {
    if(w1[i] == w2[i]) {
      n++;
    }
  }
  return INT2NUM(n);
}
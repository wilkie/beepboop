#include "ruby.h"
#include "ymf262.h"
#include "fmopl.h"

void Init_beepboop(void);

void Init_beepboop() {
  VALUE cJottoUtil = rb_define_class("Beepboop", rb_cObject);
}

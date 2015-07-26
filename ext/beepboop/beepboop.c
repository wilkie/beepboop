#include "ruby.h"
#include "ymf262.h"
#include "fmopl.h"
#include <stdio.h>

#define OPL3_INTERNAL_FREQ 14318180

void Init_beepboop(void);
VALUE rb_beepboop_opl_init(VALUE self, VALUE sample_rate);
VALUE rb_beepboop_opl_reset(VALUE self);
VALUE rb_beepboop_opl_uninit(VALUE self);
VALUE rb_beepboop_opl_write(VALUE self, VALUE reg, VALUE data);
VALUE rb_beepboop_opl_sample(VALUE self, VALUE stream, VALUE numSamples);

void Init_beepboop() {
  /* Create Beepboop::OPL */
  VALUE cBeepboop = rb_define_module("Beepboop");
  VALUE cOPL      = rb_define_class_under(cBeepboop, "OPL", rb_cObject);

  rb_define_method(cOPL, "initialize", &rb_beepboop_opl_init, 1);
  rb_define_method(cOPL, "reset", &rb_beepboop_opl_reset, 0);
  rb_define_method(cOPL, "uninit", &rb_beepboop_opl_uninit, 0);
  rb_define_method(cOPL, "write", &rb_beepboop_opl_write, 2);
  rb_define_method(cOPL, "sample", &rb_beepboop_opl_sample, 2);
}

/* Pass throughs for the various operations we require */

/* def OPL#init(sample_rate)
 *
 * Initialize an OPL chip.
 */
VALUE rb_beepboop_opl_init(VALUE self, VALUE sample_rate) {
  int cSampleRate = FIX2INT(sample_rate);
  int ymf262_device_id = YMF262Init(1, OPL3_INTERNAL_FREQ, cSampleRate);

  /* Place device id into the class instance */
  rb_iv_set(self, "@ymf_device_id", INT2FIX(ymf262_device_id));

  rb_beepboop_opl_reset(self);

  return 0;
}

VALUE rb_beepboop_opl_reset(VALUE self) {
  int ymf262_device_id = FIX2INT(rb_iv_get(self, "@ymf_device_id"));

  YMF262ResetChip(ymf262_device_id);

  return 0;
}

/* def OPL#write(register, data)
 *
 * Write a register out to the chip.
 */
VALUE rb_beepboop_opl_write(VALUE self, VALUE reg, VALUE data) {
  int cReg  = FIX2INT(reg);
  int cData = FIX2INT(data);

  int ymf262_device_id = FIX2INT(rb_iv_get(self, "@ymf_device_id"));

  int op = 0;
  if (cReg > 255) {
    op = 2;
    cReg &= 0xff;
  }

  YMF262Write(ymf262_device_id, op,   cReg);
  YMF262Write(ymf262_device_id, op+1, cData);

  return 0;
}

/* def OPL#uninit()
 *
 * Uninitialize an OPL chip.
 */
VALUE rb_beepboop_opl_uninit(VALUE self) {
  //int ymf262_device_id = FIX2INT(rb_iv_get(self, "@ymf_device_id"));

  YMF262Shutdown();

  rb_iv_set(self, "@ymf_device_id", INT2FIX(0));

  return 0;
}

/* def OPL#sample()
 *
 * Samples from the OPL synth the given number of samples.
 */
VALUE rb_beepboop_opl_sample(VALUE self, VALUE stream, VALUE numSamples) {
  int ymf262_device_id = FIX2INT(rb_iv_get(self, "@ymf_device_id"));

  /* if stream is an FFI::Pointer, then pull out the address and use that */
  VALUE cFFI        = rb_define_module("FFI");
  int hasFFIPointer = rb_const_defined(cFFI, rb_intern("Pointer")) == Qtrue;
  int isFFIPointer  = 0;

  short* stream_ptr = NULL;

  if (hasFFIPointer) {
    VALUE cFFIPointer = rb_const_get(cFFI, rb_intern("Pointer"));
    isFFIPointer  = rb_obj_is_instance_of(stream, cFFIPointer) == Qtrue;
  }

  if (isFFIPointer) {
    /* stream is an FFI::Pointer, so get the underlying memory address */
    ID iAddress = rb_intern("address");

    /* call stream#address */
    stream_ptr = (short*)(FIX2INT(rb_funcall(stream, iAddress, 0)));
  }

  if (stream_ptr) {
    YMF262UpdateOne(ymf262_device_id, stream_ptr, NULL, FIX2INT(numSamples));
  }

  return 0;
}

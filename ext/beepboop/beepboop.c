#include "ruby.h"
#include "ymf262.h"
#include "fmopl.h"

void Init_beepboop(void);

void Init_beepboop() {
  VALUE cJottoUtil = rb_define_class("Beepboop", rb_cObject);
}

/* Pass throughs for the various operations we require */

/* def OPL#out(register, data)
 *
 * Write a register out to the chip.
 */

/* def OPL#init(sample_rate)
 *
 * Initialize an OPL chip.
 */

/* def OPL#uninit()
 *
 * Uninitialize an OPL chip.
 */

/* Check DT_AUDIT and LA_SYMB_NOW.
   Copyright (C) 2021 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <support/capture_subprocess.h>
#include <support/check.h>
#include <support/xstdio.h>
#include <support/support.h>
#include <sys/auxv.h>

static int restart;
#define CMDLINE_OPTIONS \
  { "restart", no_argument, &restart, 1 },

void tst_audit25mod1_func1 (void);
void tst_audit25mod1_func2 (void);
void tst_audit25mod2_func1 (void);
void tst_audit25mod2_func2 (void);

static int
handle_restart (void)
{
  tst_audit25mod1_func1 ();
  tst_audit25mod1_func2 ();
  tst_audit25mod2_func1 ();
  tst_audit25mod2_func2 ();

  return 0;
}

static inline bool
startswith (const char *str, const char *pre)
{
  size_t lenpre = strlen (pre);
  size_t lenstr = strlen (str);
  return lenstr < lenpre ? false : memcmp (pre, str, lenpre) == 0;
}

static int
do_test (int argc, char *argv[])
{
  /* We must have either:
     - One our fource parameters left if called initially:
       + path to ld.so         optional
       + "--library-path"      optional
       + the library path      optional
       + the application name  */

  if (restart)
    return handle_restart ();

  setenv ("LD_AUDIT", "tst-auditmod25.so", 0);

  char *spargv[9];
  int i = 0;
  for (; i < argc - 1; i++)
    spargv[i] = argv[i + 1];
  spargv[i++] = (char *) "--direct";
  spargv[i++] = (char *) "--restart";
  spargv[i] = NULL;

  {
    struct support_capture_subprocess result
      = support_capture_subprogram (spargv[0], spargv);
    support_capture_subprocess_check (&result, "tst-audit25a", 0,
				      sc_allow_stderr);

    /* tst-audit25a and tst-audit25mod1 are built with -Wl,-z,now, but
       tst-audit25mod2 is built with -Wl,z,lazy.  So only
       tst_audit25mod4_func1 (called by tst_audit25mod2_func1) should not
       have LA_SYMB_BINDNOW.  */
    TEST_COMPARE_STRING (result.err.buffer,
			 "la_symbind: tst_audit25mod3_func1 32\n"
			 "la_symbind: tst_audit25mod1_func1 32\n"
			 "la_symbind: tst_audit25mod2_func1 32\n"
			 "la_symbind: tst_audit25mod1_func2 32\n"
			 "la_symbind: tst_audit25mod2_func2 32\n"
			 "la_symbind: tst_audit25mod4_func1 0\n");

    support_capture_subprocess_free (&result);
  }

  {
    setenv ("LD_BIND_NOW", "1", 0);
    struct support_capture_subprocess result
      = support_capture_subprogram (spargv[0], spargv);
    support_capture_subprocess_check (&result, "tst-audit25a", 0,
				      sc_allow_stderr);

    /* With LD_BIND_NOW all symbols are expected to have LA_SYMB_BINDNOW.
       Also the resolution order is done in breadth-first order.  */
    TEST_COMPARE_STRING (result.err.buffer,
			 "la_symbind: tst_audit25mod4_func1 32\n"
			 "la_symbind: tst_audit25mod3_func1 32\n"
			 "la_symbind: tst_audit25mod1_func1 32\n"
			 "la_symbind: tst_audit25mod2_func1 32\n"
			 "la_symbind: tst_audit25mod1_func2 32\n"
			 "la_symbind: tst_audit25mod2_func2 32\n");

    support_capture_subprocess_free (&result);
  }

  return 0;
}

#define TEST_FUNCTION_ARGV do_test
#include <support/test-driver.c>
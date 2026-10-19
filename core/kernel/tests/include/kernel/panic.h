/* SPDX-License-Identifier: BSD-2-Clause */
#include <compiler.h>

void test_panic(void) __noreturn;
#define panic(...) test_panic()
#define EMSG(...) ((void)0)
#define DMSG(...) ((void)0)

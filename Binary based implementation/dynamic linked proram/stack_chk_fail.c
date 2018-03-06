/* Copyright (C) 2005-2014 Free Software Foundation, Inc.
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
   <http://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define CMP_CANARY_OFFSET        "0x2a0"  /* offset of cmp_canary A addr. in TLS */

extern char **__libc_argv attribute_hidden;

void
__attribute__ ((noreturn))
__stack_chk_fail (uintptr_t canary)
{
   __asm__("push %rax");
   __asm__("push %rdx");
   unsigned int TLS_canary;
   asm volatile ("mov %%fs:" CMP_CANARY_OFFSET  ", %0" : "=r" (TLS_canary));

   if(((unsigned int)((canary >> 0x20)^canary))!=TLS_canary){
      __fortify_fail("stack smashing detected");
   }else{
      __asm__("pop %rdx");
      __asm__("pop %rax");
   }
}

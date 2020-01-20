/*
 * Copyright (c) 2017, NanJing University
 *
 * This software was developed by WangZhilong <mg1633081@smail.nju.edu.cn>
 * at NanJing University, NanJing, JiangShu, CHina, in May 2017.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __LIBPOLYMORPHICCANARIES_H__
#define __LIBPOLYMORPHICCANARIES_H__

#include "offsets.h"

/* urandom device path */
#define URANDOM_PATH    "/dev/urandom"

/* compiler directives for branch prediction */
#define likely(x)       __builtin_expect((x), 1)
#define unlikely(x)     __builtin_expect((x), 0)

/*==================================================================*/
/*                                                                  */
/*                              DEBUG                               */
/*                                                                  */
/*==================================================================*/

#define DEBUG	0

/* debug printing */
#define P_PTR(PTR, x)	printf(PTR ": 0x%016lx\n", (x));
#define P(...)		{ printf("[+] " __VA_ARGS__); printf("\n"); }

#if	(DEBUG)
#define D(...)		P(__VA_ARGS__)
#define DPTR(...)	P_PTR(__VA_ARGS__)
#else
#define D(...)
#define DPTR(...)
#endif

typedef struct {
	uintptr_t *fptr;
	uintptr_t *arg;
} redun_arg_t;

/*==================================================================*/
/*                                                                  */
/*                          ARCH-SPECIFIC                           */
/*                                                                  */
/*==================================================================*/

#if     defined(__i386__)
/* TODO: 32-bit */
#error  "Unsupported architecture"
#elif   defined(__x86_64__)
#define GET_STACK_PTR(x)          \
    asm volatile ("mov %%rsp, %0" : "=r" (x));
/*
 * getter & setter functions for TLS data
 */
/* canary used by RedundantGuard */
#define GET_TLS(x)                \
    asm volatile ("mov %%fs:0x0, %0" : "=r" (x));

#define SET_CHECK_CANARY(x)   \
    asm volatile ("mov %0, %%fs:" CHECK_CANARY_OFFSET ""::"r" (x) : "memory");

#define GET_CHECK_CANARY(x)   \
    asm volatile ("mov %%fs:" CHECK_CANARY_OFFSET  ", %0" : "=r" (x));

#define SET_CANARY1(x)   \
    asm volatile ("mov %0, %%fs:" CANARY1_OFFSET ""::"r" (x) : "memory");

#define GET_CANARY1(x)   \
    asm volatile ("mov %%fs:" CANARY1_OFFSET  ", %0" : "=r" (x));

#define SET_CANARY2(x)   \
    asm volatile ("mov %0, %%fs:" CANARY2_OFFSET ""::"r" (x) : "memory");

#define GET_CANARY2(x)   \
    asm volatile ("mov %%fs:" CANARY2_OFFSET  ", %0" : "=r" (x));
#else
#error  "Unsupported architecture"
#endif

#endif  /* LIBREDUNDANTGUARD_H */

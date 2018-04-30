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
 
#ifndef __POLYMORPHICCANARIES_H__
#define __POLYMORPHICCANARIES_H__

#include "gcc-plugin.h"
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tree.h"
#include "tree-pass.h"
#include "basic-block.h"
#include "intl.h"
#include "plugin-version.h"
#include "rtl.h"
#include "emit-rtl.h"
#include "hashtab.h"
#include "config/i386/i386-protos.h"
#include "bitmap.h"
#include "alloc-pool.h"

#if (GCCPLUGIN_VERSION_MAJOR == 4) && (GCCPLUGIN_VERSION_MINOR == 9)
#include "df.h"
#include "context.h"
#else
#endif

#include <string>

#include "offsets.h"

#define NAME        "POLYMORPHICCANARIES"
#define DEBUG_INSN  1
#define LOG_STR		  "/tmp/POLYMORPHICCANARIES_log.c.rtl"
#define LOG_STR_N   32
#define ONE_WORD    0x8
#define SETJMP      "_setjmp"
#define SIGSETJMP   "__sigsetjmp"
#define DYNA_UNWIND "__dyna_unwind_cstack"

#define RES_REGNO R15_REG

/* compiler directives for branch prediction */
#define likely(x) 	__builtin_expect((x), 1)
#define unlikely(x)	__builtin_expect((x), 0)

enum {
	SUCCESS = 0,	/* success; return value */
	FAILURE = 1	  /* failed; return value */
};
#endif /* __DYNAGUARD_H__ */

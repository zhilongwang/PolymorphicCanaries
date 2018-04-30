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
#include <sys/mman.h>
#include <cxxabi.h>
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

#include <string>
#include <map>
#include <iostream>

#include "LIBPolymorphicCanaries"

/* (glibc) interposition prototypes */
sighandler_t __sys_signal_handler                                       = NULL;
void        (*__sys_sigaction_handler)(int, siginfo_t *, void *)        = NULL;
void *      (*__sys_start_routine)(void *)                              = NULL;

/* associative map: thread-id to per-thread context; lock-protected */
std::map<pthread_t, uintptr_t> tid_tls_map;
pthread_mutex_t                tmutex;

unsigned long pg_sz;     /* page size (in bytes)                    */
uintptr_t     main_tls;  /* TLS base (for main thread)              */



/* 
 * Update the canary in the TLS with a fresh value.
 */
static void
init_canary(void)
{
  union {
    uintptr_t num;
    unsigned char bytes[sizeof(uintptr_t)];
  } ret;
  ssize_t reslen;
  const ssize_t filllen = sizeof(ret.bytes) - 1;
  int fd; /* file descriptor for /dev/urandom */

  ret.num = 0;
  fd = open(URANDOM_PATH, O_RDONLY);
  if (fd >= 0) {
    reslen = read(fd, ret.bytes + 1, filllen);
    if (unlikely(reslen != filllen)) {
      perror("RedunGuard: reading from the random device failed (renew_canary");
      close(fd);
      exit(EXIT_FAILURE);
    }
  }
  close(fd);

  /* update the value in the TLS */
  ret.num=ret.num & 0xFFFFFFFFFFFFFF00;
  SET_REDUNDANT_STACK_GUARD(ret.num);
  unsigned int cmp_canary=ret.num;
  ret.num=ret.num >> 0x20;
  cmp_canary=cmp_canary^(unsigned int )ret.num;
  SET_CMP_STACK_GUARD(cmp_canary);
  //printf("Init canary!");
}
/* 
 * Init the canary in the TLS with a fresh value.
 */
static void
renew_canary(void)
{
  union {
    uintptr_t num;
    unsigned char bytes[sizeof(uintptr_t)];
  } ret;
  ssize_t reslen;
  const ssize_t filllen = sizeof(ret.bytes) - 1;
  int fd; /* file descriptor for /dev/urandom */

  ret.num = 0;
  fd = open(URANDOM_PATH, O_RDONLY);
  if (fd >= 0) {
    reslen = read(fd, ret.bytes + 1, filllen);
    if (unlikely(reslen != filllen)) {
      perror("RedunGuard: reading from the random device failed (init_canary");
      close(fd);
      exit(EXIT_FAILURE);
    }
  }
  close(fd);
  unsigned int cmp_canary,tmp;
  GET_CMP_STACK_GUARD(cmp_canary);

  ret.num=ret.num & 0x00000000FFFFFF00;
  tmp=(unsigned int)ret.num;
  ret.num=(ret.num ^ (uintptr_t)cmp_canary) << 0x20;
  ret.num=ret.num+(uintptr_t)tmp;
  SET_REDUNDANT_STACK_GUARD(ret.num);
}




/*
 * pthread_create(3) helper: wraps the `start_routine' of the
 * newly-created thread to setup the TLS entries accordingly.
 */
static void *
__dyna_start_routine(void *darg)
{
  void      *ret;       /* return value                 */
  uintptr_t tls;        /* TLS address (current thread) */

  /* FIXME: ugly argument passing */
  void * (*__sys_start_routine)(void *) =
    (void * (*)(void *))((redun_arg_t *)darg)->fptr;
  void *arg = (void *)((redun_arg_t *)darg)->arg;
  free((redun_arg_t *)darg);

  /* get the TLS */
  GET_TLS(tls);

  /* add the current thread to the associative map */
  pthread_mutex_lock(&tmutex);
  tid_tls_map[pthread_self()] = tls;
  pthread_mutex_unlock(&tmutex);


  
  /* get a fresh canary */
  renew_canary();

  /* start the (original) main routine */
  ret = __sys_start_routine(arg);


  /* done */
  return ret;
}



/*
 * signal(2) hook.
 */
sighandler_t
signal(int signum, sighandler_t handler)
{
  sighandler_t __prev_handler; /* previous signal handler */

  typedef sighandler_t (*fptr) (int signum, sighandler_t handler);
  static fptr __sys_signal = NULL;

  if (unlikely(!__sys_signal &&
      !(__sys_signal = (fptr) dlsym(RTLD_NEXT, "signal")))) {
    perror("Redundantguard: failed to locate signal(2)");
    exit(EXIT_FAILURE);
  }
  /* do it */
  if (signum != SIGSEGV)
    return __sys_signal(signum, handler);

  /* save the signal handler */
  __prev_handler       = __sys_signal_handler;
  __sys_signal_handler = handler;

  /* return the previous signal handler */
  return __prev_handler;
}

/*
 * pthread_create(3) hook.
 */
int
pthread_create(pthread_t *thread,
               const pthread_attr_t *attr,
               void *(*start_routine)(void *),
               void *arg)
{
  typedef int (*fptr) (pthread_t *thread,
                       const pthread_attr_t *attr,
                       void *(*start_routine)(void *),
                       void *arg);
  static fptr __sys_pthread_create = NULL;
  redun_arg_t *darg = NULL;
  /* get the actual (POSIX threads) pthread_create(3) */
  if (unlikely(!__sys_pthread_create &&
  !(__sys_pthread_create = (fptr) dlsym(RTLD_NEXT, "pthread_create")))) {
    perror("RedundantGuard: failed to locate pthread_create(3)");
    exit(EXIT_FAILURE);
  }

  /* FIXME: ugly argument passing */
  if (unlikely((darg = (redun_arg_t *)malloc(sizeof(redun_arg_t))) == NULL)) {
    perror("RedundantGuard: failed to allocate redun_arg_t (pthread_create)");
    exit(EXIT_FAILURE);
  }

  /* original `start_routine' and its argument */
  darg->fptr = (uintptr_t *)start_routine;
  darg->arg  = (uintptr_t *)arg; 

  /* do it */
  return __sys_pthread_create(thread,
                              attr,
                              __dyna_start_routine,
                              darg);
}
/*
 * fork(2) hook.
 */
pid_t
fork(void)
{
  typedef pid_t (*fptr) (void);
  static fptr __sys_fork = NULL;
  pid_t pid;

  /* get the actual (glibc) fork(2) */
  if (unlikely(!__sys_fork &&
        !(__sys_fork = (fptr) dlsym(RTLD_NEXT, "fork")))) {
    perror("DynaGuard: failed to locate fork(2)");
    exit(EXIT_FAILURE);
  }
  /* do it */
  if ((pid = __sys_fork()) == 0) {
    /* update the canaries */
    renew_canary();
  }

  /* done */
  return pid;
}



/*
 * Constructor (called before `main'): setup RedundantGuard's canary and initialize
 * the entries in TLS related to the canary address buffer (CAB) .
 */
__attribute__((constructor)) void
setup_redundantguard(void)
{
  /* get a fresh canary */
  init_canary();


}

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

#include "PolymorphicCanaries.h"
# include <stdio.h>
# include <stdlib.h>
// #include<iostream>
#if DEBUG_INSN

static FILE *flog = NULL;

/*
 * start the auditing process
 * open the logfile in appending mode
 */
static void
openlog(void)
{
  char lname[LOG_STR_N];

  /* differet log file per thread */
  memset(lname, 0, LOG_STR_N);
  sprintf(lname, "%s.%d", LOG_STR, getpid());

  /* open the logfile in appending mode */
  if (likely((flog = fopen(lname, "a")) != NULL))
    /* dump information regarding the translation unit */
    (void)fprintf(flog, "[%s]\n", IDENTIFIER_POINTER(DECL_NAME(cfun->decl)));
  else
    /* failed */
    (void)fprintf(stderr, "%s: failed to open %s\n", NAME, lname);
}

/*
 * terminate the auditing process
 *
 * close the logfile
 */
static void
closelog(void)
{
  /* check if logging has been enabled */
  if (flog != NULL) {
    /* dump information regarding the translation unit */
    (void)fprintf(flog, "[/%s]\n", IDENTIFIER_POINTER(DECL_NAME(cfun->decl)));
    /* cleanup */
    (void)fclose(flog);
  }
}

/*
 * do the actual logging
 *
 * print the RTL expression of the inspected instruction; in case of
 * instrumented instructions print an additional discriminator (i.e.,
 * `M' for branches via an unsafe memory location, and `SM`/`R` for
 * branches via safe memory locations or registers)
 *
 * insn:	the instruction to log
 */
static void
commitlog(const rtx insn, const char *msg)
{
  /* check if logging has been enabled */
  if (flog != NULL) {
    (void)fprintf(flog, msg);
    /* dump the instruction */
    print_rtl_single(flog, insn);
  }
}
#endif /* DEBUG LOGGING END */

int plugin_is_GPL_compatible;


static std::string
signed_int_to_str(signed int num){
  char i_map_a[17]="0123456789abcdef";
  std::string str="";
  signed int absnum;
  if(num<0){
    absnum=-num;
  }else{
    absnum=num;
  }
  while(absnum>0){
    int tmp=absnum%16;
    absnum=absnum/16;
    str=i_map_a[tmp]+str;
  }
  if(num<0){
    str="-"+str;
  }
  return str;
}

/*
 *  Returns a 64-bit register string from an RTX REG expression
 */
static std::string
reg_to_str(rtx reg)
{
  std::string regstr;

  /* if reg is rxx do nothing */
  if (reg_names[REGNO(reg)][0] == 'r')
    regstr = "%%" + std::string(reg_names[REGNO(reg)],
                                strlen(reg_names[REGNO(reg)]));
  else
#ifdef __i386__
    /* if reg is ax, bx etc return the 64-bit version: rax, rbx etc */
    regstr = "%%e" + std::string(reg_names[REGNO(reg)],
                                 strlen(reg_names[REGNO(reg)]));
#elif defined __x86_64__
    /* if reg is ax, bx etc return the 64-bit version: rax, rbx etc */
    regstr = "%%r" + std::string(reg_names[REGNO(reg)],
                                 strlen(reg_names[REGNO(reg)]));
#endif
  return regstr;
}


/*
 * Pop last entry from canary address buffer upon frame destruction
 * Returns: true on success, false on error
 */
static bool
canary_pop(rtx *insn)
{
  rtx tls_set;    /* subexpressions in PARALLEL */
  rtx as;         /* assembly instructions to be pushed */
  char *mov_cstr;               /* char array for assembly insns */
  rtx creg;                     /* scratch register */
  std::string movstr, rsp_offset, r1,r2,r3;

  if (!(GET_CODE(*insn) == INSN && GET_CODE(PATTERN(*insn)) == PARALLEL))
    return false;

  /* get first PARALLEL subexpression and check if SP_TLS_TEST */
  tls_set = XVECEXP(PATTERN(*insn), 0, 0);

  /* if not a canary check return */
  if (!(tls_set                                  &&
        GET_CODE(tls_set) == SET                 &&
        REG_P(XEXP(tls_set, 0))                  &&
        REGNO(XEXP(tls_set, 0))    == FLAGS_REG  &&
        GET_MODE(XEXP(tls_set, 0)) == CCZmode    &&
        GET_CODE(XEXP(tls_set, 1)) == UNSPEC     &&
        XINT(XEXP(tls_set, 1), 1)  == UNSPEC_SP_TLS_TEST))
    return false;

 
  /*printf("step1\n");

  XEXP(tls_set, 1);
  //std::cout<<"getR"<<std::hex<<offset<<" "<<rsp_offset<<std::endl;
  std::cout<<"GET_CODE(PATTERN(XEXP(tls_set, 1)))"<<std::hex<<" "<<GET_CODE(XEXP(tls_set, 0))<<std::endl;
  std::cout<<"XVECLEN(XEXP(tls_set, 1),0)"<<std::hex<<" "<<XVECLEN(XEXP(tls_set, 1),0)<<std::endl;
  
  FILE *file=NULL;
  file=fopen("rsp.txt","w+");
  rewind(file);
  print_rtl (file, tls_set);
  rewind(file);
  char str[129];
  while(!feof(file)) {  
    if(fgets(str,128,file)!=NULL)  
    printf("%s",str);  
  }  
  fclose(file);
  printf("step3\n");
  PATTERN(XEXP(tls_set, 1));
  printf("step4\n");
  
  signed int offset= XINT(XVECEXP(XVECEXP(PATTERN(XEXP(tls_set, 1)),0,0),0,0),0);
   printf("step4\n");
  rsp_offset=signed_int_to_str(offset);
  std::cout<<"getR"<<std::hex<<offset<<" "<<rsp_offset<<std::endl;
*/
  /*FILE *file=NULL;
  file=fopen("rsp.txt","w+");
  rewind(file);
  print_rtl (file, tls_set);
  rewind(file);
  char str[129];
  while(!feof(file)) {  
    if(fgets(str,128,file)!=NULL)  
    printf("%s",str);  
  }  
  fclose(file);

  printf("\ncanary_addr\n");
  rtx  canary_addr;

  if(tls_set != NULL_RTX){
      printf("\nif__1\n");
      canary_addr=XEXP(tls_set, 1);
      FILE *file=NULL;
    file=fopen("rsp.txt","w+");
    rewind(file);
    print_rtl (file, canary_addr);
    rewind(file);
    char str[129];
    while(!feof(file)) {  
      if(fgets(str,128,file)!=NULL)  
      printf("%s",str);  
    }  
    fclose(file);
    if(canary_addr != NULL_RTX){
      printf("\nif__2\n");
      //canary_addr=PATTERN(XEXP(tls_set, 1));
      
      if(canary_addr != NULL_RTX){
        printf("\nif__3\n");
        canary_addr = XVECEXP(canary_addr, 0,0);
        FILE *file=NULL;
        file=fopen("rsp.txt","w+");
        rewind(file);
        print_rtl (file, canary_addr);
        rewind(file);
        char str[129];
        while(!feof(file)) {  
          if(fgets(str,128,file)!=NULL)  
          printf("%s",str);  
        }  
        fclose(file);
      }
    }
  }*/


  rtx canary_addr = XVECEXP(XEXP(tls_set, 1), 0,0);
  if(canary_addr == NULL_RTX){
    printf("\nNULL_RTX\n");
    return false;
  }



  /*signed int offset= XINT(XVECEXP(XVECEXP(PATTERN(XEXP(tls_set, 1)),0,0),0,1),0);
  rsp_offset=signed_int_to_str(offset);
  std::cout<<"getR"<<std::hex<<offset<<" "<<rsp_offset<<std::endl;
 

  FILE *p=fopen("rsp.txt","w");
  print_rtl (p, XEXP(canary_addr, 0));
  fclose(p);*/

  /*(plus:DI (reg/f:DI 6 bp)
    (const_int -8 [0xfffffffffffffff8]))*/




  creg = XEXP(XVECEXP(PATTERN(*insn), 0, 1), 0);
  r1   = reg_to_str(creg);
  if (REGNO(creg) == 0x1){//creg=rdx
    r2 = "%%rcx";
    r3 = "%%ecx";
  }else{
    r2 = "%%rdx";
    r3 = "%%edx";
  }

  emit_insn_before(gen_rtx_SET(DImode,
                                creg,
                                canary_addr),
                              *insn);

  /* pop canary from shadow stack */
  movstr = "";
  movstr = movstr + "push "+r2+"\n\t"; 
  movstr = movstr + "mov "+r1+", " + r2 + "\n\t";      /* copy canary to rcx*/
  movstr = movstr + "sar "+"$0x20"+", " + r2 + "\n\t"; /* get the random B in high 2 byte of rcx*/
  movstr = movstr + "xor "+r1+", " + r2 + "\n\t";      /* xor B and B' to get A */
  movstr = movstr + "xor %%fs:" CMP_CANARY_OFFSET ", "+r3+"\n\t";             /*cmp xor value with canary_cmp named A*/
  movstr = movstr + "pop "+r2+"\n\t"; 
 
  mov_cstr = new char[movstr.length() + 1];
  std::strcpy(mov_cstr, movstr.c_str());
  /* ugly hack */
  as = gen_rtx_ASM_OPERANDS(VOIDmode,
                            mov_cstr,
                            "",
                            0,
                            rtvec_alloc(0),
                            rtvec_alloc(0),
                            rtvec_alloc(0),
                            expand_location(RTL_LOCATION(*insn)).line);

  emit_insn_after(as, *insn);
  delete_insn(*insn);
  return true;
}

/*
 * main pass
 * (invoked for every translation unit)
 *
 * returns: SUCCESS on success, FAILURE on error
 */
static unsigned int
execute_redundantguard(void)
{

#ifdef __i386__
  return SUCCESS; /* FIXME */
#endif

  rtx insn;          /* instruction (INSN) iterator */

#if DEBUG_INSN
  openlog();
#endif

  /* modify canary upon canary push and modify check upon pop*/
  for (insn = get_insns(); insn; NEXT_INSN(insn)) {
    commitlog(insn, "INSN\n");

    /* ignore assembly */
    if (GET_CODE(insn) == INSN && asm_noperands(PATTERN(insn)) >= 0)
      continue;

 
    /* hacky optimization to skip extra checks */
    if (canary_pop(&insn))
      continue;
  }


#if DEBUG_INSN
  closelog();
#endif

  /* return with success */
  return SUCCESS;
}

#if (GCCPLUGIN_VERSION_MAJOR == 4) && (GCCPLUGIN_VERSION_MINOR == 9)
namespace {

const pass_data pass_data_redundantguard =
{
  RTL_PASS,      /* type */
  NAME,          /* name */
  OPTGROUP_NONE, /* optinfo_flags */
  false,         /* has gate */
  true,          /* has execute */
  TV_NONE,       /* tv_id */
  PROP_rtl,      /* properties_required */
  0,             /* properties_provided */
  0,             /* properties_destroyed */
  0,             /* todo_flags_start */
  0,             /* todo_flags_finish */
};

class pass_redundantguard : public rtl_opt_pass
{
 public:
  pass_redundantguard(gcc::context *ctxt)
      : rtl_opt_pass(pass_data_redundantguard, ctxt)
  {}

  /* opt_pass methods: */
  bool gate () {}
  unsigned int execute () { return execute_redundantguard(); }

}; /* class pass_redundantguard */

} /* anon namespace */

static rtl_opt_pass *
make_pass_redundantguard(gcc::context *ctxt)
{
  return new pass_redundantguard(ctxt);
}
#else

static bool
gate_redundantguard(void)
{
  return true;
}

static struct rtl_opt_pass redundantguard =
{
  {
    RTL_PASS,           /* type */
    NAME,               /* name */
    gate_redundantguard,     /* gate */
    execute_redundantguard,  /* execute */
    NULL,               /* sub */
    NULL,               /* next */
    0,                  /* static pass number */
    TV_NONE,            /* tv_id */
    PROP_rtl,           /* properties_required */
    0,                  /* properties_provided */
    0,                  /* properties_destroyed */
    0,                  /* todo_flags_start */
    0,                  /* todo_flags_finish */
  }
};

#endif

int
plugin_init(struct plugin_name_args *plugin_info,
            struct plugin_gcc_version *version)
{
  struct register_pass_info pass_info;

  if (!plugin_default_version_check(version, &gcc_version))
    return FAILURE;

#if (GCCPLUGIN_VERSION_MAJOR == 4) && (GCCPLUGIN_VERSION_MINOR == 9)
  pass_info.pass = make_pass_redundantguard(g);
#else
  pass_info.pass = &redundantguard.pass;
#endif
  pass_info.reference_pass_name = "vartrack";
  pass_info.ref_pass_instance_number = 1;
  pass_info.pos_op = PASS_POS_INSERT_AFTER;

  /* provide the pass information to the pass manager */
  register_callback(NAME, PLUGIN_PASS_MANAGER_SETUP, NULL, &pass_info);

  return SUCCESS;
}

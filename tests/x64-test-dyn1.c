#include "flush-cache.h"

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>

#include <libunwind.h>

static char asmfunc_1_bytes[] = {
  /*  0 */ 0x55,                                       /* push %rbp */
  /*  1 */ 0x48, 0x89, 0xe5,                           /* mov %rsp, %rbp */
  /*  4 */ 0x48, 0x83, 0xec, 0x10,                     /* sub $0x10, %rsp */
  /*  8 */ 0x48, 0xc7, 0xc0, 0x2a, 0x00, 0x00, 0x00,   /* mov $42, %rax */
  /* 15 */ 0x48, 0x89, 0xec,                           /* mov %rbp, %rsp */
  /* 18 */ 0x5d,                                       /* pop %rbp */
  /* 19 */ 0xc3,                                       /* ret */
};
static int asmfunc_1_insn_offsets[] = {
  0, 1, 4, 8, 15, 18, 19
};

static sigjmp_buf return_env;
static char *exec_mapping;

struct captured_frame_info {
  char procname[256];
  unw_word_t offset;
};

static struct captured_frame_info captured_frames[5];

static void trap_handler(int signal, siginfo_t *siginfo, void *context)
{
  int r;  
  unw_cursor_t c;
  unw_context_t uc;

  r = unw_getcontext(&uc);
  assert(r == 0);
  r = unw_init_local(&c, &uc);
  assert(r == 0);

  /* Step five times - we expect to see:
       * trap_handler
       * the __sigaction thunk
       * asmfunc_1
       * run_asmfunc1_traps
       * main
  */

  for (int i = 0; i < 5; i++)
    {
      unw_word_t ip, sp;
      assert(unw_get_reg(&c, UNW_X86_64_RIP, &ip) == 0);
      assert(unw_get_reg(&c, UNW_X86_64_RSP, &sp) == 0);
      printf("(unwound ip 0x%lx sp 0x%lx)\n", ip, sp);

      r = unw_get_proc_name(&c, captured_frames[i].procname,
                            sizeof(captured_frames[i].procname),
                            &captured_frames[i].offset);
      assert(r == 0);
      if (i < 4)
        assert(unw_step(&c) > 0);
    }
  siglongjmp(return_env, 1);
}

__attribute__ ((noinline))
static void run_asmfunc_1_traps(void)
{
  /* make sure this function can't get optimized away */
  asm("");

  int insn_count = sizeof(asmfunc_1_insn_offsets)/sizeof(asmfunc_1_insn_offsets[0]);
  for (int i = 0; i < insn_count; i++)
    {
      /* copy the function into the executable memory */
      memcpy(exec_mapping, asmfunc_1_bytes, sizeof(asmfunc_1_bytes));
      /* overwrite the instruction with one that will fault. I'd like to use 0xcc (INT 3),
        which generates a SIGTRAP, but that actually inteferes with debugging the test in
        gdb, which is kind of annoying. Instead, use 0xf4 (HLT), which will generate a
        SIGSEGV when called outside of kernel mode. */
      exec_mapping[asmfunc_1_insn_offsets[i]] = 0xf4;
      flush_cache(exec_mapping, sizeof(asmfunc_1_bytes));
      unw_flush_cache(unw_local_addr_space, 0, 0);

      int r = sigsetjmp(return_env, 1);
      if (r == 0)
        {
          /* runs the first time here */
          ((void (*)(void))exec_mapping)();
        }
      /* control falls here after the trap handler runs */

      for (int j = 0; j < 5; j++)
        {
          printf("frame %d: %s+0x%lx\n", j, captured_frames[j].procname, captured_frames[j].offset);
        }
      
      printf("asserting round %d (offset %d)\n", i, asmfunc_1_insn_offsets[i]);
      assert(strcmp(captured_frames[0].procname, "trap_handler") == 0);
      /* don't asserting on __sigaction return thunk */
      assert(strcmp(captured_frames[2].procname, "asmfunc_1") == 0);
      assert(captured_frames[2].offset == asmfunc_1_insn_offsets[i]);
      assert(strcmp(captured_frames[3].procname, "run_asmfunc_1_traps") == 0);
    }

    asm("");
}

static unw_dyn_info_t asmfunc_1_proc_info_sp;
void configure_asmfunc_1_proc_info_sp(void)
{
  asmfunc_1_proc_info_sp.start_ip = (uintptr_t)exec_mapping;
  asmfunc_1_proc_info_sp.end_ip = ((uintptr_t)exec_mapping) + sizeof(asmfunc_1_bytes);
  asmfunc_1_proc_info_sp.format = UNW_INFO_FORMAT_DYNAMIC;
  asmfunc_1_proc_info_sp.u.pi.name_ptr = (uintptr_t)"asmfunc_1";

  size_t pre_region_sz = _U_dyn_region_info_size(3);
  unw_dyn_region_info_t *pre_region = malloc(pre_region_sz);
  memset(pre_region, 0, pre_region_sz);

  _U_dyn_op_add(&pre_region->op[0], _U_QP_TRUE, 0, UNW_X86_64_RSP, -0x08);
  _U_dyn_op_spill_sp_rel(&pre_region->op[1], _U_QP_TRUE, 0, UNW_X86_64_RIP, 0);
  _U_dyn_op_stop(&pre_region->op[2]);
  pre_region->insn_count = 0;
  pre_region->op_count = 3;

  size_t body_region_sz = _U_dyn_region_info_size(7);
  unw_dyn_region_info_t *body_region = malloc(body_region_sz);
  memset(body_region, 0, body_region_sz);

  _U_dyn_op_add(&body_region->op[0], _U_QP_TRUE, 0, UNW_X86_64_RSP, -0x08);
  _U_dyn_op_spill_sp_rel(&body_region->op[1], _U_QP_TRUE, 0, UNW_X86_64_RBP, 0);
  _U_dyn_op_add(&body_region->op[2], _U_QP_TRUE, 4, UNW_X86_64_RSP, -0x10);
  _U_dyn_op_add(&body_region->op[3], _U_QP_TRUE, 15, UNW_X86_64_RSP, 0x10);
  _U_dyn_op_add(&body_region->op[4], _U_QP_TRUE, 18, UNW_X86_64_RSP, 0x08);
  _U_dyn_op_restore_reg(&body_region->op[5], _U_QP_TRUE, 18, UNW_X86_64_RBP);
  _U_dyn_op_stop(&body_region->op[6]);
  body_region->insn_count = sizeof(asmfunc_1_bytes);
  body_region->op_count = 7;
  
  pre_region->next = body_region;
  asmfunc_1_proc_info_sp.u.pi.regions = pre_region;
}

static unw_dyn_info_t asmfunc_1_proc_info_fp;
void configure_asmfunc_1_proc_info_fp(void)
{
  asmfunc_1_proc_info_fp.start_ip = (uintptr_t)exec_mapping;
  asmfunc_1_proc_info_fp.end_ip = ((uintptr_t)exec_mapping) + sizeof(asmfunc_1_bytes);
  asmfunc_1_proc_info_fp.format = UNW_INFO_FORMAT_DYNAMIC;
  asmfunc_1_proc_info_fp.u.pi.name_ptr = (uintptr_t)"asmfunc_1";


  size_t pre_region_sz = _U_dyn_region_info_size(3);
  unw_dyn_region_info_t *pre_region = malloc(pre_region_sz);
  memset(pre_region, 0, pre_region_sz);

  _U_dyn_op_add(&pre_region->op[0], _U_QP_TRUE, 0, UNW_X86_64_RSP, -0x08);
  _U_dyn_op_spill_sp_rel(&pre_region->op[1], _U_QP_TRUE, 0, UNW_X86_64_RIP, 0);
  _U_dyn_op_stop(&pre_region->op[2]);
  pre_region->insn_count = 0;
  pre_region->op_count = 3;

  size_t body_region_sz = _U_dyn_region_info_size(10);
  unw_dyn_region_info_t *body_region = malloc(body_region_sz);
  memset(body_region, 0, body_region_sz);

  /* Although the title of this test is "fp relative", we can only refer to the saved
     value of RBP by the stack pointer after the first instruction - we haven't set up
     the frame pointer yet. We'll overwrite this for subsequent instructions. */
  _U_dyn_op_add(&body_region->op[0], _U_QP_TRUE, 0, UNW_X86_64_RSP, -0x08);
  _U_dyn_op_spill_sp_rel(&body_region->op[1], _U_QP_TRUE, 0, UNW_X86_64_RBP, 0);
  /* _now_, at the second instructoin, we can make it fp-relative */
  _U_dyn_op_save_sp(&body_region->op[2], _U_QP_TRUE, 1);
  /* previous %rbp is at [%rbp] */
  _U_dyn_op_spill_fp_rel(&body_region->op[3], _U_QP_TRUE, 1, UNW_X86_64_RBP, 0);
  /* previous ip is at [%rbp+8] */
  _U_dyn_op_spill_fp_rel(&body_region->op[4], _U_QP_TRUE, 1, UNW_X86_64_RIP, 8);
  /* epilogue */
  _U_dyn_op_restore_reg(&body_region->op[5], _U_QP_TRUE, 18, UNW_X86_64_RBP);
  /* %rip & %rbp can't be fp-relative anymore */
  _U_dyn_op_restore_reg(&body_region->op[6], _U_QP_TRUE, 18, UNW_X86_64_RSP);
  _U_dyn_op_add(&body_region->op[7], _U_QP_TRUE, 18, UNW_X86_64_RSP, -0x08);
  _U_dyn_op_spill_sp_rel(&body_region->op[8], _U_QP_TRUE, 18, UNW_X86_64_RIP, 0);
  _U_dyn_op_stop(&body_region->op[9]);
  body_region->insn_count = sizeof(asmfunc_1_bytes);
  body_region->op_count = 10;
  
  pre_region->next = body_region;
  asmfunc_1_proc_info_fp.u.pi.regions = pre_region;
}

int main(int argc, char *argv[])
{
  int r;
  exec_mapping = mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  assert(exec_mapping != MAP_FAILED);


  struct sigaction sa = {0};
  sa.sa_sigaction = trap_handler;
  sa.sa_flags = SA_SIGINFO;
  r = sigaction(SIGSEGV, &sa, NULL);
  assert(!r);

  /* There are two different sets of debuginfo we can use for asmfunc_1;
     one uses sp-relative offsets, one uses fp-relative offsets. */
  configure_asmfunc_1_proc_info_sp();
  configure_asmfunc_1_proc_info_fp();


  unw_set_caching_policy(unw_local_addr_space, UNW_CACHE_NONE);
  _U_dyn_register(&asmfunc_1_proc_info_sp);
  printf("running sp-relative local tests\n");
  run_asmfunc_1_traps();
  _U_dyn_cancel(&asmfunc_1_proc_info_sp);

  _U_dyn_register(&asmfunc_1_proc_info_fp);
  printf("running fp-relative local tests\n");
  run_asmfunc_1_traps();
  _U_dyn_cancel(&asmfunc_1_proc_info_fp);
  

  printf("exiting normally\n");
  exit(0);
}

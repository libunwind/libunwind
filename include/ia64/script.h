/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2002 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

This file is part of libunwind.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

#define IA64_LOG_UNW_CACHE_SIZE	7
#define IA64_UNW_CACHE_SIZE	(1 << IA64_LOG_UNW_CACHE_SIZE)

#define IA64_LOG_UNW_HASH_SIZE	(IA64_LOG_UNW_CACHE_SIZE + 1)
#define IA64_UNW_HASH_SIZE	(1 << IA64_LOG_UNW_HASH_SIZE)

typedef unsigned char unw_hash_index_t;

enum ia64_script_insn_opcode
  {
    IA64_INSN_SET,		/* s[dst] = val */
    IA64_INSN_SET_REG,		/* s[dst] = REG(val) */
    IA64_INSN_ADD_PSP,		/* s[dst] = (s.psp + val) */
    IA64_INSN_ADD_SP,		/* s[dst] = (s.sp + val) */
    IA64_INSN_MOVE,		/* s[dst] = s[val] */
    IA64_INSN_MOVE_STACKED,	/* s[dst] = ia64_rse_skip(*s.bsp_loc, val) */
    IA64_INSN_MOVE_SCRATCH,	/* s[dst] = scratch reg "val" */
    IA64_INSN_SETNAT_MEMSTK,	/* s[dst].nat.type = s.pri_unat_loc | MEMSTK */
    IA64_INSN_INC_PSP,		/* psp += val */
    IA64_INSN_LOAD_PSP		/* psp = *psp_loc */
  };

struct ia64_script_insn
  {
    unsigned int opc;
    unsigned int dst;
    unw_word_t val;
  };

/* Preserved general static registers (r4-r7) give rise to two script
   instructions; everything else yields at most one instruction; at
   the end of the script, the psp gets popped, accounting for one more
   instruction.  */
#define IA64_MAX_SCRIPT_LEN	(IA64_NUM_PREGS + 5)

struct ia64_script
  {
    unw_word_t ip;		/* ip this script is for */
    unw_word_t pr_mask;		/* mask of predicates script depends on */
    unw_word_t pr_val;		/* predicate values this script is for */
    unw_proc_info_t pi;		/* info about underlying procedure */
    unsigned short lru_chain;	/* used for least-recently-used chain */
    unsigned short coll_chain;	/* used for hash collisions */
    unsigned short hint;	/* hint for next script to try (or -1) */
    unsigned short count;	/* number of instructions in script */
    unsigned short abi_marker;
    struct ia64_script_insn insn[IA64_MAX_SCRIPT_LEN];
  };

struct ia64_script_cache
  {
#ifdef HAVE_ATOMIC_OPS_H
    AO_TS_t busy;		/* is the script-cache busy? */
#else
    pthread_mutex_t lock;
#endif
    unsigned short lru_head;	/* index of lead-recently used script */
    unsigned short lru_tail;	/* index of most-recently used script */

    /* hash table that maps instruction pointer to script index: */
    unsigned short hash[IA64_UNW_HASH_SIZE];

    uint32_t generation;	/* generation number */

    /* script cache: */
    struct ia64_script buckets[IA64_UNW_CACHE_SIZE];
  };

#define ia64_get_cached_proc_info	UNW_OBJ(ia64_get_cached_proc_info)

struct cursor;			/* forward declaration */

extern int ia64_get_cached_proc_info (struct cursor *c);

/* libunwind - a platform-independent unwind library
   Copyright (C) 2001-2002 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

This file is part of libunwind.

libunwind is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

libunwind is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

As a special exception, if you link this library with other files to
produce an executable, this library does not by itself cause the
resulting executable to be covered by the GNU General Public License.
This exception does not however invalidate any other reasons why the
executable file might be covered by the GNU General Public
License.  */

#define IA64_LOG_UNW_CACHE_SIZE	7
#define IA64_UNW_CACHE_SIZE	(1 << IA64_LOG_UNW_CACHE_SIZE)

#define IA64_LOG_UNW_HASH_SIZE	(IA64_LOG_UNW_CACHE_SIZE + 1)
#define IA64_UNW_HASH_SIZE	(1 << IA64_LOG_UNW_HASH_SIZE)

typedef unsigned char unw_hash_index_t;

enum ia64_script_insn_opcode
  {
    IA64_INSN_SET,		/* s[dst] = val */
    IA64_INSN_ADD,		/* s[dst] += val */
    IA64_INSN_ADD_PSP,		/* s[dst] = (s.psp + val) */
    IA64_INSN_ADD_SP,		/* s[dst] = (s.sp + val) */
    IA64_INSN_MOVE,		/* s[dst] = s[val] */
    IA64_INSN_MOVE_STACKED,	/* s[dst] = ia64_rse_skip(*s.bsp_loc, val) */
    IA64_INSN_MOVE_SCRATCH,	/* s[dst] = scratch reg "val" */
    IA64_INSN_SETNAT_MEMSTK,	/* s[dst].nat.type = s.pri_unat_loc | MEMSTK */
    IA64_INSN_LOAD		/* s[dst] = *s[val] */
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
    struct ia64_proc_info pi;	/* info about underlying procedure */
    unsigned short lru_chain;	/* used for least-recently-used chain */
    unsigned short coll_chain;	/* used for hash collisions */
    unsigned short hint;	/* hint for next script to try (or -1) */
    unsigned short count;	/* number of instructions in script */
    struct ia64_script_insn insn[IA64_MAX_SCRIPT_LEN];
  };

struct ia64_script_cache
  {
    unsigned short lru_head;	/* index of lead-recently used script */
    unsigned short lru_tail;	/* index of most-recently used script */

    /* hash table that maps instruction pointer to script index: */
    unsigned short hash[IA64_UNW_HASH_SIZE];

    uint32_t generation;	/* generation number */

    /* script cache: */
    struct ia64_script buckets[IA64_UNW_CACHE_SIZE];
  };

#define ia64_script_lookup	UNW_OBJ(ia64_script_lookup)

extern struct ia64_script *ia64_script_lookup (struct ia64_cursor *c);

/* libunwind - a platform-independent unwind library
   Copyright (C) 2002 Hewlett-Packard Co
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

#ifndef mempool_h
#define mempool_h

/* Memory pools provide simple memory management of fixed-size
   objects.  Memory pools are used for two purposes:

     o To ensure a stack can be unwound even when a process
       is out of memory.

      o To ensure a stack can be unwound at any time in a
        multi-threaded process (e.g., even at a time when the normal
        malloc-lock is taken, possibly by the very thread that is
        being unwind).


    To achieve the second objective, memory pools allocate memory
    directly via mmap() system call (or an equivalent facility).

    The first objective is accomplished by reserving memory ahead of
    time.  Since the memory requirements of stack unwinding generally
    depends on the complexity of the procedures being unwind, there is
    no absolute guarantee that unwinding will always work, but in
    practice, this should not be a serious problem.  */

#include <sys/types.h>

#define IPREFIX			UNW_PASTE(_UI,UNW_TARGET)
#define sos_alloc(s)		UNW_PASTE(IPREFIX, _sos_alloc)(s)
#define sos_free(p)		UNW_PASTE(IPREFIX, _sos_free)(p)
#define mempool_init(p,s,r)	UNW_PASTE(IPREFIX, _mempool_init)(p,s,r)
#define mempool_alloc(p)	UNW_PASTE(IPREFIX, _mempool_alloc)(p)
#define mempool_free(p,o)	UNW_PASTE(IPREFIX, _mempool_free)(p,o)

/* The mempool structure should be treated as an opaque object.  It's
   declared here only to enable static allocation of mempools.  */
struct mempool
  {
    size_t obj_size;		/* object size (rounded up for alignment) */
    unsigned int reserve;	/* minimum (desired) size of the free-list */
    size_t chunk_size;		/* allocation granularity */
    unsigned int num_free;	/* number of objects on the free-list */
    struct object
      {
	struct object *next;
      }
    *free_list;
  };

/* Emergency allocation for one-time stuff that doesn't fit the memory
   pool model.  */
extern void *sos_alloc (size_t size);
extern void sos_free (void *ptr);

/* Initialize POOL for an object size of OBJECT_SIZE bytes.  RESERVE
   is the number of objects that should be reserved for use under
   tight memory situations.  If it is zero, mempool attempts to pick a
   reasonable default value.  */
extern void mempool_init (struct mempool *pool,
			  size_t obj_size, size_t reserve);
extern void *mempool_alloc (struct mempool *pool);
extern void mempool_free (struct mempool *pool, void *object);

#endif /* mempool_h */

/* libunwind - a platform-independent unwind library
   Copyright (C) 2002-2003 Hewlett-Packard Co
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>

#include "libunwind.h"
#include "mempool.h"

#define MAX_ALIGN	(sizeof (long double))
#define SOS_MEMORY_SIZE	16384

static char sos_memory[SOS_MEMORY_SIZE];
static char *sos_memp = sos_memory;
static size_t pg_size = 0;

void *
sos_alloc (size_t size)
{
  char *mem;

#ifdef HAVE_CMPXCHG
  char *old_mem;

  size = (size + MAX_ALIGN - 1) & -MAX_ALIGN;
  do
    {
      old_mem = sos_memp;

      mem = (char *) (((unsigned long) old_mem + MAX_ALIGN - 1) & -MAX_ALIGN);
      mem += size;
      if (mem >= sos_memory + sizeof (sos_memory))
	abort ();
    }
  while (cmpxchg_ptr (&sos_memp, old_mem, mem) != old_mem);
#else
  static pthread_mutex_t sos_lock = PTHREAD_MUTEX_INITIALIZER;
  sigset_t saved_sigmask;

  size = (size + MAX_ALIGN - 1) & -MAX_ALIGN;

  sigprocmask (SIG_SETMASK, &unwi_full_sigmask, &saved_sigmask);
  mutex_lock(&sos_lock);
  {
    mem = (char *) (((unsigned long) sos_memp + MAX_ALIGN - 1) & -MAX_ALIGN);
    mem += size;
    if (mem >= sos_memory + sizeof (sos_memory))
      abort ();
    sos_memp = mem;
  }
  mutex_unlock(&sos_lock);
  sigprocmask (SIG_SETMASK, &saved_sigmask, NULL);
#endif
  return mem;
}

static void *
alloc_memory (size_t size)
{
  /* Hopefully, mmap() goes straight through to a system call stub...  */
  void *mem = mmap (0, size, PROT_READ | PROT_WRITE,
		    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mem == MAP_FAILED)
    return NULL;

  return mem;
}

/* Must be called while holding the mempool lock. */

static void
free_object (struct mempool *pool, void *object)
{
  struct object *obj = object;

  obj->next = pool->free_list;
  pool->free_list = obj;
  ++pool->num_free;
}

static void
add_memory (struct mempool *pool, char *mem, size_t size, size_t obj_size)
{
  char *obj;

  for (obj = mem; obj + obj_size <= mem + size; obj += obj_size)
    free_object (pool, obj);
}

static void
expand (struct mempool *pool)
{
  size_t size;
  char *mem;

  size = pool->chunk_size;
  mem = alloc_memory (size);
  if (!mem)
    {
      size = (pool->obj_size + pg_size - 1) & -pg_size;
      mem = alloc_memory (size);
      if (!mem)
	{
	  /* last chance: try to allocate one object from the SOS memory */
	  size = pool->obj_size;
	  mem = sos_alloc (size);
	}
    }
  add_memory (pool, mem, size, pool->obj_size);
}

void
mempool_init (struct mempool *pool, size_t obj_size, size_t reserve)
{
  if (pg_size == 0)
    pg_size = getpagesize ();

  memset (pool, 0, sizeof (*pool));

  mutex_init (&pool->lock);

  /* round object-size up to integer multiple of MAX_ALIGN */
  obj_size = (obj_size + MAX_ALIGN - 1) & -MAX_ALIGN;

  if (!reserve)
    {
      reserve = pg_size / obj_size / 2;
      if (!reserve)
	reserve = 16;
    }

  pool->obj_size = obj_size;
  pool->reserve = reserve;
  pool->chunk_size = (2*reserve*obj_size + pg_size - 1) & -pg_size;

  expand (pool);
}

void *
mempool_alloc (struct mempool *pool)
{
  sigset_t saved_sigmask;
  struct object *obj;

  sigprocmask (SIG_SETMASK, &unwi_full_sigmask, &saved_sigmask);
  mutex_lock(&pool->lock);
  {
    if (pool->num_free <= pool->reserve)
      expand (pool);

    assert (pool->num_free > 0);

    --pool->num_free;
    obj = pool->free_list;
    pool->free_list = obj->next;
  }
  mutex_unlock(&pool->lock);
  sigprocmask (SIG_SETMASK, &saved_sigmask, NULL);
  return obj;
}

void
mempool_free (struct mempool *pool, void *object)
{
  sigset_t saved_sigmask;

  sigprocmask (SIG_SETMASK, &unwi_full_sigmask, &saved_sigmask);
  mutex_lock(&pool->lock);
  {
    free_object (pool, object);
  }
  mutex_unlock(&pool->lock);
  sigprocmask (SIG_SETMASK, &saved_sigmask, NULL);
}

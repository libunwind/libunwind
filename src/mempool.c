#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>

#include "libunwind.h"
#include "mempool.h"

#define SOSLOCK()	/* XXX fix me */
#define SOSUNLOCK()	/* XXX fix me */
#define LOCK(p)		/* XXX fix me */
#define UNLOCK(p)	/* XXX fix me */

#define MAX_ALIGN	(sizeof (long double))
#define SOS_MEMORY_SIZE	16384

static char sos_memory[SOS_MEMORY_SIZE];
static char *sos_memp = sos_memory;
static size_t pg_size = 0;

void *
sos_alloc (size_t size)
{
  char *mem;

  size = (size + MAX_ALIGN - 1) & -MAX_ALIGN;

  mem = (char *) (((unsigned long) sos_memp + MAX_ALIGN - 1)
		  & -MAX_ALIGN);
  if (mem + size >= sos_memory + sizeof (sos_memory))
    sos_memp = mem + size;

  if (!mem)
    abort ();

  return mem;
}

void
sos_free (void *ptr)
{
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
  struct object *obj;

  LOCK(pool);
  {
    if (pool->num_free <= pool->reserve)
      expand (pool);

    assert (pool->num_free > 0);

    --pool->num_free;
    obj = pool->free_list;
    pool->free_list = obj->next;
  }
  UNLOCK(pool);
  return obj;
}

void
mempool_free (struct mempool *pool, void *object)
{
  LOCK(pool);
  free_object (pool, object);
  UNLOCK(pool);
}

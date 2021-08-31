/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef os_solaris_h
#define os_solaris_h

#include <stdio.h>
#include <string.h>
#include <libproc.h>
#include <limits.h>

typedef struct
{
  prmap_t    md_map;
  char    *md_objname;
} mapdata_t;

static  mapdata_t  *maps;
static  int    map_count;
static  int    map_alloc;

static mapdata_t *
nextmap(void)
{
  if (map_count == map_alloc)
  {
    int next;
    if (map_alloc == 0)
      next = 16;
    else
      next = map_alloc * 2;

    mapdata_t *newmaps = realloc(maps, next * sizeof (mapdata_t));
    if (newmaps == NULL)
      return NULL;

    memset(newmaps + map_alloc, '\0', (next - map_alloc) * sizeof (mapdata_t));

    map_alloc = next;
    maps = newmaps;
  }

  return &maps[map_count++];
}

static int
gather_map(void *ignored, const prmap_t *map, const char *objname)
{
  mapdata_t *data = nextmap();
  if (data == NULL)
    return 1;

  data->md_map = *map;
  data->md_objname = objname ? strdup(objname) : NULL;
  return 0;
}

static char *
get_elf_path(pid_t pid, unw_word_t ip)
{
  char *path = NULL;
  char pid_str[32];
  snprintf(pid_str, sizeof(pid_str), "%ld", (long)pid);
  int perr;
  struct ps_prochandle *Pr = proc_arg_grab(pid_str, PR_ARG_PIDS, PGRAB_RDONLY, &perr);
  if ((Pr == NULL) || (Pstatus(Pr)->pr_flags & PR_ISSYS))
    goto GET_FILE_PATH_END;

  int mapfd = -1;
  char buf[PATH_MAX];
  snprintf(buf, sizeof (buf), "/proc/%s/map", pid_str);
  if ((mapfd = open(buf, O_RDONLY)) < 0)
    goto GET_FILE_PATH_END;

  maps = NULL;
  map_count = 0;
  map_alloc = 0;

  if (Pmapping_iter_resolved(Pr, gather_map, NULL) != 0)
    goto GET_FILE_PATH_END;

  for (int i = 0; i < map_count; i++)
  {
    if ((maps[i].md_objname != NULL) &&
      (ip >= maps[i].md_map.pr_vaddr) &&
      (ip < maps[i].md_map.pr_vaddr + maps[i].md_map.pr_size))
    {
      path = strdup(maps[i].md_objname);
      break;
    }
  }
  
GET_FILE_PATH_END:
  if (Pr != NULL)
    Prelease(Pr, 0);
  if (mapfd != -1)
    close(mapfd);
  for (int i = 0; i < map_count; i++)
  {
    if (maps[i].md_objname != NULL)
      free(maps[i].md_objname);
  }
  free(maps);
  maps = NULL;
  map_count = 0;
  map_alloc = 0;

  return path;
}

#endif /* os_solaris_h */

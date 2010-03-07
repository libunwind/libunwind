/* libunwind - a platform-independent unwind library
   Copyright (C) 2010 Konstantin Belousov

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

#ifndef UNW_REMOTE_ONLY

#include <sys/types.h>
#include <sys/user.h>
#include <libutil.h>
#include <stdio.h>

#include "libunwind_i.h"

int
tdep_get_elf_image (struct elf_image *ei, pid_t pid, unw_word_t ip,
		    unsigned long *segbase, unsigned long *mapoff)
{
	struct kinfo_vmentry *freep, *kve;
	int cnt, rc, i;

	freep = kinfo_getvmmap(pid, &cnt);
	if (freep == NULL)
		return (-1);
	for (i = 0; i < cnt; i++) {
		kve = &freep[i];
		if (ip < kve->kve_start || ip >= kve->kve_end)
			continue;
		if (kve->kve_type != KVME_TYPE_VNODE) {
			free(freep);
			return (-1);
		}
		*segbase = kve->kve_start;
		*mapoff = kve->kve_offset;
		rc = elf_map_image(ei, kve->kve_path);
		free(freep);
		return (rc);
	}
	return (-1);
}

#endif /* UNW_REMOTE_ONLY */

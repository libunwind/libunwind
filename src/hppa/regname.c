/* libunwind - a platform-independent unwind library
   Copyright (c) 2004-2005 Hewlett-Packard Development Company, L.P.
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

#include "unwind_i.h"

#ifdef __LP64__
static const char *regname[] =
  {
    "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
    "r8",  "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
    "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
    "fr4",  "fr5",  "fr6",  "fr7",
    "fr8",  "fr9", "fr10", "fr11",
    "fr12", "fr13", "fr14", "fr15",
    "fr16", "fr17", "fr18", "fr19",
    "fr20", "fr21", "fr22", "fr23",
    "fr24", "fr25", "fr26", "fr27",
    "fr28", "fr29", "fr30", "fr31",
    "sar"
    "ip",
    "eh0", "eh1", "eh2", "eh3",
    "cfa"
  };
#else
static const char *regname[] =
  {
    "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
    "r8",  "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
    "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
    "fr4",  "fr4R",  "fr5",  "fr5R",  "fr6",  "fr6R",  "fr7",  "fr7R",
    "fr8",  "fr8R",  "fr9",  "fr9R",  "fr10", "fr10R", "fr11", "fr11R",
    "fr12", "fr12R", "fr13", "fr13R", "fr14", "fr14R", "fr15", "fr15R",
    "fr16", "fr16R", "fr17", "fr17R", "fr18", "fr18R", "fr19", "fr19R",
    "fr20", "fr20R", "fr21", "fr21R", "fr22", "fr22R", "fr23", "fr23R",
    "fr24", "fr24R", "fr25", "fr25R", "fr26", "fr26R", "fr27", "fr27R",
    "fr28", "fr28R", "fr29", "fr29R", "fr30", "fr30R", "fr31", "fr31R",
    "sar"
    "ip",
    "eh0", "eh1", "eh2", "eh3",
    "cfa"
  };
#endif

const char *
unw_regname (unw_regnum_t reg)
{
  if (reg < (unw_regnum_t) ARRAY_SIZE (regname))
    return regname[reg];
  else
    return "???";
}

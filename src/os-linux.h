/* libunwind - a platform-independent unwind library
   Copyright (C) 2003-2004 Hewlett-Packard Co
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

#ifndef os_linux_h
#define os_linux_h

struct map_iterator
  {
    off_t offset;
    int fd;
    size_t buf_size;
    char *buf;
    char *buf_end;
  };

static inline char *
ltoa (char *buf, long val)
{
  char *cp = buf, tmp;
  ssize_t i, len;

  do
    {
      *cp++ = '0' + (val % 10);
      val /= 10;
    }
  while (val);

  /* reverse the order of the digits: */
  len = cp - buf;
  --cp;
  for (i = 0; i < len / 2; ++i)
    {
      tmp = buf[i];
      buf[i] = cp[-i];
      cp[-i] = tmp;
    }
  return buf + len;
}

static inline void
maps_init (struct map_iterator *mi, pid_t pid)
{
  char path[PATH_MAX], *cp;

  memcpy (path, "/proc/", 6);
  cp = ltoa (path + 6, pid);
  memcpy (cp, "/maps", 6);

  mi->fd = open (path, O_RDONLY);
  mi->offset = 0;

  cp = NULL;
  if (mi->fd >= 0)
    {
      /* Try to allocate a page-sized buffer.  If that fails, we'll
	 fall back on reading one line at a time.  */
      mi->buf_size = getpagesize ();
      cp = mmap (0, mi->buf_size, PROT_READ | PROT_WRITE,
		 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (cp == MAP_FAILED)
	cp = NULL;
      else
	cp += mi->buf_size;
    }
  mi->buf = mi->buf_end = cp;
}

static inline char *
skip_whitespace (char *cp)
{
  if (!cp)
    return NULL;

  while (*cp == ' ' || *cp == '\t')
    ++cp;
  return cp;
}

static inline char *
scan_hex (char *cp, unsigned long *valp)
{
  unsigned long num_digits = 0, digit, val = 0;

  cp = skip_whitespace (cp);
  if (!cp)
    return NULL;

  while (1)
    {
      digit = *cp;
      if ((digit - '0') <= 9)
	digit -= '0';
      else if ((digit - 'a') < 6)
	digit -= 'a' - 10;
      else if ((digit - 'A') < 6)
	digit -= 'A' - 10;
      else
	break;
      val = (val << 4) | digit;
      ++num_digits;
      ++cp;
    }
  if (!num_digits)
    return NULL;
  *valp = val;
  return cp;
}

static inline char *
scan_dec (char *cp, unsigned long *valp)
{
  unsigned long num_digits = 0, digit, val = 0;

  if (!(cp = skip_whitespace (cp)))
    return NULL;

  while (1)
    {
      digit = *cp;
      if ((digit - '0') <= 9)
	{
	  digit -= '0';
	  ++cp;
	}
      else
	break;
      val = (10 * val) + digit;
      ++num_digits;
    }
  if (!num_digits)
    return NULL;
  *valp = val;
  return cp;
}

static inline char *
scan_char (char *cp, char *valp)
{
  if (!cp)
    return NULL;

  *valp = *cp;

  /* don't step over NUL terminator */
  if (*cp)
    ++cp;
  return cp;
}

/* Scan a string delimited by white-space.  Fails on empty string or
   if string is doesn't fit in the specified buffer.  */
static inline char *
scan_string (char *cp, char *valp, size_t buf_size)
{
  size_t i = 0;

  if (!(cp = skip_whitespace (cp)))
    return NULL;

  while (*cp != ' ' && *cp != '\t' && *cp != '\0')
    {
      if (i < buf_size - 1)
	valp[i++] = *cp;
      ++cp;
    }
  if (i == 0 || i >= buf_size)
    return NULL;
  valp[i] = '\0';
  return cp;
}

static inline int
maps_next (struct map_iterator *mi,
	   unsigned long *low, unsigned long *high, unsigned long *offset,
	   char *path, size_t path_size)
{
  char line[256 + PATH_MAX], perm[16], dash, colon, *cp;
  unsigned long major, minor, inum;
  size_t to_read = 256;	/* most lines fit in 256 characters easy */
  ssize_t i, nread;

  if (mi->fd < 0)
    return 0;

  while (1)
    {
      if (mi->buf)
	{
	  ssize_t bytes_left = mi->buf_end - mi->buf;
	  char *eol = NULL;

	  for (i = 0; i < bytes_left; ++i)
	    {
	      if (mi->buf[i] == '\n')
		{
		  eol = mi->buf + i;
		  break;
		}
	      else if (mi->buf[i] == '\0')
		break;
	    }
	  if (!eol)
	    {
	      /* copy down the remaining bytes, if any */
	      if (bytes_left > 0)
		memcpy (mi->buf_end - mi->buf_size, mi->buf, bytes_left);

	      mi->buf = mi->buf_end - mi->buf_size;
	      nread = read (mi->fd, mi->buf + bytes_left,
			    mi->buf_size - bytes_left);
	      if (nread <= 0)
		return 0;

	      eol = mi->buf + bytes_left + nread - 1;

	      for (i = bytes_left; i < bytes_left + nread; ++i)
		if (mi->buf[i] == '\n')
		  {
		    eol = mi->buf + i;
		    break;
		  }
	    }
	  cp = mi->buf;
	  mi->buf = eol + 1;
	  *eol = '\0';
	}
      else
	{
	  /* maps_init() wasn't able to allocate a buffer; do it the
	     slow way.  */
	  lseek (mi->fd, mi->offset, SEEK_SET);

	  if ((nread = read (mi->fd, line, to_read)) <= 0)
	    return 0;
	  for (i = 0; i < nread && line[i] != '\n'; ++i)
	    /* skip */;
	  if (i < nread)
	    {
	      line[i] = '\0';
	      mi->offset += i + 1;
	    }
	  else
	    {
	      if (to_read < sizeof (line))
		to_read = sizeof (line) - 1;
	      else
		mi->offset += nread;	/* not supposed to happen... */
	      continue;	/* duh, no newline found */
	    }
	  cp = line;
	}

      /* scan: "LOW-HIGH PERM OFFSET MAJOR:MINOR INUM PATH" */
      cp = scan_hex (cp, low);
      cp = scan_char (cp, &dash);
      cp = scan_hex (cp, high);
      cp = scan_string (cp, perm, sizeof (perm));
      cp = scan_hex (cp, offset);
      cp = scan_hex (cp, &major);
      cp = scan_char (cp, &colon);
      cp = scan_hex (cp, &minor);
      cp = scan_dec (cp, &inum);
      cp = scan_string (cp, path, path_size);
      if (!cp || dash != '-' || colon != ':')
	continue;	/* skip line with unknown or bad format */
      return 1;
    }
  return 0;
}

static inline void
maps_close (struct map_iterator *mi)
{
  if (mi->fd < 0)
    return;
  close (mi->fd);
  mi->fd = -1;
  if (mi->buf)
    {
      munmap (mi->buf_end - mi->buf_size, mi->buf_size);
      mi->buf = mi->buf_end = 0;
    }
}

#endif /* os_linux_h */

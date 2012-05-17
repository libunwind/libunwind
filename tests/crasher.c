/* This program should crash and produce coredump */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

void a(void) __attribute__((noinline));
void b(int x) __attribute__((noinline));

#ifdef __linux__
void write_maps(char *fname)
{
    char buf[512], path[128];
    char exec;
    uintmax_t addr;
    FILE *maps = fopen("/proc/self/maps", "r");
    FILE *out = fopen(fname, "w");

    if (!maps || !out)
        exit(EXIT_FAILURE);

    while (fgets(buf, sizeof(buf), maps))
    {
        if (sscanf(buf, "%jx-%*jx %*c%*c%c%*c %*x %*s %*d /%126[^\n]", &addr, &exec, path+1) != 3)
            continue;

        if (exec != 'x')
            continue;

        path[0] = '/';
        fprintf(out, "0x%jx:%s ", addr, path);
    }
    fprintf(out, "\n");

    fclose(out);
    fclose(maps);
}
#else
#error Port me
#endif

void a(void)
{
  *(int *)NULL = 42;
}

void b(int x)
{
  if (x)
    a();
  else
    b(1);
}

int
main (int argc, char **argv)
{
  if (argc > 1)
      write_maps(argv[1]);
  b(0);
  return 0;
}


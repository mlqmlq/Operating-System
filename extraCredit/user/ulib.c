#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"

#define PGSIZE 4096
#define MAX_THREADS 64

struct pair {
    void *allocated;
    void *stack;
};

static struct pair threads[MAX_THREADS];


char*
strcpy(char *s, char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  stosb(dst, c, n);
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, void *vsrc, int n)
{
  char *dst, *src;
  
  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
}

int
thread_create(void (*start_routine)(void*, void*), void *arg1, void *arg2)
{
  void *allocated, *stack;
  int i = 0;

  allocated = malloc(2*PGSIZE);
  if (allocated == 0) {
    return -1;
  }
  stack = (void *)(((uint)allocated + PGSIZE-1) & ~(PGSIZE-1));

  while (threads[i].allocated != 0) {
    ++i;
  }
  threads[i].allocated = allocated;
  threads[i].stack = stack;

  return clone(start_routine, arg1, arg2, stack);
}

int
thread_join(void)
{
  void *allocated, *stack;
  int pid;
  int i = 0;

  pid = join(&stack);
  if (pid < 0) {
    return -1;
  }

  while (threads[i].stack != stack) {
    ++i;
  }
  allocated = threads[i].allocated;
  threads[i].allocated = 0;
  threads[i].stack = 0;

  free(allocated);

  return pid;
}


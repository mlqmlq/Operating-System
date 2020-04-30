
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "spinlock.h"
#include "rand.h"
//#include "time.h"

struct run {
  struct run *next;
  //int pg_idx;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  int num_free;
} kmem;

struct {
  struct spinlock lock;
  char *inuse_arr[4000];
  int num_alloc;
} inuse;

extern char end[]; // first address after kernel loaded from ELF file

// Initialize free list of physical pages.
void
kinit(void)
{
  char *p;
 
  //srand(time(0));
  initlock(&kmem.lock, "kmem");
  initlock(&inuse.lock, "inuse");
  inuse.num_alloc = 0;
  kmem.num_free = 0;
  p = (char*)PGROUNDUP((uint)end);
  for(; p + PGSIZE <= (char*)PHYSTOP; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || (uint)v >= PHYSTOP) 
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(v, 1, PGSIZE);

  acquire(&kmem.lock);
  r = (struct run*)v;
  r->next = kmem.freelist;
  kmem.freelist = r;
  kmem.num_free++;
  release(&kmem.lock);
  
  acquire(&inuse.lock);
  char **arr = inuse.inuse_arr;
  int i;
  for (i = 0; arr[i] != 0; i++)
      if (arr[i] == v){
          inuse.num_alloc--;
          break;
      }
  for (int j = i; arr[j] != 0; j++)
      arr[j] = arr[j+1];
  release(&inuse.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r;
  struct run *curr;
  struct run *prev;

  int rand = xv6_rand()%kmem.num_free;
  acquire(&kmem.lock);
  if (!kmem.freelist)
      ;
  else if (rand == 0){
      r = kmem.freelist;
      kmem.freelist = kmem.freelist->next; 
      kmem.num_free--;
  } else {
      prev = kmem.freelist;
      curr = prev->next;
      for (int i = 0; i < rand-1; i++){
          prev = curr;
          curr = curr->next;
      }
      prev->next = curr->next;
      r = curr;
      kmem.num_free--;
  }

  release(&kmem.lock);
  
  acquire(&inuse.lock);
  if (r) {
      cprintf("%x\n", (uint)r);
      inuse.inuse_arr[inuse.num_alloc] = (char*)r;
      inuse.num_alloc++;
  }
  release(&inuse.lock);
  return (char*)r;
}

int 
dump_allocated(int* frames, int numframes){
    acquire(&inuse.lock);
    char **arr = inuse.inuse_arr;
    int j = 0;
    for (int i = inuse.num_alloc-1; i >= 0; i--){
        frames[j] = (uint)arr[i];
        j++;
    }   

    release(&inuse.lock);
    if (j < numframes)
        return -1;
    return 0;
}

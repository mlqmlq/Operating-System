
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "spinlock.h"


struct run {
  struct run *next;
  //int pg_idx;
};

struct {
  struct spinlock lock;
  struct run *freelist;
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

  initlock(&kmem.lock, "kmem");
  initlock(&inuse.lock, "inuse");
  inuse.num_alloc = 0;
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

  r = (struct run*)v;
 
  struct run *curr;
  acquire(&kmem.lock);
  curr = kmem.freelist;
  if (!curr || (char*)curr < v){
      r->next = kmem.freelist;
      kmem.freelist = r;
  } else {
      while (curr->next && (char*)curr->next > v){
          curr = curr->next;
      }
      r->next = curr->next; 
      curr->next = r;
  }
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

  acquire(&kmem.lock);
  struct run *curr = kmem.freelist;
  struct run *prev = curr;
  if (!curr || !curr->next)
      ;
  else{
      curr = prev->next;
      if ((char*)prev == (char*)(PHYSTOP-PGSIZE)
          && (char*)curr == (char*)(PHYSTOP-2*PGSIZE)){
          kmem.freelist = curr;
          r = prev;
      } else {
          while (curr->next) {
              if (((char*)prev - (char*)curr == PGSIZE) &&
                  ((char*)curr - (char*)curr->next == PGSIZE))
                  break;
              prev = curr;
              curr = curr->next;
          }
          if (curr->next){
              prev->next = curr->next;
              r = curr;
          } else if ((char*)curr == (char*)PGROUNDUP((uint)end) 
              && (char*)prev == (char*)PGROUNDUP((uint)end) + PGSIZE){
              prev->next = curr->next;
              r = curr;
          }else {
              ;
          }
      }
  }

  release(&kmem.lock);
  
  acquire(&inuse.lock);
  if (r) {
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
    for (int i = 0; i < numframes; i++){
        if (!arr[i]){
            release(&inuse.lock);
            return -1;
        }
        frames[i] = (uint)arr[i];
    }   
    release(&inuse.lock);
    return 0;
}

#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "sysfunc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since boot.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_clone(void)
{
  void(*fcn)(void*, void*);
  int arg1;
  int arg2;
  void *stack;

  if (argptr(0, (void *)&fcn, 0) < 0
    || argint(1, &arg1) < 0
    || argint(2, &arg2) < 0
    || argptr(3, (void *)&stack, PGSIZE) < 0
  ) {
    return -1;
  }

  if(((uint)stack % PGSIZE) != 0 || proc->sz < (uint)stack + PGSIZE)
        return -1;
  //if (((uint)stack & (PGSIZE-1)) != 0) {
  //  return -1;
  //}

  return clone(fcn, (void *)arg1, (void *)arg2, stack);
}

int sys_join(void)
{
  void **stack;

  if (argptr(0, (void *)&stack, sizeof(*stack)) < 0) {
    return -1;
  }

  return join(stack);
}

int sys_sem_init(void) {
	int *sem_id;
	int value;
	if (argptr(0, (void *)&sem_id, sizeof(*sem_id)) < 0)
	  return -1;
	if (argint(1, &value) < 0)
	  return -1;

	return sem_init(sem_id, value);
}

int sys_sem_destroy(void) {
  int sem_id;
  if (argint(0, &sem_id) < 0)
    return -1;
  return sem_destroy(sem_id);
}

int sys_sem_wait(void) {
  int sem_id;
  if (argint(0, &sem_id) < 0)
    return -1;
  return sem_wait(sem_id);
}

int sys_sem_post(void) {
  int sem_id;
  if (argint(0, &sem_id) < 0)
    return -1;
  return sem_post(sem_id);
}







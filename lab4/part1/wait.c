// wait.c file

extern PROC *running;
extern PROC *sleepList;

int kexit()  // SIMPLE kexit() for process to terminate
{
  printf("proc %d exit\n", running->pid);
  running->status = ZOMBIE;
  tswitch();
}

int ksleep(int event)
{
  int SR = int_off();
  printf("", running->pid);  
  running->event = event;
  running->status = SLEEP;
  enqueue(&sleepList, running);
  tswitch();
  int_on(SR);
}

int kwakeup(int event)
{
  int SR = int_off();
  PROC *p, *tmp=0;
  // if no event specified just wake everyone up
  if(!event){
    while((p = dequeue(&sleepList))!=0){
      p->status = READY;
      enqueue(&readyQueue,p);
    }
    sleepList = tmp;
  }
  else{
    while((p = dequeue(&sleepList))!=0){
      if (p->event==event){
	p->status = READY;
	enqueue(&readyQueue, p);
      }
      else{
	enqueue(&tmp, p);
      }
    }
    sleepList = tmp;
  }
  int_on(SR);
}


// wait.c file

extern PROC *running;
extern PROC *sleepList;
extern PROC *childList;

int kwait(int *status){
  if(!running->child){
    printf("!!!!!!!!CALLER HAS NO CHILD!!!!!!!!\n",running->pid);
    return -1;
  }

  PROC *temp = running->child;

  while(1){
    // if zombie child free it
    if(temp->status == 4){
      //put exitValue into status
      *status = temp->exitCode;

      // if the zombie child is the first child in childList link sibling to be the new first child in list
      if(temp == running->child){
	running->child = temp->sibling;
      }
      else{
	//zombie child not first in list 
	PROC *currChild = running->child;
	PROC *prevChild = running->child;
	
	while(currChild != temp){
	  prevChild = currChild;
	  currChild = currChild->sibling;
	}

	prevChild->sibling = currChild->sibling;
      }
      temp->parent = 0;
      temp->child = 0;
      temp->sibling = 0;
      temp->status = 0;
      temp->priority = 0;
      enqueue(&freeList,temp);
      return temp->pid;
    }
    ksleep((int)running);
  }  
}

int kexit(int exitValue)  // SIMPLE kexit() for process to terminate
{
  //Never let P1 exit!
  if(running->pid == 1){
    printf("!!!!!!!!!YOU CANNOT EXIT P1!!!!!!!!!\n",running->pid);
    return 0;
  }
  //If current process has children send them to P1!!!!!
  PROC *childrens = running->child;
  while(childrens){
    childrens->parent = &proc[1];
    childrens->ppid = 1;
    childrens = childrens->sibling;
  }
  
  PROC *papaP1House = &proc[1];
  papaP1House = papaP1House->child;
  
  while(papaP1House->sibling){
    papaP1House = papaP1House->sibling;
  }
  papaP1House->sibling = running->child;

  //RECORD EXIT VALUE 
  running->exitCode = exitValue;
  //BECOME ZOMBIE
  running->status = 4;
  //WAKE UP PARENT
  kwakeup((int)running->parent);
  //SWITCH PROCESS GIVE UP CPU
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

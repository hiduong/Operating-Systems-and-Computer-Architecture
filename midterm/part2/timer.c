// timer register u32 offsets from base address
#define TLOAD 0x0
#define TVALUE 0x1
#define TCNTL 0x2
#define TINTCLR 0x3
#define TRIS 0x4
#define TMIS 0x5
#define TBGLOAD 0x6

extern int color;
extern PROC *running;
int hold = 0;
int body();

typedef volatile struct timer{
  u32 *base; // timer's base address; as u32 pointer
  int tick, hh, mm, ss; // per timer data area
  char clock[16];
}TIMER;

typedef struct tqe {
  struct tqe *next;
  PROC *proc;
  int time;
  int action;
}TQE;

TQE tqe[9],*timerFreeList,*timerReadyQueue;
volatile TIMER timer; //4 timers; 2 per unit; at 0x00 and 0x20


int tqe_enqueue(TQE **queue, TQE *p)
{
  TQE *q  = *queue;
  if (q==0 || p->time < q->time){
    *queue = p;
     q->time = q->time - p->time;
    p->next = q;
    return;
  }
  while (q->next && p->time >= q->next->time){
    q = q->next;
  }
  p->next = q->next;
  q->next = p;
  p->time = p->time - q->time;
}

TQE *tqe_dequeue(TQE **queue)
{
  TQE *p = *queue;
  if (p)
    *queue = p->next;
  return p;
}

int printQueueList(TQE *p)
{
  printf("readyQueue=");
  while(p){
    printf("[%d,%d]->", p->proc->pid, p->time);
    p = p->next;
  }
  printf("NULL\n");
}


int mystrcpy(char *dest, char *src)
{
  while(*src){
    *dest++ = *src++;
  }
  *dest = 0;
}

void timer_init()
{
  int i; TIMER *tp;
  TQE *tp2;
  kprintf("timer_init()\n");
  for (i=0; i<4; i++){
    tp = &timer;
    tp->base = (u32 *)0x101E2000;
    *(tp->base+TLOAD) = 0x0; // reset
    *(tp->base+TVALUE)= 0xFFFFFFFF;
    *(tp->base+TRIS) = 0x0;
    *(tp->base+TMIS) = 0x0;
    *(tp->base+TLOAD) = 0x100;
    // CntlReg=011-0010=|En|Pe|IntE|-|scal=01|32bit|0=wrap|=0x66
    *(tp->base+TCNTL) = 0x66;
    *(tp->base+TBGLOAD) = 0x1C00; // timer counter value
    tp->tick = tp->hh = tp->mm = tp->ss = 0; // initialize wall clock
    mystrcpy((char *)tp->clock, "00:00:00");
  }

  for (i = 0; i < 9; i++){
    tp2 = &tqe[i];
    tp2->next = tp2 + 1;
    tp2->proc = running;
    tp2->time = 0;
    tp2->action = 0;
  }
  timerFreeList = &tqe[0];
  timerReadyQueue = 0;
  
}

void timer_handler() {
  int i;
  TIMER *t = &timer;
  t->tick++;
  TQE *tp = timerReadyQueue;
  
  if(t->tick == 10){
    if(timerReadyQueue != 0){
      if(hold == 1){
	printQueueList(timerReadyQueue);
	printsleepList(sleepList);
	tp->time--;
	hold = 0;
      }
	if(tp->time == 0){
	  printQueueList(timerReadyQueue);
	  TQE * wake = tqe_dequeue(&timerReadyQueue);
	  printf("wakeup %d\n",tp->proc->pid);
	  kwakeup(wake->action);
	}
	else {
	  printQueueList(timerReadyQueue);
	  tp->time--;
	}
	tp = tp->next;
    }
    t->tick = 0; t->ss++;
    if (t->ss == 60){
      t->ss = 0; t->mm++;
      if (t->mm == 60){
	t->mm = 0; t->hh++; // no 24 hour roll around
      }
    }
    t->clock[7]='0'+(t->ss%10); t->clock[6]='0'+(t->ss/10);
    t->clock[4]='0'+(t->mm%10); t->clock[3]='0'+(t->mm/10);
    t->clock[1]='0'+(t->hh%10); t->clock[0]='0'+(t->hh/10);
  }
  for (i=0; i<8; i++){
    kpchar(t->clock[i], 0, 70+i); // to line n of LCD
  }
  timer_clearInterrupt(); // clear timer interrupt
}

void timer_start() // timer_start(0), 1, etc.
{
  TIMER *tp = &timer;
  kprintf("timer_start\n");
  *(tp->base+TCNTL) |= 0x80; // set enable bit 7
}

int timer_clearInterrupt() // timer_start(0), 1, etc.
{
  TIMER *tp = &timer;
  *(tp->base+TINTCLR) = 0xFFFFFFFF;
}

void timer_stop() // stop a timer
{
  TIMER *tp = &timer;
  *(tp->base+TCNTL) &= 0x7F; // clear enable bit 7
}

int body()
{
  hold = 1;
  printf("proc %d enter body()\n",running->pid);
  int i = 0;
  TQE * tp = tqe_dequeue(&timerFreeList);
  printf("proc %d running, enter a timer value\n",running->pid);
  i = geti();
  tp->time = i;
  tp->action = (int)&tp;
  tp->proc = running;
  tqe_enqueue(&timerReadyQueue,tp);
  ksleep(tp->action);
}

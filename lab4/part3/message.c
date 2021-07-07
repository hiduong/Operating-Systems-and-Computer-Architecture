#define NMBUF 10

extern PROC *running;
extern PROC proc[NPROC];
extern PROC *readyQueue;

SEMAPHORE nmbuf, mlock;
MBUF mbuf[NMBUF], *mbufList;

int menqueue(MBUF **queue, MBUF *p){
  MBUF *q  = *queue;
  if (q==0){
    *queue = p;
    p->next = 0;
    return;
  }
  if ((*queue)->priority < p->priority){
    p->next = *queue;
    *queue = p;
    return;
  }
  while (q->next && p->priority <= q->next->priority){
    q = q->next;
  }
  p->next = q->next;
  q->next = p;
}

MBUF *mdequeue(MBUF **queue){
  MBUF *p = *queue;
  if (p)
    *queue = p->next;
  return p;
}

int block(SEMAPHORE *s){
  running->status = BLOCK;
  enqueue(&s->queue, running);
  tswitch();
}

int signal(SEMAPHORE *s){
  PROC *p = dequeue(&s->queue);
  p->status = READY;
  enqueue(&readyQueue,p);
}

int P(SEMAPHORE *s)
{
  int SR = int_off();
  s->value--;
  if (s->value < 0){
    block(s);
  }
  int_on(SR);
}

int V(SEMAPHORE *s){
  int SR = int_off();
  s->value++;
  if (s->value <= 0){
    signal(s);
  }
  int_on(SR);
}

int msg_init()
{
  int i; MBUF *mp;
  printf("msg_init()\n");
  mbufList = 0;
  for (i = 0; i<NMBUF; i++){
    menqueue(&mbufList, &mbuf[i]);
  }
  nmbuf.value = NMBUF;
  nmbuf.queue = 0;
  mlock.value = 1;
  mlock.queue = 0;
}

MBUF *get_mbuf()
{
  P(&nmbuf);
  P(&mlock);
  MBUF *mp = mdequeue(&mbufList);
  V(&mlock);
  return mp;
}

int put_mbuf(MBUF *mp)
{
  P(&mlock);
  menqueue(&mbufList,mp);
  V(&mlock);
  V(&nmbuf);
}

int send(char *msg, int pid)
{
  if (pid < 0){
    return -1;
  }
  PROC *p = &proc[pid];
  MBUF *mp = get_mbuf();
  mp->pid = running->pid;
  mp->priority = 1;
  strcpy(mp->contents,msg);
  P(&p->mQlock);
  menqueue(&p->mQueue,mp);
  V(&p->mQlock);
  V(&p->nmsg);
  return 0;
}

int recv(char *msg)
{
  P(&running->nmsg);
  P(&running->mQlock);
  MBUF *mp = mdequeue(&running->mQueue);
  V(&running->mQlock);
  strcpy(msg,mp->contents);
  int sender = mp->pid;
  put_mbuf(mp);
  return sender;
}

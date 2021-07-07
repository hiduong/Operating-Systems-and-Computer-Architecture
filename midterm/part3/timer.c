// timer register u32 offsets from base address
#define TLOAD 0x0
#define TVALUE 0x1
#define TCNTL 0x2
#define TINTCLR 0x3
#define TRIS 0x4
#define TMIS 0x5
#define TBGLOAD 0x6

extern int color;
typedef volatile struct timer{
  u32 *base; // timer's base address; as u32 pointer
  int tick, hh, mm, ss; // per timer data area
  char clock[16];
}TIMER;

volatile TIMER timer; //4 timers; 2 per unit; at 0x00 and 0x20



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

}

void timer_handler() {
  int i;
  TIMER *t = &timer;
  t->tick++;
   
  if(t->tick == 10){
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

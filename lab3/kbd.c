/******** kbd.c file ******************/
#include "keymap"

/******** KBD register byte offsets; for char *base *****/
#define KCNTL 0x00 // 7-6- 5(0=AT)4=RxIntEn 3=TxIntEn
#define KSTAT 0x04 // 7-6=TxE 5=TxBusy 4=RXFull 3=RxBusy
#define KDATA 0x08 // data register;
#define KCLK 0x0C // clock divisor register; (not used)
#define KISTA 0x10 // interrupt status register;(not used)


typedef volatile struct kbd{ // base = 0x10006000
  char *base; // base address of KBD, as char *
  char buf[128]; // input buffer
  int head, tail, data, room; // control variables
}KBD;

extern int color;

int hold = 0;
int shift = 0;
int ctrl = 0;

volatile KBD kbd; // KBD data structure

int kbd_init()
{
  KBD *kp = &kbd;
  kp->base = (char *)0x10006000;
  *(kp->base+KCNTL) = 0x14; // 00010100=INTenable, Enable
  *(kp->base+KCLK) = 8; // PL051 manual says a value 0 to 15
  kp->data = 0; kp->room = 128; // counters
  kp->head = kp->tail = 0; // index to buffer
}

//left shift make code = 0x2A , left shift break code = 0xAA
//right shift make code = 0x36, right shift break code = 0xB6
//left ctrl make code = 0x1D, left ctrl break code = 0x9D

void kbd_handler() // KBD interrupt handler in C
{
  u8 scode, c;
  int i;
  KBD *kp = &kbd;
  color = RED; // int color in vid.c file
  scode = *(kp->base+KDATA); // read scan code in data register
  
  //ignores key releases
  if(scode & 0x80){
    //left shift break code
    if(scode == 0xAA || scode == 0xB6){
      hold = 0;
      shift = 0;
      kprintf("Shift Released\n");
    }
    //left ctrl break code
    if(scode == 0x9D){
      hold = 0;
      ctrl = 0;
      kprintf("CTRL Released\n");
    }
    return;
  }

  if (scode == 0x2A || scode == 0x36){
    hold = 1;
    shift = 1;
    kprintf("Shift Pressed\n");
  }

  if(scode == 0x1D){
    hold = 1;
    ctrl = 1;
    kprintf("CTRL Pressed\n");
  }

  if(hold == 1 && ctrl == 1 && scode == 0x2E){
    kprintf("Control-C key\n");
  }

  if(hold == 1 && shift == 1){
    c = sh[scode];
  }
  else{
    if(hold == 1 && ctrl == 1 && scode == 0x2E){
      c = 0x0;
    }else{
      c = unsh[scode];
    }
  }

  if(hold == 1 && ctrl == 1 && scode == 0x20){
    c = 0x4;
  }
 
  if (c != 0x0){
    if (c != '\r'){
      kprintf("kbd interrupt: c=%x %c\n", c, c);
    }
   kp->buf[kp->head++] = c; // enter key into CIRCULAR buf[ ]
   kp->head %= 128;
   kp->data++; kp->room--; // update counters
  }
}




int kgetc() // main program calls kgetc() to return a char
{
  char c;
  KBD *kp = &kbd;
  unlock(); // enable IRQ interrupts
  while(kp->data <= 0); // wait for data; READONLY
  lock(); // disable IRQ interrupts
  c = kp->buf[kp->tail++];// get a c and update tail index
  kp->tail %= 128;
  kp->data--; kp->room++; // update with interrupts OFF
  unlock(); // enable IRQ interrupts
  return c;
}

int kgets(char s[ ]) // get a string from KBD
{
  char c;
  while((c=kgetc()) != '\r'){
    *s++ = c;
  }
  *s = 0;
  return strlen(s);
}

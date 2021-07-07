/********************************************************************
Copyright 2010-2017 K.C. Wang, <kwang@eecs.wsu.edu>
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************/
/**************************
 need to catch Control-C, Contorl-D keys
 so need to recognize LCTRL key pressed and then C or D key
******************************/

#include "keymap"
#include "keymap2"

#define KCNTL 0x00
#define KSTAT 0x04
#define KDATA 0x08
#define KCLK  0x0C
#define KISTA 0x10

extern int kputc(char);

typedef volatile struct kbd{
  char *base;
  char buf[128];
  int head, tail;
  int data, room;
}KBD;

volatile KBD kbd;
int release = 0;

volatile int keyset;

int kbd_init()
{
  char scode;
  keyset = 1; // default to scan code set-1
  
  KBD *kp = &kbd;
  kp->base = (char *)0x10006000;
  *(kp->base + KCNTL) = 0x10; // bit4=Enable bit0=INT on
  *(kp->base + KCLK)  = 8;
  kp->head = kp->tail = 0;
  kp->data = 0; kp->room = 128;
  
  release = 0;

  printf("Detect KBD scan code: press the ENTER key : ");
  while( (*(kp->base + KSTAT) & 0x10) == 0);
  scode = *(kp->base + KDATA);
  printf("scode=%x ", scode);
  if (scode==0x5A)
    keyset=2;
  printf("keyset=%d\n", keyset);
}

// kbd_handler1() for scan code set 1
void kbd_handler1()
{
  u8 scode, c;
  KBD *kp = &kbd;
  scode = *(kp->base + KDATA);
  if (scode & 0x80)
    return;

  c = unsh[scode];

  if (c == 'r')
     kputc('\n');
  kputc(c);

  kp->buf[kp->head++] = c;
  kp->head %= 128;
  kp->data++; kp->room--;

}

// kbd_handelr2() for scan code set 2
void kbd_handler2()
{
  u8 scode, c;
  KBD *kp = &kbd;

  scode = *(kp->base + KDATA);

  if (scode == 0xF0){ // key release 
    release = 1;
    return;
  }
  
  if (release){ // 2nd interrupt of release
    release = 0;
    return;
  }

  c = ltab[scode];
  if (c=='\r')
    kputc('\n');
  kputc(c);
  
  kp->buf[kp->head++] = c;
  kp->head %= 128;
  kp->data++; kp->room--;
  
}

void kbd_handler()
{
  if (keyset == 1)
    kbd_handler1();
  else
    kbd_handler2();
}

int kgetc()
{
  char c;
  KBD *kp = &kbd;
  
  while(kp->data == 0);

  lock();
  c = kp->buf[kp->tail++];
  kp->tail %= 128;
  kp->data--; kp->room++;
  unlock();
  return c;
}

int kgets(char s[ ])
{
  char c;
  while( (c = kgetc()) != '\r'){
    if (c=='\b'){
      s--;
      continue;
    }
    *s++ = c;
  }
  *s = 0;
  return strlen(s);
}

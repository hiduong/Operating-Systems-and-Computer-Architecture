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
#include "defines.h"
#include "type.h"

char *tab = "0123456789ABCDEF";

int kprintf(char *fmt, ...);
int kputc(char);

#include "string.c"
#include "queue.c"
#include "vid.c"

#include "kbd.c"
#include "uart.c"
#include "pipe.c"
#include "exceptions.c"
#include "kernel.c"


PIPE *kpipe;

void copy_vectors(void) {
    extern u32 vectors_start;
    extern u32 vectors_end;
    u32 *vectors_src = &vectors_start;
    u32 *vectors_dst = (u32 *)0;

    while(vectors_src < &vectors_end)
       *vectors_dst++ = *vectors_src++;
}

// IRQ interrupts handler entry point
void IRQ_handler()
{
    int vicstatus, sicstatus;

    // read VIC SIC status reg to find out which interrupt
    vicstatus = VIC_STATUS;
    sicstatus = SIC_STATUS;  

    if (vicstatus & (1<<31)){ // SIC interrupts=bit_31=>KBD at bit 3 
      if (sicstatus & (1<<3)){
          kbd_handler();
       }
    }
}

int pipe_writer()
{
  char c, *cp; 
  struct uart *up = &uart[0];
  char line[128];
  int i;

  while(1){
    uprintf("Enter a line for task1 to get : ");
    ugets(up, line);
    uprints(up, "\r\n");
    printf("proc%d writes %d line=[%s] to pipe %d\n", running->pid, i, line);
    write_pipe(kpipe, line, strlen(line));
  }
  //kpipe->nwriter--;
  //uprintf("pipe writer proc %d exit\n", running->pid);
  //kexit();
}

int pipe_reader()
{
  char c, *cp; 
  char line[128];
  int  i, j, n;

  for (i=0; i<2; i++){
    printf("proc%d read from pipe %d\n", running->pid, i);
    n = read_pipe(kpipe, line, 20);
    printf("proc%d read n=%d bytes from pipe : [", running->pid, n);
    for (j=0; j<n; j++)
      kputc(line[j]);
    kprintf("]\n");
  }
  printf("pipe reader proc%d exit\n", running->pid);
  kpipe->nreader--;
  kexit();
}
  
int main()
{ 
   int i; 
   char line[128]; 
   u8 kbdstatus, key, scode;
   KBD *kp = &kbd;
   color = WHITE;
   row = col = 0; 

   fbuf_init();
   kprintf("Welcome to Wanix in ARM\n");

   kbd_init();   
   /* enable timer0,1, uart0,1 SIC interrupts */
   VIC_INTENABLE = 0;
   VIC_INTENABLE |= (1<<31); // SIC to VIC's IRQ31
   
   /* enable KBD IRQ */
   SIC_INTENABLE = (1<<3); // KBD int=bit3 on SIC
   SIC_ENSET = (1<<3);  // KBD int=3 on SIC
   *(kp->base+KCNTL) = 0x12;

   uart_init();

   pipe_init();
   kpipe = create_pipe();   

   init();

   kprintf("P0 kfork tasks: ");
   kfork(pipe_writer, 1);
   kfork(pipe_reader, 1);

   printQ(readyQueue);

   unlock();
   while(1){
     if (readyQueue)
        tswitch();
   }
}

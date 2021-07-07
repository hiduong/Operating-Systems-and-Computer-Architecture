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
int color;

#include "type.h"
#include "string.c"
#include "uart.c"
#include "vid.c"
#include "kbd.c"
#include "timer.c"
#include "exceptions.c"

int mkptable()
{
  int i, pentry, *ptable;
  printf("1. build level-1 pgtable at 16 KB\n");
  ptable = (int *)0x4000;
  for (i = 0; i< 8192; i++){
    ptable[i] = 0;
  }
  printf("2. fill 258 entries off pgtable to ID map 258 MB VA to PA\n");
  pentry = 0x412;
  for (i = 0; i < 258; i++){
    ptable[i] = pentry;
    pentry += 0x100000;
  }
  printf("3. finished building level-1 page table\n");
  printf("4. return to set TTB, domain and enabble MMU\n");
}

int data_chandler()
{
  u32 fault_status, fault_addr, domain, status;
  int spsr = get_spsr();
  printf("data_abort exception in ");
  if ((spsr & 0x1F) == 0x13){
    printf("SVC mode\n");
  }
  fault_status = get_fault_status();
  fault_addr = get_fault_addr();
  domain = (fault_status & 0xF0) >> 4;
  status = fault_status & 0xF;
  printf("status = %x: domain = %x status = %x (0x5 = Trans Invalid)\n",fault_status,domain,status);
  printf("VA addr = %x\n",fault_addr);
}


int copy_vectors_table() {
  extern u32 vectors_start;
  extern u32 vectors_end;
  u32 *vectors_src = &vectors_start;
  u32 *vectors_dst = (u32 *)0;

  while(vectors_src < &vectors_end)
    *vectors_dst++ = *vectors_src++;
}

int kprintf(char *fmt, ...);
int irq_chandler()
{
    int vicstatus, sicstatus;
    vicstatus = VIC_STATUS; // VIC_STATUS=0x10140000=status reg
    sicstatus = SIC_STATUS;  

    if(vicstatus & (1 << 4)){
      timer_handler();
    }
    if(vicstatus & (1 << 12)){
      uart_handler(&uart[0]);
    }
    if (vicstatus & (1 << 31)){
      if (sicstatus & (1 << 3)){
          kbd_handler();
       }
    }
}

int main()
{ 
  int i, *p;
   char line[128]; 
   //   u8 kbdstatus, key, scode;

   color = WHITE;
   row = col = 0; 

   kbd_init();
   uart_init();
   
   // allow KBD interrupts   
   VIC_INTENABLE |= (1<<4); // allow VIC IRQ31
   VIC_INTENABLE |= (1<<12); // allow VIC IRQ31
   VIC_INTENABLE = 1<<31; // allow VIC IRQ31

   UART0_IMSC = 1 << 4;
   
   // enable KBD IRQ 
   SIC_ENSET = 1<<3;     // KBD int=3 on SIC
   SIC_PICENSET = 1<<3;  // KBD int=3 on SIC

   //kbd->control = 1 << 4;
   timer_init(); timer_start();
   
   printf("test MM at VA=2MB\n");
   p = (int *)(2*0x100000); *p = 123;
		
   printf("test MM at VA=127MB\n");
   p = (int *)(127*0x100000); *p = 123;
		
   printf("test MM at VA=128MB\n");
   *p = (int *)(128*0x100000); *p = 123;
		
   printf("test MM at VA=512MB\n");
   *p = (int *)(512*0x100000); *p = 123;
   
   while(1){
     printf("main runnning Input a line: ");
     kgets(line);
     printf(" line = %s\n", line);
   }
}

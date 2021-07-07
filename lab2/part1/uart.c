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
#define DR   0x00
#define FR   0x18

#define RXFE 0x10
#define TXFF 0x20

typedef struct uart{
  char *base;
  int n;
}UART;

UART uart[4];

typedef unsigned int u32;

//char *tab = "0123456789ABCDEF";

int uart_init()
{
  int i; UART *up;

  for (i=0; i<4; i++){
    up = &uart[i];
    up->base = (char *)(0x101F1000 + i*0x1000);
    up->n = i;
  }
  uart[3].base = (char *)(0x10009000); // uart3 at 0x10009000
}

int ugetc(UART *up)
{
  while (*(up->base + FR) & RXFE);
  return *(up->base + DR);
}

int uputc(UART *up, char c)
{
  while(*(up->base + FR) & TXFF);
  *(up->base + DR) = c;
}

int ugets(UART *up, char *s)
{
  while ((*s = (char)ugetc(up)) != '\r'){
    uputc(up, *s);
    s++;
  }
 *s = 0;
}

int uprints(UART *up, char *s)
{
  while(*s)
    uputc(up, *s++);
}


int urpu(UART *up, u32 x){
  char c;
  if(x)
    {
      c = tab[x % 10];
      urpu(up,x / 10);
    }
  uputc(up,c);
}

int urpu_hex(UART *up, u32 x)
{
  char c;
  if(x)
    {
      c = tab[x % 16];
      urpu_hex(up,x / 16);
    }
    uputc(up,c);
}

int uprintu(UART *up,u32 x)
{
  (x==0)?uputc(up,'0'):urpu(up,x);
}

int uprintx(UART *up, u32 x)
{
  uputc(up,'0');
  uputc(up,'x');
  if(x == 0){
    uputc(up,'0');
  }
  else{
    urpu_hex(up,x);
  }
  uputc(up,' ');
}

int uprintd(UART *up,int x)
{
  if (x < 0)
    {
      uputc(up,'-');
      uprintu(up,-x);
    }
  else
    {
      uprintu(up,x);
    }
}

/** WRITE YOUR uprintf(UART *up, char *fmt, . . .) for formatted print **/
int uprintf(UART *up, char *fmt,...)
{
  char *cp = fmt; //cp points at the format string
  int *ip = (int *)&fmt+1;  // ip points to first item to be printed on stack
  while(*cp != '\0')
    {
      if(*cp == '\n')
	{
	  uputc(up,'\n');uputc(up,'\r');
	}
      else if(*cp == '\t')
	{
	  uputc(up,'\t');
	}
      else if(*cp == '%')
	{
	  cp = cp + 1; // will get character after %
	  if(*cp == 'c')
	    { uputc(up,*ip);}
	  else if(*cp == 's')
	    {
	      uprints(up,(char *)*ip);
	    }
	  else if(*cp == 'u')
	    {
	      uprintu(up,*ip);
	    }
	  else if(*cp == 'd')
	    {
	      uprintd(up,*ip);
	    }
	  else if(*cp == 'x')
	    {
	      uprintx(up,*ip);
	    }
	  ip++;
	}
      else
	{
	  uputc(up,*cp);
	}

      cp++;
    }
}

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

#define NPIPE 10

#define PSIZE 8
typedef struct pipe{
  char buf[PSIZE];
  int head, tail, data, room;
  int nreader, nwriter;
  int busy;
}PIPE;

PIPE pipe[NPIPE];
extern PROC *running;

int pipe_init()
{
  int i;
  printf("pipe_init()\n");
  for (i=0; i<NPIPE; i++){
    pipe[i].busy = 0;
  }
}

PIPE *create_pipe()
{
  int i; PIPE *p;
  printf("creating pipe ");
  for (i=0; i<NPIPE; i++){
     p = &pipe[i];
     if (p->busy==0){
        p->busy = 1;
        break;
     }
  }
  p->head = p->tail = 0;
  p->data = 0;  p->room = PSIZE;

  printf("OK\n");
  return p;
}

int read_pipe(PIPE *p, char *buf, int n)
{
  int r = 0;

  if(n<=0){
    return 0;
  }
  
  while(n){
    while(p->data){
      *buf = p->buf[p->tail++];
      p->tail %= PSIZE;
      p->data--; p->room++; buf++; r++; n--;
      if (n==0)
	break;
    }

    kwakeup((int)&p->room);
	
    if (r){
      return r;
    }

    ksleep((int)&p->data);
  }
}

int write_pipe(PIPE *p, char *buf, int n)
{
  int r = 0; int w = 0;
    
  while(n){
    if(!p->nreader){
      uprintf("no more readers: BROKEN PIPE\n",running->pid);
      printf("no more readers: BROKEN PIPE\n",running->pid);
      kexit();
    }
    w = 0;
    while (p->room){
      p->buf[p->head++] = *buf;
      p->head %= PSIZE;
      p->data++; p->room--; buf++; w++; r++; n--;
      if (n==0)
	break;
    }
    printf("proc%d wrote %d bytes\n", running->pid, w);
    kwakeup((int)&p->data);
    if (n==0){
      printf("proc%d finished writing %d bytes\n", running->pid, r);
      return r;
    }
    ksleep((int)&p->room);
  }
}


  

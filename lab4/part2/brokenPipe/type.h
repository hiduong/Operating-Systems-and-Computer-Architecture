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
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

#define BLUE   0
#define GREEN  1
#define RED    2
#define CYAN   3
#define YELLOW 4
#define PURPLE 5
#define WHITE  6

#define  SSIZE 1024

#define  FREE   0
#define  READY  1
#define  SLEEP  2
#define  BLOCK  3
#define  ZOMBIE 4
#define  PAUSE  5
#define  printf  kprintf
 
typedef struct proc{
  struct proc *next;
  int    *ksp;
  
  int    pid;
  int    ppid;
  int    priority;
  int    status;
  int    event;
  int    exitCode;

  struct proc *parent;
  struct proc *child;
  struct proc *sibling;

  int    kstack[SSIZE];
}PROC;

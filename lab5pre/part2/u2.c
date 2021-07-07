typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned int    u32;

#include "string.c"
#include "uio.c"
#include "ucode.c"

int main()
{
  int pid, ppid;
  char line[64];
  u32 mode,  *up;

  mode = getcpsr();
  mode = mode & 0x1F;
  printf("CPU MODE=%x\n", mode);  // verify we are in USER mode

  pid = getpid();
  ppid = getppid();
  
  while(1){
    printf("THIS IS PROCESS %d IN UMODE PARENT=%d\n", pid, ppid);
    umenu();
    printf("INPUT A COMMAND : ");
    ugets(line); 
    printf("\n"); 
 
    if (strcmp(line, "GETPID")==0)
       ugetpid();
    if (strcmp(line, "GETPPID")==0)
       ugetppid();
    if (strcmp(line, "PD")==0)
       ups();
    if (strcmp(line, "CHNAME")==0)
       uchname();
    if (strcmp(line, "SWITCH")==0)
       uswitch();
  }
}


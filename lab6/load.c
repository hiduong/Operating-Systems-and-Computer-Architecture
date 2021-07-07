
char buf1[1024], buf2[1024];

u16 search(INODE *ip, char *name)
{
  int i; char c; DIR *dp;
  for (i = 0; i < 12; i++){
    if((u16)ip->i_block[i]){
      getblock((u16)ip->i_block[i],buf2);
      dp = (DIR *)buf2;
      while((char *)dp < &buf2[1024]){
	c = dp->name[dp->name_len];
	dp->name[dp->name_len] = 0;
	printf("%s ",dp->name);
	if(strcmp(dp->name,name)==0){
	  printf("\n\r");
	  return ((u16)dp->inode);
	}
	dp->name[dp->name_len] = c;
	dp = (char *)dp + dp->rec_len;
      }
    }
  }
}

int load(char *filename, PROC *p)
{ 
  char *name[2];
  u16 i, ino, blk, iblk;
  u32 *up;
  GD *gp;
  INODE *ip;
  DIR *dp;

  name[0] = "bin";
  name[1] = filename;
  char *pagetable = (char *) (0x800000 + (p->pid - 1) *0x100000);

  getblock(2,buf1);
  gp = (GD *)buf1;
  iblk = (u16)gp->bg_inode_table;
  getblock(iblk,buf1);
  ip = (INODE *)buf1 + 1;
  for(i = 0; i < 2; i++){
    ino = search(ip,name[i]) -1;
    if(ino < 0) error();
    getblock(iblk + (ino/8),buf1);
    ip = (INODE *)buf1 + (ino % 8);
  }

  if((u16)ip->i_block[12]){
    getblock((u16)ip->i_block[12],buf2);   
  }

  for(i = 0; i < 12; i++){
    if(ip->i_block[i] == 0){
      break;
    }
    getblock(ip->i_block[i],pagetable);
    pagetable += 1024;
  }

  if (ip->i_block[12]){
    up = (u32 *)buf2;
    while(*up){
      getblock(*up,pagetable);
      up++;
      pagetable += 1024;
    }
  }

  return 1;
}   

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <ncp/nwcalls.h>
#include "private/libintl.h"
#include <netinet/in.h>

typedef unsigned int __u32;

#define MAXIDS 1000
struct nwuid_mapping_table
{  uid_t uid;
   __u32 object_id;
} mymap[MAXIDS+1];

struct ncp_setuidmapping_ioctl
{  struct nwuid_mapping_table *map;
};

#define NCP_IOC_SETUIDMAPPING           _IOR('n', 12,struct ncp_setuidmapping_ioctl)

int main(int argc,char *argv[])
{  struct ncp_conn *conn;
   int fd,r,count;
   long err;
   struct ncp_setuidmapping_ioctl arg;
   __u32 nwid;
   uid_t uid;

   for(count=0;count<MAXIDS && scanf("%x %d",&nwid,&uid)==2;count++)
   {  mymap[count].object_id=htonl(nwid);
      mymap[count].uid=uid;
      /*printf("%08x -> %d\n",mymap[count].object_id,mymap[count].uid);*/
   }
   mymap[count].object_id=0;

   if((conn=ncp_initialize_2(&argc,argv,1,NCP_BINDERY_USER,&err,0))==NULL)
   {  char *path;
      if(argc==2)
         path=argv[1];
      else
         path=".";
      if(NWParsePath(path,NULL,&conn,NULL,NULL) || !conn)
      {  fprintf(stderr,"%s is not on ncpfs filesystem\n",path);
         return 2;
      }
   }

   if((fd=ncp_get_fid(conn))==-1)
   {  fprintf(stderr,"Can't get connection handle\n");
      return 2;
   }

   arg.map=mymap;

   r=ioctl(fd,NCP_IOC_SETUIDMAPPING,&arg);

   if(r==-1)
      perror("ncpfs-nwuid-test in NCP_IOC_SETUIDMAPPING ioctl");

   close(fd);

   return 0;
}

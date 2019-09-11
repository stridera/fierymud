#include "../conf.h"
#include "../sysdep.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

enum errlist
{
  BAD_ARGS,BAD_HOST,NO_IDENT,SOCK_ERR
};

void
usage(error)
enum errlist error;
{
  fprintf(stderr,"ident-scan: ");
  switch(error)
  {
    case BAD_ARGS: fprintf(stderr,"usage: ident-scan hostname [low port] [hi port]\n");
                   break;
    case BAD_HOST: fprintf(stderr,"error: cant resolve hostname\n");
                   break;
    case NO_IDENT: fprintf(stderr,"error: ident isnt running on host\n");
                   break;
    case SOCK_ERR: fprintf(stderr,"error: socket() failed\n");
                   break;
  }
  exit(-1);
}

struct hostent *
fill_host(machine,host)
char *machine;
struct hostent *host;
{

  if ((host=gethostbyname(machine))==NULL)
  {
     if ((host=gethostbyaddr(machine,4,AF_INET))==NULL)
        return(host);
  }
  return(host);
}

int
main(argc,argv)
int argc;
char **argv;
{
  struct sockaddr_in forconnect,forport,forident;
  int i,sockfd,identfd,len=sizeof(forport),hiport=9999,loport=1,curport;
  struct servent *service;
  struct hostent *host;
  char identbuf[15], recieved[85], *uid;

  if ((argc<2) || (argc>4))
    usage(BAD_ARGS);
  if (argc>2)
     loport=atoi(argv[2]);
  if (argc>3)
     hiport=atoi(argv[3]);
  if ((host=fill_host(argv[1],host))==NULL)
    usage(BAD_HOST);
  forconnect.sin_family=host->h_addrtype;
  forconnect.sin_addr.s_addr=*((long *)host->h_addr);
  forident.sin_family=host->h_addrtype;
  forident.sin_addr.s_addr=*((long *)host->h_addr);
  forident.sin_port=htons(113);

  if ((identfd=socket(AF_INET,SOCK_STREAM,0))== -1)
     usage(SOCK_ERR);
  if ((connect(identfd,(struct sockaddr *)&forident,sizeof(forident)))!=0)
     usage(NO_IDENT);
  close(identfd);

  for(curport=loport;curport<=hiport;curport++)
  {
     for(i=0;i!=85;i++)
        recieved[i]='\0';
     forconnect.sin_port=htons(curport);
     if ((sockfd=socket(AF_INET,SOCK_STREAM,0))== -1)
        usage(SOCK_ERR);


     if (connect(sockfd,(struct sockaddr *)&forconnect,sizeof(forconnect))==0)
     {
       if (getsockname(sockfd,(struct sockaddr *)&forport,&len)==0)
       {
          if ((identfd=socket(AF_INET,SOCK_STREAM,0))== -1)
             usage(SOCK_ERR);
          if (connect(identfd,(struct sockaddr *)&forident,sizeof(forident))==0)
          {
             sprintf(identbuf,"%u,%u",htons(forconnect.sin_port),
                htons(forport.sin_port));

             write(identfd,identbuf,strlen(identbuf)+1);
             read(identfd,recieved,80);
             recieved[strlen(recieved)-1]='\0';
             uid=strrchr(recieved,' ');
             service=getservbyport(forconnect.sin_port,"tcp");
             printf("Port: %3d\tService: %10s\tUserid: %s\n",curport,
                (service==NULL)?"(?)":service->s_name,uid);
          }
       }
    }
    close(sockfd);
    close(identfd);
  }
}

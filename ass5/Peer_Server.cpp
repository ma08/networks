#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <utility>      // std::pair, std::make_pair
#include <string>       // std::string
#include <iostream>     // std::cout
#include <vector>     // std::cout
#include <netinet/in.h>
#include <netdb.h> 
#include <map>

#define MAXSIZE 100
#define CHUNKSIZE 32
using namespace std;

void SigCatcher(int n)
{
    pid_t kidpid=waitpid(-1,NULL,WNOHANG);
    printf("\nChild %d terminated\n",kidpid);
}

void handle_socket(int sock){
  int n,nread,f;
  char buffer[256];
  bzero(buffer,256);
  n=read(sock,buffer,50);
  if(n<0)perror("ERROR reading from socket");
  printf("\n\n%s\n\n",buffer);
  if(buffer[0]=='R'&&buffer[1]=='E'){
    if((f=open(buffer+8,O_RDONLY))>=0){
      strcpy(buffer,"SUCCESS");
      fflush(stdout);
      n=write(sock,buffer,20);
      if(n<0)perror("ERROR writing to socket");
      while((nread=read(f,buffer,CHUNKSIZE))>0){
        fflush(stdout);
        n=write(sock,buffer,nread);
        if(n<0)
          perror("Can't write");
      }
      printf("\nSuccesfully Uploaded\n");
    }
    else{
      strcpy(buffer,"FAIL");
      printf("\n\n%s",buffer);
      fflush(stdout);
      n=write(sock,buffer,20);
      if(n<0)perror("ERROR writing to socket");
    }
  }
  close(sock);
}

int main(int argc, char *argv[]) {
  int sockfd,newsockfd,portno;
  int server_here_port=10011;
  int num_files;
  pid_t pid;
  socklen_t clilen;
  struct sockaddr_in cli_addr;
  socklen_t len;
  int nbytes;
  char buffer[MAXSIZE];
  char buffer_2[MAXSIZE];
  char buffer_3[MAXSIZE];
  struct hostent *server;
  struct sockaddr_in serv_addr,serv_addr_stream;
  if(argc<3){
    printf("\nPlease specify address and port\n");
    exit(1);
  }
  portno=atoi(argv[2]);
  sockfd=socket(AF_INET, SOCK_DGRAM, 0);
  if(sockfd<0) 
    perror("ERROR opening socket");
  server=gethostbyname(argv[1]);
  if(server==NULL){
    printf("\nERROR, no such host\n");
    exit(0);
  }
  bzero((char*)&serv_addr,sizeof(serv_addr));
  serv_addr.sin_family=AF_INET;
  bcopy((char*)server->h_addr,(char*)&serv_addr.sin_addr.s_addr,server->h_length);
  serv_addr.sin_port=htons(portno);
  len=sizeof(serv_addr);
  printf("\nEnter number of files you would like to share: ");
  scanf("%d",&num_files);
  buffer_2[0]='\0';
  while(num_files--){
    strcat(buffer_2,"\n");
    printf("\nEnter file name: ");
    scanf("%s",buffer);
    strcat(buffer_2,buffer);
  }
  strcpy(buffer,"SHARE ");
  int sockfd_stream=socket(AF_INET,SOCK_STREAM,0);
  if(sockfd_stream<0) 
    perror("ERROR opening socket");
  bzero((char*)&serv_addr_stream,sizeof(serv_addr_stream));
  serv_addr_stream.sin_family=AF_INET;
  serv_addr_stream.sin_addr.s_addr=INADDR_ANY;
  serv_addr_stream.sin_port=htons(server_here_port);
  while (bind(sockfd_stream,(struct sockaddr*)&serv_addr_stream,sizeof(serv_addr_stream))<0){
    printf("\nERROR on binding to %d",server_here_port);
    server_here_port=rand()%5001+10001;
    serv_addr_stream.sin_port=htons(server_here_port);
  }
  printf("\nUsing port %d for socket stream",server_here_port);
  sprintf(buffer_3,"%d ",server_here_port);
  strcat(buffer,buffer_3);
  strcat(buffer,buffer_2);
  if((nbytes=sendto(sockfd,buffer,MAXSIZE,0,(struct sockaddr*)&serv_addr,len))<0){
    perror("Can't send");
  }
  nbytes=recvfrom(sockfd,buffer_2,MAXSIZE,0,(struct sockaddr*)&serv_addr,&len);
  if(nbytes<0){
    perror("no Ack");
  }else
    printf("\nGot ack: %s\n",buffer_2);
  listen(sockfd_stream,5);
  clilen=sizeof(cli_addr);
  while(1){
    fflush(stdout);
    newsockfd=accept(sockfd_stream,(struct sockaddr*)&cli_addr,&clilen);
    fflush(stdout);
    if(newsockfd<0) 
      perror("ERROR on accept");
    else{
      fflush(stdout);
      pid=fork();
      if(pid<0)
        perror("ERROR on fork");
      if(pid==0){
        close(sockfd_stream);
        fflush(stdout);
        handle_socket(newsockfd);
        exit(0);
       }
      else{
        signal(SIGCHLD,SigCatcher);
        close(newsockfd);
      }
    }
  } 
  close(sockfd);
}

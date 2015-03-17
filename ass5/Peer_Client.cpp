#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
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

#define CHUNKSIZE 32
#define MAXSIZE 100

using namespace std;

int dostuff()
{
    int sockfd_stream,portno_stream,n;
    struct sockaddr_in serv_addr_stream;
    struct hostent *server_stream;
    char buffer_stream[256];
    portno_stream=10011;
    sockfd_stream=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd_stream <0)
        perror("ERROR opening socket");
    server_stream=gethostbyname("localhost");
    if(server_stream==NULL){
        fprintf(stderr,"\nERROR, no such host\n");
        exit(0);
    }
    bzero((char*)&serv_addr_stream,sizeof(serv_addr_stream));
    serv_addr_stream.sin_family=AF_INET;
    bcopy((char*)server_stream->h_addr,(char*)&serv_addr_stream.sin_addr.s_addr,server_stream->h_length);
    serv_addr_stream.sin_port=htons(portno_stream);
    while(connect(sockfd_stream,(struct sockaddr*)&serv_addr_stream,sizeof(serv_addr_stream))<0)
      ;
    return 0;
}
 
int main(int argc, char *argv[]) {
  int sockfd,portno;
  socklen_t len;
  char *token;
  int nbytes;
  char buffer[MAXSIZE];
  char buffer_2[MAXSIZE];
  char buffer_3[MAXSIZE];
  struct hostent *server;
  struct sockaddr_in serv_addr;
  if(argc<3){
    printf("\nPlease specify address and port\n");
    exit(1);
  }
  portno=atoi(argv[2]);
  sockfd=socket(AF_INET,SOCK_DGRAM,0);
  if(sockfd<0)
    perror("ERROR opening socket");
  server=gethostbyname(argv[1]);
  if(server==NULL){
    printf("ERROR, no such host\n");
    exit(0);
  }
  bzero((char*)&serv_addr,sizeof(serv_addr));
  serv_addr.sin_family=AF_INET;
  bcopy((char*)server->h_addr,(char*)&serv_addr.sin_addr.s_addr,server->h_length);
  serv_addr.sin_port=htons(portno);
  len=sizeof(serv_addr);
  while(1){
    printf("\nEnter filename to download: ");
    scanf("%s",buffer_2);
    strcpy(buffer,"REQUEST ");
    strcat(buffer,buffer_2);
    strcpy(buffer_3,buffer);
    if((nbytes=sendto(sockfd,buffer,MAXSIZE,0,(struct sockaddr*)&serv_addr,len))<0){
      perror("Can't send");
    }
    nbytes=recvfrom(sockfd,buffer_2,MAXSIZE,0,(struct sockaddr*)&serv_addr,&len);
   printf("\nGot ack: %s\n",buffer_2);
    if(nbytes>0){
      if(buffer_2[0]=='F'&&buffer_2[1]=='A'){
        printf("\n File not available\n");
      }
      else{
        token=strtok(buffer_2+8," ");
        int sockfd_stream, portno_stream, n;
        struct sockaddr_in serv_addr_stream;
        struct hostent *server_stream;
        char buffer_stream[256];
        portno_stream=10011;
        sockfd_stream=socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd_stream<0) 
            perror("ERROR opening socket");
        server_stream=gethostbyname(token);
        token=strtok(NULL," ");
        portno_stream=atoi(token);
        if(server_stream==NULL) {
            fprintf(stderr,"\nERROR, no such host\n");
            exit(0);
        }
        bzero((char*)&serv_addr_stream,sizeof(serv_addr_stream));
        serv_addr_stream.sin_family=AF_INET;
        bcopy((char*)server_stream->h_addr,(char*)&serv_addr_stream.sin_addr.s_addr,server_stream->h_length);
        serv_addr_stream.sin_port=htons(portno_stream);
        while(connect(sockfd_stream,(struct sockaddr*)&serv_addr_stream,sizeof(serv_addr_stream))<0) 
          ;
        nbytes=write(sockfd_stream,buffer_3,strlen(buffer_3));
        if(nbytes<0){
          perror("writing: ");
        }
        fflush(stdout);
        nbytes=read(sockfd_stream,buffer,20);
        fflush(stdout);
        if(nbytes<0){
          perror("Can't read");
        }
        if(strcmp(buffer,"SUCCESS")==0){
          fflush(stdout);
          char *filename=basename(buffer_3+8);
          int nwritten,nread;
          printf("\nGive path to save the file:");
          scanf("%s",buffer_3);
          if(buffer_3[strlen(buffer_3)-1]!='/'){
            strcat(buffer_3,"/");
          }
          strcat(buffer_3,filename);
          printf("\n\n%s\n",buffer_3);
          int fd_to=open(buffer_3,O_WRONLY|O_CREAT,0666);
          if(fd_to<0){
            perror("Can't write/create");
          }
          else{
            int i;
            while(nbytes=read(sockfd_stream,buffer,CHUNKSIZE),nbytes>0){
              fflush(stdout);
              char *out_ptr=buffer;
              do{
                nwritten=write(fd_to,out_ptr,nbytes);
                if(nwritten>=0)
                {
                  nbytes-=nwritten;
                  out_ptr+=nwritten;
                }
                else if(errno!=EINTR)
                {
                  if(fd_to>=0)
                    close(fd_to);
                  return -1;
                }
              } while(nbytes>0);
            }
            close(sockfd_stream);
            close(fd_to);
            printf("\nFile successfully downloaded\n");
          }
        }
        else{
          printf("\nCan't read\n");
        }
      }
    }else{
      printf("\nCan't get response from FIS server");
    }
  }
}

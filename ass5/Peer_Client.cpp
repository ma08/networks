#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include<errno.h>
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
    printf("Please specify address and port");
    exit(1);
  }
  portno = atoi(argv[2]);
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    perror("ERROR opening socket");
  server = gethostbyname(argv[1]);
  if (server == NULL) {
    printf("ERROR, no such host\n");
    exit(0);
  }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(portno);
  len=sizeof(serv_addr);
  printf("Enter filename: ");
  scanf("%s",buffer_2);
  strcpy(buffer,"REQUEST ");
  strcat(buffer,buffer_2);
  strcpy(buffer_3,buffer_2);
  if((nbytes = sendto(sockfd, buffer, MAXSIZE, 0, (struct sockaddr *) & serv_addr, len))<0){
    perror("Can't send");
  }
  nbytes = recvfrom (sockfd, buffer_2, MAXSIZE, 0, (struct sockaddr *) & serv_addr, &len);
  printf("Got ack: %s\n",buffer_2);
  if(nbytes>0){
    if(buffer_2[0]=='F'&&buffer_2[1]=='A'){
      printf("\n File not available");
    }
    else{
      token=strtok(buffer_2+8," ");
      printf("\n%s---",token);
      server = gethostbyname(token);
      if (server == NULL) {
        printf("ERROR, no such host\n");
        exit(0);
      }
      bzero((char *) &serv_addr, sizeof(serv_addr));
      bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
      serv_addr.sin_port = htons(10011);
      sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (sockfd < 0) 
        perror("ERROR opening socket");
      while (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("ERROR connecting");
        fflush(stdout);
      }
      bzero(buffer,256);
      printf("\n\nAAA");
      fflush(stdout);
      nbytes = read(sockfd,buffer,20);
      fflush(stdout);
      if(nbytes<0){
        printf("\n\nBBB");
        perror("Can't read");
        /*continue;*/
      }
      if(strcmp(buffer,"SUCCESS")==0){
        printf("\n\nSuccess");
        fflush(stdout);
        char *filename=basename(buffer_3);
        int nwritten,nread;
        printf("\nGive path to save the file:");
        scanf("%s",buffer_3);
        if(buffer_3[strlen(buffer_3)-1]!='/'){
          strcat(buffer_3,"/");
        }
        strcat(buffer_3,filename);

        int fd_to = open(buffer_3, O_WRONLY | O_CREAT , 0666);
        if (fd_to < 0){
          perror("Can't write/create");
        }else{
          while (nbytes = read(sockfd, buffer,CHUNKSIZE ), nbytes > 0){
            printf("\n\n%s\n\n",buffer);
            fflush(stdout);
            char *out_ptr = buffer;
            do {
              nwritten = write(fd_to, out_ptr, nbytes);
              if (nwritten >= 0)
              {
                nbytes -= nwritten;
                out_ptr += nwritten;
              }
              else if (errno != EINTR)
              {
                /*close(fd_from);*/
                if (fd_to >= 0)
                  close(fd_to);
                return -1;
              }
            } while (nbytes > 0);
          }
          close(sockfd);
          close(fd_to);
        }
      }else{
        printf("\nCan't read");
      }
    }
  }
}


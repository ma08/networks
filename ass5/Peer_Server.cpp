#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
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

#define MAXSIZE 100
#define CHUNKSIZE 32

using namespace std;
void handle_socket(int sock){
  int n;
  int nread;
  char buffer[256];
  bzero(buffer,256);
  n = read(sock,buffer,255);
  int f;
  if (n < 0) perror("ERROR reading from socket");
  printf("\n\n%s\n\n",buffer);
  if(buffer[0]=='R'&&buffer[1]=='E'){
    if((f=open(buffer+8,O_RDONLY))<0){
      strcpy(buffer,"SUCCESS");
      n = write(sock,buffer,20);
      while((nread = read(f, buffer, 32))>0){
        printf("\n\n%s",buffer);
        fflush(stdout);
        n = write(sock,buffer,20);
        if(n<0)
          perror("Can't write");
      }
      if (n < 0) perror("ERROR writing to socket");
    }else{
      strcpy(buffer,"FAIL");
      n = write(sock,buffer,20);
      if (n < 0) perror("ERROR writing to socket");
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
  printf("Enter number of files you would like to share: ");
  scanf("%d",&num_files);
  buffer_2[0]='\0';
  while(num_files--){
    printf("Enter file name: ");
    scanf("%s",buffer);
    strcpy(buffer_2,"\n");
    strcpy(buffer_2,buffer);
  }
  strcpy(buffer,"SHARE ");
  strcat(buffer,buffer_2);
  //printf("\n %s",buffer);
  if((nbytes = sendto(sockfd, buffer, MAXSIZE, 0, (struct sockaddr *) & serv_addr, len))<0){
    perror("Can't send");
  }
  nbytes = recvfrom (sockfd, buffer_2, MAXSIZE, 0, (struct sockaddr *) & serv_addr, &len);
  if(nbytes<0){
    perror("no Ack");
  }
  printf("\nGot ack: %s\n",buffer_2);

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
    perror("ERROR opening socket");
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(server_here_port);
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
    perror("ERROR on binding");
   listen(sockfd,5);
   /*perror("woo");*/
   clilen = sizeof(cli_addr);
   while (1) {
     printf("ccccc");
     fflush(stdout);
     newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
     printf("aaaaaaaaaaa");
     fflush(stdout);
     if (newsockfd < 0) 
       perror("ERROR on accept");
     fflush(stdout);
     pid = fork();
     if (pid < 0)
       perror("ERROR on fork");
     if (pid == 0)  {
       close(sockfd);
       printf("bbbbbbb");
       fflush(stdout);
       handle_socket(newsockfd);
       exit(0);
     }else
       close(newsockfd);
   } /* end of while */
   close(sockfd);
}


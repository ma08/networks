#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
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

using namespace std;
int main(int argc, char *argv[]) {
  int sockfd,portno;
  int num_files;
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
  printf("Got ack: %s",buffer_2);

}


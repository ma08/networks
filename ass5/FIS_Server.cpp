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
#include <map>

#define MAXSIZE 100

using namespace std;

typedef pair<string, int> address_pair;
typedef map<string, vector<address_pair> > file_info; 

void print_file_info(file_info &x){
  file_info::iterator it=x.begin();
  int i=0;
  address_pair foo;
  for(;it!=x.end();it++){
    cout<<endl<<it->first;
    /*cout<<" "<<it->second.size();*/
    /*cout<<"foooo";*/
    /*fflush(stdout);*/
    for (i = 0; i < it->second.size(); ++i)
    {
      foo=it->second.at(i);
      cout<<endl<<"\t"<<foo.first<<"  "<<foo.second;
    }
  }
  fflush(stdout);
}


int main(int argc, char *argv[])
{
  file_info files_to_ip = file_info();
  int sockfd,server_port;
  string s;
  socklen_t clilen;
  char buffer[MAXSIZE];
  int nbytes;
  char message[MAXSIZE];
  char *token;
  struct sockaddr_in serv_addr;
  struct sockaddr_in client_addr;
  int len=100;
  char buffer_2[len];
  char buffer_3[len];
  if(argc>1){
    server_port=atoi(argv[1]);
  }else{
    server_port=571719;
  }
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(server_port);
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR on binding");
    exit(1);
  }
  listen(sockfd,5);
  while (1){
    clilen=sizeof(client_addr);
    nbytes = recvfrom (sockfd, message, MAXSIZE, 0, (struct sockaddr *) & client_addr, &clilen);
    if (nbytes < 0){
      perror ("recfrom (server)");
      /*exit (EXIT_FAILURE);*/
    }
    inet_ntop(AF_INET, &(client_addr.sin_addr), buffer_2, len);
    printf("\nMESSAGE RECEIVED: %s",message);
    printf("\n\taddress:%s",buffer_2);
    printf("\n\tport:%d\n",ntohs((client_addr.sin_port)));
    fflush(stdout);
    if(message[0]=='S'&&message[1]=='H'){
      printf("\nNew peer connected to share stuff\n");
      fflush(stdout);
      address_pair foo=make_pair(string(buffer_2),ntohs(client_addr.sin_port));
      file_info::iterator it=files_to_ip.begin();
      token=strtok(message+6,"\n");
      if(token==NULL){
        exit(1);
      }
      s=string(token);
      //cout<<s;
      it=files_to_ip.find(s);
      if(it!=files_to_ip.end()){
        /*it->second.clear();*/
        it->second.push_back(foo);
      }else{
        file_info::iterator it=files_to_ip.begin();
        vector<address_pair> x;
        x.push_back(foo);
        pair<string,vector<address_pair> > bar=make_pair(s,x);
        it=files_to_ip.insert(it,bar);
      }
      while((token=strtok(NULL,"\n"))!=NULL){
        s=string(token);
        it=files_to_ip.find(s);
        if(it!=files_to_ip.end()){
          it->second.push_back(foo);
        }else{
          it=files_to_ip.begin();
          vector<address_pair> x;
          x.push_back(foo);
          pair<string,vector<address_pair> > bar=make_pair(s,x);
          it=files_to_ip.insert(it,bar);

        }
      }
      print_file_info(files_to_ip);
      strcpy(buffer,"Message Received");
    }else if(message[0]=='R'&&message[1]=='E'){
      s=string(message+8);
      //cout<<s;
      file_info::iterator it=files_to_ip.begin();
      it=files_to_ip.find(s);
      if(it!=files_to_ip.end()){
        address_pair bar=it->second.at(it->second.size()-1);
        strcpy(buffer,"SUCCESS ");
        strcpy(buffer_2,bar.first.c_str());
        strcat(buffer_2," ");
        sprintf(buffer_3,"%d",bar.second);
        strcat(buffer_2,buffer_3);
        strcat(buffer,buffer_2);
      }else{
        strcpy(buffer,"FAIL");
      }
    }
    fflush(stdout);
    /*strcpy(buffer,"Message Received");*/
    nbytes = sendto(sockfd, buffer, MAXSIZE, 0, (struct sockaddr *) & client_addr, clilen);
    if (nbytes < 0){
      perror ("sendto (server)");
      /*exit (EXIT_FAILURE);*/
    }
  }
  return 0;
}



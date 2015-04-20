#include <stdio.h>
#include <error.h>
#include <signal.h>
#include <stdlib.h>
#include <algorithm>
#include <netdb.h> 
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <utility>    
#include <string>     
#include <iostream>   
#include <vector>     
#include <map>
#define MAXSIZE 1000
using namespace std;
typedef pair<string, int> address_pair;
typedef pair<address_pair, unsigned long long> address_hash_pair;

bool customSort(const address_hash_pair& p1, const address_hash_pair& p2){
  return p1.second < p2.second;
}
  string host,result; 
  vector<address_hash_pair> node_info;

  int sockfd;
void intHandler(int dummy=0) {
  int i,portno;
  int nbytes;
  int len;
  struct hostent *server;
  struct sockaddr_in serv_addr;
  char buff[100];
  for (i = 0; i < node_info.size(); ++i)
  {
    address_hash_pair x = node_info.at(i);
    server=gethostbyname(x.first.first.c_str());
    sprintf(buff,"%d",x.first.second);
    result=host+string(buff);
    if(server==NULL){
      printf("ERROR, no such host\n");
      exit(0);
    } 
    portno=x.first.second;
    serv_addr.sin_family=AF_INET;
    bcopy((char*)server->h_addr,(char*)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port=htons(portno);
    len=sizeof(serv_addr);
    strcpy(buff,"START");
    printf("\nSending message to Node %d: %s",i,buff);
    if((nbytes=sendto(sockfd,buff,strlen(buff)+1,0,(struct sockaddr*)&serv_addr,len))<0){
      perror("Can't send");
    }
  }
}

unsigned long long oat_hash(const char *p, int len)
{
    
  unsigned long long h = 0;
  int i;
  for (i = 0; i < len; i++)
  {
      h += p[i];
      h += (h << 10);
      h ^= (h >> 6);
  }
  h += (h << 3);
  h ^= (h >> 11);
  h += (h << 15);
  return h;
}

int getNodeForFile(vector<address_hash_pair>& node_info, unsigned long long hash){
  int i=0;
  for (i = 0; i < node_info.size(); ++i)
  {
    if(node_info.at(i).second>=hash)
      return i;
  }
  return 0;
}

int mod(int a, int b)
{
    int r = a % b;
    return r < 0 ? r + b : r;
}

void printSucPred(vector<address_hash_pair>& node_info,int ind, char* buf){
    char *p;
    address_hash_pair pred = node_info.at(mod(ind-1,node_info.size()));
    address_hash_pair suc = node_info.at((ind+1)%node_info.size());
    strcpy(buf,pred.first.first.c_str());
    p=buf+strlen(buf);
    sprintf(p,"$%d$",pred.first.second);
    strcat(buf,suc.first.first.c_str());
    p=buf+strlen(buf);
    sprintf(p,"$%d$",suc.first.second);
}

void printFiles(vector<vector<string> > file_map, int ind, char* buf){
  vector<string> x = file_map.at(ind);
  buf[0]='\0';
  int i;
  for (i = 0; i < x.size(); ++i)
  {
    strcat(buf,x.at(i).c_str());
    strcat(buf,"$");
  }
  buf[strlen(buf)]='\0';
}
int main(int argc, char *argv[])
{
  int portno;
  socklen_t len;
  int nbytes;
  char buffer[MAXSIZE];
  struct hostent *server;
  struct sockaddr_in serv_addr;
  sockfd=socket(AF_INET,SOCK_DGRAM,0);
  vector<vector<string> > file_map;


  if(sockfd<0)
    perror("ERROR opening socket");
  
  bzero((char*)&serv_addr,sizeof(serv_addr));
  unsigned long long hash;
  pair<string,int> node_addr;
  string fname;
  int node_n,file_n,i,j,port;
  cout<<"Enter number of nodes:";
  cin>>node_n;
  char buff[100];
  for (i = 0; i < node_n; ++i)
  {
    cout<<"\nNode "<<i<<" Enter host address and port separated by space:";
    cin>>host>>port;
    node_addr=(make_pair(host,port));
    sprintf(buff,"%d",port);
    result=host+string(buff);
    hash=oat_hash(result.c_str(),result.length());
    node_info.push_back(make_pair(node_addr,hash));
    cout<<"Node "<<i<<" hash:"<<hash;
  }
  sort(node_info.begin(),node_info.end(),customSort);
  for (i = 0; i < node_n; ++i)
  { 
    address_hash_pair x = node_info.at(i);
    cout<<"\n"<<i<<" "<<x.first.first<<":"<<x.first.second<<"   "<<x.second;
  }
  /*cout<<"\nEnter number of files:";
  cin>>file_n;
  for (i = 0; i < node_n; ++i)
  {
    file_map.push_back(vector<string>());
  }
  for (i = 0; i < file_n; ++i)
  {
    cout<<"\nEnter file name: ";
    cin>>fname;
    hash=oat_hash(fname.c_str(),fname.length());
    j=getNodeForFile(node_info,hash);
    file_map.at(j).push_back(fname);
    cout<<hash;
    cout<<"\n"<<fname<<" will be stored at Node "<<j;
  }*/
  for (i = 0; i < node_n; ++i)
  {
    address_hash_pair x = node_info.at(i);
    server=gethostbyname(x.first.first.c_str());
    sprintf(buff,"%d",x.first.second);
    result=host+string(buff);
    if(server==NULL){
      printf("ERROR, no such host\n");
      exit(0);
    } 
    portno=x.first.second;
    serv_addr.sin_family=AF_INET;
    bcopy((char*)server->h_addr,(char*)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port=htons(portno);
    len=sizeof(serv_addr);
    printSucPred(node_info,i,buffer);
    //printFiles(file_map,i,buffer+strlen(buffer));
    printf("\nSending message to Node %d: %s",i,buffer);
    if((nbytes=sendto(sockfd,buffer,strlen(buffer)+1,0,(struct sockaddr*)&serv_addr,len))<0){
      perror("Can't send");
    }
  }
   signal(SIGTSTP, intHandler);
   while(1){}
  
  return 0;
}



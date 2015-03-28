#include <stdio.h>
#include <netdb.h> 
#include <errno.h> 
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <algorithm>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <utility>      // std::pair, std::make_pair
#include <fcntl.h>
#include <string>       // std::string
#include <iostream>     // std::cout
#include <vector>     // std::cout
#include <map>
using namespace std;
#define CHUNKSIZE 32
#define MAXSIZE 1000
typedef pair<string, int> address_pair;
typedef pair<address_pair, unsigned long long> address_hash_pair;

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
unsigned long long getHash(address_pair& x){
  char buf[100];
  strcpy(buf,x.first.c_str());
  char*p=buf+strlen(buf);
  sprintf(p,":%d",x.second);
  return oat_hash(buf,strlen(buf));
}

string recvDatagram(int sockfd,struct sockaddr_in* client_addr=NULL){
  if(client_addr==NULL){
    client_addr=new struct sockaddr_in();
  }
  char message[MAXSIZE];
  socklen_t clilen=sizeof(*client_addr);
  int nbytes=recvfrom(sockfd,message,MAXSIZE,0,(struct sockaddr*)client_addr,&clilen);
  printf("-------------%d--------------",nbytes);
  printf("\n\n%s",message);
  fflush(stdout);
  if(nbytes>0){
    return string(message);
  }
  return string("");
}

class Node
{
public:
  Node ():cur(),pred(),succ(),succ_point(NULL),pred_point(NULL){
  }
  Node (address_pair x,unsigned long long hash):pred(),succ(){
    this->cur=make_pair(x,hash);
  };
  Node* successor(int sockfd){
    if(succ_point!=NULL){
      return this->succ_point;
    }
    this->succ_point=new Node();
    this->succ_point->cur.first=pair<string,int>(this->succ.first);
    this->succ_point->cur.second=this->succ.second;
    string s="GET INFO";
    this->succ_point->sendToMe(s,sockfd);
    s=recvDatagram(sockfd);
    this->succ_point->setInfo(s);
    return this->succ_point;
  }
  void setInfo(string s){
    char buffer[100],message[100],buffer_2[100];
    char *token;
    strcpy(message,s.c_str());
    token=strtok(message,"$");
    strcpy(buffer,token);
    /*printf("\n\t%s",token);*/
    token=strtok(NULL,"$");
    strcpy(buffer_2,token);
    address_pair x=make_pair(string(buffer),atoi(buffer_2));
    this->pred = make_pair(x,getHash(x));
    token=strtok(NULL,"$");
    strcpy(buffer,token);
    token=strtok(NULL,"$");
    strcpy(buffer_2,token);
    address_pair y=make_pair(string(buffer),atoi(buffer_2));
    this->succ = make_pair(y,getHash(y));
  }
  Node* predecessor(int sockfd){
    if(pred_point!=NULL){
      return this->pred_point;
    }
    this->pred_point=new Node();
    this->pred_point->cur.first=pair<string,int>(this->pred.first);
    this->pred_point->cur.second=this->pred.second;
    string s="GET INFO";
    this->pred_point->sendToMe(s,sockfd);
    s=recvDatagram(sockfd);
    this->pred_point->setInfo(s);
    return this->pred_point;
  }
  address_hash_pair cur,pred,succ;
  Node *succ_point=NULL;
  Node *pred_point=NULL;
  int node_is_end,node_is_start;

  int sendToAdr(string s, int sockfd,address_pair x){
    int portno;
    socklen_t len;
    int nbytes;
    char buffer[MAXSIZE];
    struct hostent *server;
    struct sockaddr_in serv_addr;
    strcpy(buffer,s.c_str());
    bzero((char*)&serv_addr,sizeof(serv_addr));
    server=gethostbyname(x.first.c_str());
    portno=x.second;
    serv_addr.sin_family=AF_INET;
    bcopy((char*)server->h_addr,(char*)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port=htons(portno);
    len=sizeof(serv_addr);
    nbytes=sendto(sockfd,buffer,strlen(buffer)+1,0,(struct sockaddr*)&serv_addr,len);
    return nbytes;
  }

  int sendToMe(string s,int sockfd){
    int portno;
    socklen_t len;
    int nbytes;
    char buffer[MAXSIZE];
    struct hostent *server;
    struct sockaddr_in serv_addr;
    strcpy(buffer,s.c_str());
    bzero((char*)&serv_addr,sizeof(serv_addr));
    server=gethostbyname(this->cur.first.first.c_str());
    portno=this->cur.first.second;
    serv_addr.sin_family=AF_INET;
    bcopy((char*)server->h_addr,(char*)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port=htons(portno);
    len=sizeof(serv_addr);
    nbytes=sendto(sockfd,buffer,strlen(buffer)+1,0,(struct sockaddr*)&serv_addr,len);
    return nbytes;
  }

  address_pair find(string s,int sockfd){
      unsigned long long hash=oat_hash(s.c_str(),s.length());
      printf("\n\n---------%s-----%d--------",this->cur.first.first.c_str(),this->cur.first.second);
      fflush(stdout);
      if(hash>cur.second){
      /*if(1){*/
        //query succ(start)
        string re="FILE "+s;
        this->sendToAdr(re,sockfd,this->succ.first);
        re=recvDatagram(sockfd);
        if(re[0]=='S'&&re[1]=='U'){
          string y=re.substr(8);
          int x=atoi(y.c_str());
          return address_pair(this->succ.first.first,x);
        }
        cout<<"\n\n----"<<re;
        printf("\n\nQuerying---------%s-----%d--------",this->succ.first.first.c_str(),this->succ.first.second);
        fflush(stdout);
        return this->successor(sockfd)->find(s,sockfd);
        return succ.first;
      }else{
        //query pred
        string re="FILE "+s;
        this->sendToAdr(re,sockfd,this->pred.first);
        re=recvDatagram(sockfd);
        if(re[0]=='S'&&re[1]=='U'){
          string y=re.substr(8);
          int x=atoi(y.c_str());
          return address_pair(this->pred.first.first,x);
        }
        cout<<"\n\n----"<<re;
        printf("\n\nQuerying---------%s-----%d--------",this->pred.first.first.c_str(),this->pred.first.second);
        fflush(stdout);
        this->predecessor(sockfd);
        return this->predecessor(sockfd)->find(s,sockfd);
      }
      /*if(node_is_end){
        if(hash>cur.second){
          //query succ(start)
          return succ.first;
        }else{
          //query pred
          return pred.first;
        }
      }
      [>
      else if(node_is_start){
        if(hash>node.cur.second){
          //query succ
        }else{
          //trouble
        }
      }
      <]
      else{
        if(hash>cur.second){
          //query succ
          return succ.first;
          
        }else{
          //query pred
          return succ.first;
        }
     }*/
  }

private:
  unsigned long long hash;
  
  /* data */
};



int main(int argc, char *argv[])
{

  vector<string> file_list;
  int sockfd,server_port;
  unsigned long long hash;
  string s;
  socklen_t clilen;
  char buffer[MAXSIZE],buffer_2[MAXSIZE];
  int nbytes;
  char message[MAXSIZE];
  char *token;
  struct sockaddr_in serv_addr;
  struct sockaddr_in client_addr;
  int len=1000;
  if(argc>2){
    server_port=atoi(argv[2]);
  }
  else{
    server_port=571719;
  }
  sockfd=socket(AF_INET,SOCK_DGRAM,0);
  bzero((char*)&serv_addr,sizeof(serv_addr));
  serv_addr.sin_family=AF_INET;
  serv_addr.sin_addr.s_addr=INADDR_ANY;
  serv_addr.sin_port=htons(server_port);
  /*inet_ntop(AF_INET,&(serv_addr.sin_addr),buffer_2,len);*/
  /*printf("\n----------%s--------",buffer_2);*/
  fflush(stdout);
  if(bind(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0) {
    perror("ERROR on binding");
    exit(1);
  }
  address_pair x =make_pair(string(argv[1]),server_port);
  Node node=Node(x,getHash(x));
  listen(sockfd,5);
  clilen=sizeof(client_addr);
  printf("\n------receiving--------");
  fflush(stdout);
  nbytes=recvfrom(sockfd,message,MAXSIZE,0,(struct sockaddr*)&client_addr,&clilen);
  printf("\n\n%s",message);
  fflush(stdout);
  token=strtok(message,"$");
  strcpy(buffer,token);
  /*printf("\n\t%s",token);*/
  token=strtok(NULL,"$");
  strcpy(buffer_2,token);
  x=make_pair(string(buffer),atoi(buffer_2));
  node.pred = make_pair(x,getHash(x));
  token=strtok(NULL,"$");
  strcpy(buffer,token);
  token=strtok(NULL,"$");
  strcpy(buffer_2,token);
  address_pair y=make_pair(string(buffer),atoi(buffer_2));
  node.succ = make_pair(y,getHash(y));
  while((token=strtok(NULL,"$"))!=NULL){
    file_list.push_back(string(token));
    printf("\n\t%s",token);
  }
  node.node_is_start=node.cur.second<node.pred.second&&node.cur.second<node.succ.second;
  node.node_is_end=node.cur.second>node.pred.second&&node.cur.second>node.succ.second;
  vector<string>::iterator it;
  fflush(stdout);

  int server_here_port=10011;
  struct sockaddr_in serv_addr_stream;
  bzero((char*)&serv_addr_stream,sizeof(serv_addr_stream));
  serv_addr_stream.sin_family=AF_INET;
  serv_addr_stream.sin_addr.s_addr=INADDR_ANY;
  serv_addr_stream.sin_port=htons(server_here_port);
  int sockfd_stream=socket(AF_INET,SOCK_STREAM,0);
  if(sockfd_stream<0) 
    perror("ERROR opening socket");
  while (bind(sockfd_stream,(struct sockaddr*)&serv_addr_stream,sizeof(serv_addr_stream))<0){
    printf("\nERROR on binding to %d",server_here_port);
    fflush(stdout);
    server_here_port=rand()%5001+10001;
    serv_addr_stream.sin_port=htons(server_here_port);
  }
  int pid=fork();
  int pid2;
  if(pid!=0){
    pid2=fork();
    if(pid2!=0){
      string s;
      close(sockfd);
      sockfd=socket(AF_INET,SOCK_DGRAM,0);
      bzero((char*)&serv_addr,sizeof(serv_addr));
      serv_addr.sin_family=AF_INET;
      serv_addr.sin_addr.s_addr=INADDR_ANY;
      int port=10000;
      serv_addr.sin_port=htons(port);
      while(bind(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0) {
        /*printf("\nERROR on binding to %d",port);*/
        port=rand()%5001+10001;
        serv_addr.sin_port=htons(port);
      }
      /*while(1){}*/
      while(1){
        cout<<"\nEnter file name to download: ";
        cin>>s;
        it=find(file_list.begin(),file_list.end(),s);
        if(it!=file_list.end()){
          cout<<"\nThe file is already present";
        }else{
          char buffer_3[100];
          int sockfd_stream;
          sockfd_stream=socket(AF_INET, SOCK_STREAM, 0);
          if(sockfd_stream<0) 
              perror("ERROR opening socket");
          struct sockaddr_in serv_addr_stream;
          struct hostent *server_stream;
          int portno_stream;
          address_pair x = node.find(s,sockfd);
          portno_stream=x.second;
          printf("\n\n I am here %d",portno_stream);
          fflush(stdout);
          server_stream=gethostbyname(x.first.c_str());
          bzero((char*)&serv_addr_stream,sizeof(serv_addr_stream));
          serv_addr_stream.sin_family=AF_INET;
          bcopy((char*)server_stream->h_addr,(char*)&serv_addr_stream.sin_addr.s_addr,server_stream->h_length);
          serv_addr_stream.sin_port=htons(portno_stream);
          while(connect(sockfd_stream,(struct sockaddr*)&serv_addr_stream,sizeof(serv_addr_stream))<0) {perror("Can't connect");}

            ;
          printf("\n\n I am here %d",portno_stream);
          strcpy(buffer_3,"REQUEST ");
          strcat(buffer_3,s.c_str());
          nbytes=write(sockfd_stream,buffer_3,strlen(buffer_3));
          if(nbytes<0){
            perror("writing: ");
          }
          fflush(stdout);
          printf("\n\n I am here %d",portno_stream);
          fflush(stdout);
          nbytes=read(sockfd_stream,buffer,20);
          printf("\n\n I am here %s",buffer);
          fflush(stdout);
          if(nbytes<0){
            perror("Can't read");
          }
          if(strcmp(buffer,"SUCCESS")==0){
            fflush(stdout);
            const char *filename=basename(s.c_str());
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
        }
      }
    }else{
      /*freopen("inp", "r", stdin);*/
      /*freopen("output", "w", stdout);*/
      while(1){
        /*printf("reeeeeeeeeeeeeeeeee");*/
        fflush(stdout);
        string s=recvDatagram(sockfd,&client_addr);
        fflush(stdout);
        cout<<"\n\n"<<s;
        if(s[0]=='G'&&s[1]=='E'){
          strcpy(buffer,node.pred.first.first.c_str());
          strcat(buffer,"$");
          sprintf(buffer_2,"$%d$",node.pred.first.second);
          strcat(buffer,buffer_2);
          strcat(buffer,node.succ.first.first.c_str());
          sprintf(buffer_2,"$%d",node.succ.first.second);
          strcat(buffer,buffer_2);
          clilen=sizeof(client_addr);
        }
        else if(s[0]=='F'){
          string y=s.substr(5);
          cout<<"\n\n"<<y;
          it=find(file_list.begin(),file_list.end(),y);
          if(it!=file_list.end()){
            strcpy(buffer,"SUCCESS ");
            sprintf(buffer+strlen(buffer),"%d",server_here_port);
          }else{
            strcpy(buffer,"FAIL");
          }
        }
        printf("\n\nResponse %s",buffer);
        nbytes=sendto(sockfd,buffer,strlen(buffer)+1,0,(struct sockaddr*)&client_addr,clilen);
      }
    }
  }else{
    listen(sockfd_stream,5);
    int newsockfd;
    clilen=sizeof(client_addr);
    while(1){
      printf("Accepting");
      fflush(stdout);
      newsockfd=accept(sockfd_stream,(struct sockaddr*)&client_addr,&clilen);
      printf("\n\nAccepted");
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
  }
  return 0;
}

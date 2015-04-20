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
#include <utility>    
#include <fcntl.h>
#include <string>     
#include <iostream>   
#include <vector>    
#include <map>
using namespace std;
#define CHUNKSIZE 32
#define MAXSIZE 1000
typedef pair<string, int> address_pair;
typedef pair<address_pair, unsigned long long> address_hash_pair;

typedef map<string, address_pair> file_info; 

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
  printf("\n\n%s",message);
  fflush(stdout);
  if(nbytes>0){
    return string(message);
  }
  return string("");
}
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
  static address_pair origin;
  int node_is_end,node_is_start;
  int stream_port;

  

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

  address_pair* find(string s,int sockfd){
    char message[100];
    char* token;
    unsigned long long hash=oat_hash(s.c_str(),s.length());
    fflush(stdout);
    
    if(hash>cur.second){
      if((this->succ.first.first.compare(this->origin.first)==0)&&this->succ.first.second==this->origin.second){
        return NULL;
      }
      string re="FILE "+s;
      sendToAdr(re,sockfd,this->succ.first);
      re=recvDatagram(sockfd);
      if(re[0]=='S'&&re[1]=='U'){
        string y=re.substr(8);
        strcpy(message,y.c_str());
        token = strtok(message,"$");
        y = string(token);
        token = strtok(NULL,"$");
        int x=atoi(token);
        return new address_pair(y,x);
      }
      cout<<"\n\n----"<<re;
      printf("\n\nQuerying---------%s-----%d--------",this->succ.first.first.c_str(),this->succ.first.second);
      fflush(stdout);
      return (this->successor(sockfd)->find(s,sockfd));
    }else{
      //query pred
      if((this->pred.first.first.compare(this->origin.first)==0)&&this->pred.first.second==this->origin.second){
        return NULL;
      }
      string re="FILE "+s;
      sendToAdr(re,sockfd,this->pred.first);
      re=recvDatagram(sockfd);
      if(re[0]=='S'&&re[1]=='U'){
        string y=re.substr(8);
        strcpy(message,y.c_str());
        token = strtok(message,"$");
        y = string(token);
        token = strtok(NULL,"$");
        int x=atoi(token);
        return new address_pair(y,x);
      }
      cout<<"\n\n----"<<re;
      printf("\n\nQuerying---------%s-----%d--------",this->pred.first.first.c_str(),this->pred.first.second);
      fflush(stdout);
      this->predecessor(sockfd);
      return this->predecessor(sockfd)->find(s,sockfd);
    }
  }

private:
  unsigned long long hash;
  
  /* data */
};

address_pair Node::origin;


void fileShare(int sockfd, Node& node, vector<string>& file_list_uploaded, file_info& file_map, string* file_name,address_pair* store_add,char *m=NULL){
  char buf[100];
  if(store_add==NULL){
    strcpy(buf,m);
    char *token;
    token = strtok(buf+6,"$");
    file_name=new string(token);
    token = strtok(NULL,"$");
    strcpy(buf,token);
    token = strtok(NULL,"$");
    store_add = new address_pair(string(buf),atoi(token));
  }
  fflush(stdout);
  unsigned long long hash=oat_hash(file_name->c_str(),file_name->length());
  int startedHere=node.cur.first.first.compare(store_add->first)==0&&node.cur.first.second==store_add->second;
  address_pair target;
  if(hash<=node.cur.second){
    if(hash>node.pred.second || node.node_is_start){
      fflush(stdout);
      if(startedHere){
        file_list_uploaded.push_back(*file_name);
      }else{
        file_map.insert(make_pair(*file_name,*store_add));
      }
      return;
    }else{
      target=node.pred.first;
      //send to pred
    }
  }else{
    if(node.node_is_start && hash>node.pred.second){
      fflush(stdout);
      if(startedHere){
        file_list_uploaded.push_back(*file_name);
      }else{
        file_map.insert(make_pair(*file_name,*store_add));
      }
      return;
    }else{
      //send to succ
      target=node.succ.first;
    }
  }

  fflush(stdout);
  if(m==NULL){
    m = (char *)(malloc(sizeof(char)*100));
    strcpy(m,"SHARE$");
    strcat(m,file_name->c_str());
    sprintf(m+strlen(m),"$%s$%d",store_add->first.c_str(),store_add->second);
  }
  sendToAdr(string(m),sockfd,target);
  cout<<"\n\n Sending file share message "<<string(m);
  fflush(stdout);
}

int main(int argc, char *argv[])
{
  int n;
  file_info file_map;
  vector<string> file_list_uploaded;
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
  fflush(stdout);
  if(bind(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0) {
    perror("ERROR on binding");
    exit(1);
  }
  address_pair x =make_pair(string(argv[1]),server_port);
  Node node=Node(x,getHash(x));
  Node::origin=node.cur.first;
  listen(sockfd,5);
  clilen=sizeof(client_addr);
  printf("\n------receiving--------");
  fflush(stdout);
  nbytes=recvfrom(sockfd,message,MAXSIZE,0,(struct sockaddr*)&client_addr,&clilen);
  printf("\n\n%s",message);
  fflush(stdout);
  token=strtok(message,"$");
  strcpy(buffer,token);
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
  printf("\nEnter number of files to upload: ");
  cin>>n;
  int i;
  node.node_is_start=node.cur.second<node.pred.second&&node.cur.second<node.succ.second;
  node.node_is_end=node.cur.second>node.pred.second&&node.cur.second>node.succ.second;
  for (i = 0; i < n; ++i)
  {
    cout<<"\nEnter the fie name: ";
    cin>>s;
    file_list_uploaded.push_back(s);
    
  }
  
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
  node.stream_port=server_here_port;
  address_pair *stream_add=new address_pair(node.cur.first.first,server_here_port);

  for (i = 0; i < n; ++i)
  {
    fileShare(sockfd,node,file_list_uploaded,file_map,&file_list_uploaded[i],stream_add,NULL);
  }

  while(1){
    string msg = recvDatagram(sockfd);
    if(msg.compare(string("START"))==0)
      break;
    strcpy(buffer,msg.c_str());
    fileShare(sockfd,node,file_list_uploaded,file_map,NULL,NULL,buffer);
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
        port=rand()%5001+10001;
        serv_addr.sin_port=htons(port);
      }
      while(1){
        cout<<"\nEnter file name to download: ";
        cin>>s;
        it=find(file_list_uploaded.begin(),file_list_uploaded.end(),s);
        if(it!=file_list_uploaded.end()){
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
          address_pair *x = node.find(s,sockfd);
          if(x==NULL){
            printf("\nFile not available in any node");
            continue;
          }
          portno_stream=x->second;
          fflush(stdout);
          server_stream=gethostbyname(x->first.c_str());
          bzero((char*)&serv_addr_stream,sizeof(serv_addr_stream));
          serv_addr_stream.sin_family=AF_INET;
          bcopy((char*)server_stream->h_addr,(char*)&serv_addr_stream.sin_addr.s_addr,server_stream->h_length);
          serv_addr_stream.sin_port=htons(portno_stream);
          while(connect(sockfd_stream,(struct sockaddr*)&serv_addr_stream,sizeof(serv_addr_stream))<0) {perror("Can't connect");}
          strcpy(buffer_3,"REQUEST ");
          strcat(buffer_3,s.c_str());
          nbytes=write(sockfd_stream,buffer_3,strlen(buffer_3));
          if(nbytes<0){
            perror("writing: ");
          }
          fflush(stdout);
          fflush(stdout);
          nbytes=read(sockfd_stream,buffer,20);
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
      while(1){
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
          it=find(file_list_uploaded.begin(),file_list_uploaded.end(),y);
          if(it!=file_list_uploaded.end()){
            strcpy(buffer,"SUCCESS$");
            sprintf(buffer+strlen(buffer),"%s$%d",node.cur.first.first.c_str(),node.stream_port);
          }else{
            //check in file_map
            string y=s.substr(5);
            file_info::iterator it=file_map.find(y);
            if(it!=file_map.end()){
              strcpy(buffer,"SUCCESS$");
              sprintf(buffer+strlen(buffer),"%s$%d",it->second.first.c_str(),it->second.second);
            }else{
              strcpy(buffer,"FAIL");
            }
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
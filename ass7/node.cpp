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

typedef map<string, address_pair> file_info; 

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
  return h%(1<<3);
}

void SigCatcher(int n)
{
    pid_t kidpid=waitpid(-1,NULL,WNOHANG);
    printf("\nChild %d terminated\n",kidpid);
}

bool adcompare(address_pair p1,address_pair p2){
  return p1.first.compare(p2.first)==0&&p1.second==p2.second;
}

void print_file_share_info(file_info& f){
  file_info::iterator t;
  cout<<"\n\n FILE SHARE INFO";
  unsigned long long hash;
  for(t=f.begin();t!=f.end();t++){
    hash=oat_hash(t->first.c_str(),t->first.size());
    cout<<"\n"<<t->first<<" hash: "<<hash<<" "<<t->second.first<<":"<<t->second.second;
  }
  fflush(stdout);
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

class FingerTable{
  public:
    //max size is 32
    vector<address_hash_pair> finger;
    
    


};

class Node
{
public:
  Node ():cur(),pred(),succ(),succ_point(NULL),pred_point(NULL),finger(){
  }
  Node (address_pair x,unsigned long long hash):pred(),succ(),finger(){
    this->cur=make_pair(x,hash);
    int i;
    for (i = 0; i < 3; ++i)
    {
      this->finger.finger.push_back(this->cur);
    }
  };
  unsigned long long start(int i){
      return (this->cur.second+(1<<i))%(1<<3);

  }
  void print_finger(){
      printf("\n\n----FINGER TABLE-----");
      int i;
      for (i = 0; i < finger.finger.size(); ++i)
      {
        cout<<"\n"<<this->start(i)<<"  "<<this->finger.finger[i].first.first<<" "<<this->finger.finger[i].first.second;
      }
      printf("\n\n");

    }
  void init_finger_table(int sockfd){
    int i;
    unsigned long long start;
    address_pair ad;
    for(i=0;i<3;++i){
      start= this->start(i);
      ad=this->find_successor(sockfd,start);
      this->finger.finger[i]=make_pair(ad,getHash(ad));
    }
  }
  void update_finger_table(address_pair ad){
    unsigned long long hash=getHash(ad);
    address_hash_pair foo= make_pair(ad,hash);
    int i;
    for (i = 0; i < 3; ++i)
    {
      if(this->finger.finger[i].second>=this->start(i)){
        /*printf("\nlooooo");*/
        fflush(stdout);
        if(hash<this->finger.finger[i].second && hash>=this->start(i)){
          /*printf("\nkooooo");*/
          /*fflush(stdout);*/
          //put ad in ith entry
          this->finger.finger[i]=foo;
        }
      }else{
        if(hash<this->finger.finger[i].second&& hash<this->start(i)){
          //put ad in ith entry
          this->finger.finger[i]=foo;
        }else if(hash>=this->start(i)){
          //put ad in ith entry
          this->finger.finger[i]=foo;
        }
      }
    }
  }
  void handle_new_node(address_pair ad,int sockfd,file_info& f){
    printf("\nhaaaaaaaaaaaaaaaaaaaaaaa");
    fflush(stdout);
    unsigned long long hash = getHash(ad);
    bool isSuc=false;
    if(this->cur.second<hash && hash<this->succ.second){
      this->succ=make_pair(ad,hash);
      //put as successor
      isSuc=true;
    }else if(this->cur.second>=hash && hash>this->pred.second){
      this->pred=make_pair(ad,hash);
      //put as predecessor
    }else if(this->cur.second<this->pred.second && hash > this->pred.second){
      this->pred=make_pair(ad,hash);
      //put as predecessor
    }else if(this->cur.second>this->succ.second && hash > this->cur.second){
      this->succ=make_pair(ad,hash);
      isSuc=true;
      //put as successor
    }
    if(isSuc){
      file_info migrate;
      file_info::iterator t;
      unsigned long long file_hash;
      unsigned long long curadhash;
      for(t=f.begin();t!=f.end();t++){
        file_hash=oat_hash(t->first.c_str(),t->first.size());
        curadhash=getHash(this->cur.first);
        if(curadhash>=file_hash){
          if(hash<curadhash&&hash>=file_hash){
            //ad is successor of this file
            migrate.insert(make_pair(t->first,t->second));
          }
        }else{
          if(hash>=file_hash||hash<curadhash){
            //ad is the successor of this file
            migrate.insert(make_pair(t->first,t->second));
          }
        }
      }
      for(t=migrate.begin();t!=migrate.end();t++){
        f.erase(t->first);
      }
      char buf[1000];
      sprintf(buf,"MSHARE");
      for(t=migrate.begin();t!=migrate.end();t++){
        printf("\nlooooooooooooooooooooool");
        fflush(stdout);
        sprintf(buf+strlen(buf),"$%s$%s$%d",t->first.c_str(),t->second.first.c_str(),t->second.second);
      }
      printf("\n\n sending message %s",buf);
      fflush(stdout);
      sendToAdr(string(buf),sockfd,ad);
      
    }
    print_file_share_info(f);
    this->update_finger_table(ad);
    this->print_succ_pred();
  }
  void add_node(int sockfd,struct sockaddr_in client_addr,file_info& f){
    bool isSuc=false;
    address_pair old_suc=this->succ.first;
    char *c=inet_ntoa(client_addr.sin_addr);
    int port=ntohs(client_addr.sin_port);
    string s=string(c);
    address_pair ad = make_pair(s,port);
    int hash=getHash(ad);
    char buff[100];
    if(this->pred.first.first.compare(this->cur.first.first)==0&&this->pred.first.second==this->cur.first.second){
      //printf("NOw only 1 node");
      fflush(stdout);
      //only 1 node tillnow
      this->succ=make_pair(ad,hash);
      this->pred=make_pair(ad,hash);
      isSuc=true;
      sprintf(buff,"%s$%d$%s$%d",this->cur.first.first.c_str(),this->cur.first.second,
          this->cur.first.first.c_str(),this->cur.first.second);
      sendToAdr(string(buff),sockfd,ad);
    }else{
      Node n;
      n.succ = make_pair(this->find_successor(sockfd, hash),hash);
      printf("\nrooo");
      fflush(stdout);
      if(adcompare(n.succ.first,this->cur.first)){
        isSuc=true;
        //this node is the successor
        printf("\n I am successor");
        fflush(stdout);
        this->pred=make_pair(ad,hash);
        sprintf(buff,"%s$%d$%s$%d",this->pred.first.first.c_str(),this->pred.first.second,
          this->cur.first.first.c_str(),this->cur.first.second);
      }else{
        if(adcompare(n.succ.first,this->succ.first)&&adcompare(n.succ.first,this->pred.first)){
          sprintf(buff,"%s$%d$%s$%d",this->cur.first.first.c_str(),this->cur.first.second,
            n.succ.first.first.c_str(),n.succ.first.second);
          printf("\n 2 nodes");
          fflush(stdout);
          if(this->cur.second>this->succ.second){
            if(this->cur.second>hash){
              isSuc=true;
              //this node is the successor of the new node
              sprintf(buff,"%s$%d$%s$%d", n.succ.first.first.c_str(),n.succ.first.second,
                  this->cur.first.first.c_str(),this->cur.first.second);
              this->pred=make_pair(ad,hash);

            }else{
              //this node is the predecessor of the new node
              sprintf(buff,"%s$%d$%s$%d",this->cur.first.first.c_str(),this->cur.first.second,
                  n.succ.first.first.c_str(),n.succ.first.second);
              this->succ=make_pair(ad,hash);
            }
          }else{
            if(this->succ.second>hash){
              sprintf(buff,"%s$%d$%s$%d",this->cur.first.first.c_str(),this->cur.first.second,
                  n.succ.first.first.c_str(),n.succ.first.second);
              //succ node is the successor of the new node
              this->pred=make_pair(ad,hash);
            }else{
              
              sprintf(buff,"%s$%d$%s$%d", n.succ.first.first.c_str(),n.succ.first.second,
                  this->cur.first.first.c_str(),this->cur.first.second);
              //succ node is the predecessor of the new node
              isSuc=true;
              this->succ=make_pair(ad,hash);
            }
          }
        }else{
          Node * suc = n.successor(sockfd);
          printf("\n got successor");
          fflush(stdout);
          /*sprintf(buff,"US %s %d",ad.first.c_str(),ad.second);
          sendToAdr(string(buff),sockfd,suc->pred.first);
          sprintf(buff,"UP %s %d",ad.first.c_str(),ad.second);
          suc->sendToMe(string(buff),sockfd);*/
          sprintf(buff,"%s$%d$%s$%d",suc->pred.first.first.c_str(),suc->pred.first.second,
            n.succ.first.first.c_str(),n.succ.first.second);
          if(adcompare(suc->pred.first,this->cur.first)){
              this->succ=make_pair(ad,hash);
          }else if(adcompare(suc->succ.first,this->cur.first)){
              isSuc=true;
              this->pred=make_pair(ad,hash);
          }
        }
      }
      printf("\n sending message %s",buff);
      fflush(stdout);
      sendToAdr(string(buff),sockfd,ad);
      
      
      sprintf(buff,"NEW NODE %s %d",ad.first.c_str(),ad.second);
      printf("\n sending message %s",buff);
      fflush(stdout);
      sendToAdr(string(buff),sockfd,old_suc);
    }
    if(isSuc){
      file_info migrate;
      file_info::iterator t;
      unsigned long long file_hash;
      unsigned long long curadhash;
      for(t=f.begin();t!=f.end();t++){
        file_hash=oat_hash(t->first.c_str(),t->first.size());
        curadhash=getHash(t->second);
        if(curadhash>=file_hash){
          if(hash<curadhash&&hash>=file_hash){
            //ad is successor of this file
            migrate.insert(make_pair(t->first,t->second));
          }
        }else{
          if(hash>=file_hash||hash<curadhash){
            //ad is the successor of this file
            migrate.insert(make_pair(t->first,t->second));
          }
        }
      }
      for(t=migrate.begin();t!=migrate.end();t++){
        f.erase(t->first);
      }
      char buf[1000];
      sprintf(buf,"MSHARE");
      for(t=migrate.begin();t!=migrate.end();t++){
        printf("\nlooooooooooooooooooooool");
        fflush(stdout);
        sprintf(buf+strlen(buf),"$%s$%s$%d",t->first.c_str(),t->second.first.c_str(),t->second.second);
      }
      printf("\n\n sending message %s",buf);
      fflush(stdout);
      sendToAdr(string(buf),sockfd,ad);
      
    }

    this->update_finger_table(ad);
    print_file_share_info(f);
    this->print_finger();
    this->print_succ_pred();

  }
  void print_succ_pred(){
    printf("\n current successor: %s %d",this->succ.first.first.c_str(),this->succ.first.second);
    printf("\n current predecessor: %s %d",this->pred.first.first.c_str(),this->pred.first.second);
    
  }
  void fileShare(int sockfd, file_info& file_map, string file_name){
    char buf[100];
    unsigned long long hash=oat_hash(file_name.c_str(),file_name.size());
    address_pair ad=this->find_successor(sockfd,hash);
    if(adcompare(ad,this->cur.first)){
      file_map.insert(make_pair(file_name,make_pair(ad.first,this->stream_port)));
      return;
    }
    sprintf(buf,"SHARE$%s$%s$%d",file_name.c_str(),this->cur.first.first.c_str(),this->stream_port);
    sendToAdr(string(buf),sockfd,ad);
  }
  address_pair find_successor(int sockfd, int id,const char* req=NULL){
    /*printf("\n %d %llu",id,this->cur.second);*/
    /*this->print_succ_pred();*/
    address_pair target;
    if(id==this->cur.second){
      return this->cur.first;
    }
    else if(id==this->succ.second){
      return this->succ.first;
    }
    else if(id==this->pred.second){
      return this->pred.first;
    }
    else if(id>this->cur.second&&id<=this->succ.second){
      //between current and succ
      return this->succ.first;
    } else if(this->cur.second >= id && this->pred.second<id){
      //between current and pred
      return this->cur.first;
    }else if(this->cur.second<this->pred.second && (this->pred.second<id||this->cur.second>id)){
      //node is at start 
      return this->cur.first;
    } else if(this->cur.second>this->succ.second && this->cur.second<id){
      //node is at end
      return this->succ.first;
    } else if(this->cur.second>this->succ.second && this->succ.second>=id){
      //node is at end
      return this->succ.first;
    }else if(this->cur.second>id && this->pred.second>id){
      //send to predecessor of pred
      target=this->pred.first;
    }else if(this->cur.second<id && this->pred.second<id){
      target=this->succ.first;
      printf("\n yooo");
      fflush(stdout);
    }
    printf("\n\n This doesn't look good");
      fflush(stdout);

    target = this->closest_preceding_node(sockfd,id);
    if(adcompare(target,this->cur.first)){
      if(adcompare(this->cur.first,this->succ.first)){
        return target;
      }
      target=this->succ.first;
      /*printf("\ngooooooooooooooooooooo %s %d",this->succ.first.first.c_str(),this->succ.first.second);*/
      /*return this->cur.first;*/
    } 
    /*printf("\n\n\n\n I am here");*/
    fflush(stdout);
    char m[100];
    if(req==NULL)
      sprintf(m,"FSUC$%d$%s$%d",id,this->cur.first.first.c_str(),this->cur.first.second);
    else
      sprintf(m,"%s",req);
    //printf("\n sending message %s",m);
    fflush(stdout);
    sendToAdr(string(m),sockfd,target);
    string s=recvDatagram(sockfd);
    strcpy(m,s.c_str());
    char buffer[100];
    char buffer_2[100];
    char *token;
    strcpy(m,s.c_str());
    token=strtok(m,"$");
    strcpy(buffer,token);
    token=strtok(NULL,"$");
    strcpy(buffer_2,token);
    address_pair x=make_pair(string(buffer),atoi(buffer_2));
    return x;
  }
  address_pair closest_preceding_node(int sockfd, int id){
    int i;
    for (i = this->finger.finger.size(); i >= 0; --i)
    {
      if(finger.finger[i].second>this->cur.second&&finger.finger[i].second<id){
        return finger.finger[i].first;
      }
    }
    return this->cur.first;
  }
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
  static address_pair origin;
  int node_is_end,node_is_start;
  int stream_port;
  FingerTable finger;

  

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


  address_pair* find(string s,int sockfd,file_info& f){
    char message[100];
    char* token;
    unsigned long long hash=oat_hash(s.c_str(),s.length());
    printf("\n haaash  %llu",hash);
    fflush(stdout);

    string re="FILE "+s;
    address_pair ad = this->find_successor(sockfd,hash);
    if(adcompare(ad,this->cur.first)){
      file_info::iterator it;
      it=f.find(s);
      if(it!=f.end()){
        return &(it->second);
      }else{
        return NULL;
      }
    }
    sendToAdr(re,sockfd,ad);
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
    return NULL;
  }

private:
  unsigned long long hash;
  
  /* data */
};

address_pair Node::origin;




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
  /*inet_ntop(AF_INET,&(serv_addr.sin_addr),buffer_2,len);*/
  /*printf("\n----------%s--------",buffer_2);*/
  fflush(stdout);
  if(bind(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0) {
    perror("ERROR on binding");
    exit(1);
  }
  address_pair x =make_pair(string(argv[1]),server_port);
  printf("\n Current hash %llu",getHash(x));
  Node node=Node(x,getHash(x));
  node.pred=node.cur;
  node.succ=node.cur;
  Node::origin=node.cur.first;
  listen(sockfd,5);
  clilen=sizeof(client_addr);
  printf("\n------receiving--------");
  fflush(stdout);
  if(server_port!=9004){
    strcpy(buffer,"JOIN");
    bzero((char*)&serv_addr,sizeof(serv_addr));
    hostent* server=gethostbyname(argv[1]);
    serv_addr.sin_family=AF_INET;
    bcopy((char*)server->h_addr,(char*)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port=htons(9004);//0th node
    len=sizeof(serv_addr);
    nbytes=sendto(sockfd,buffer,strlen(buffer)+1,0,(struct sockaddr*)&serv_addr,len);
    printf("\n sent message");
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
    node.update_finger_table(node.succ.first);
    node.update_finger_table(node.pred.first);
    node.init_finger_table(sockfd);
    node.print_finger();
    node.print_succ_pred();
  /*while((token=strtok(NULL,"$"))!=NULL){
    file_list.push_back(string(token));
    printf("\n\t%s",token);
  }*/
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
    node.fileShare(sockfd,file_map,s);
  }
  print_file_share_info(file_map);
  
  


  
  node.print_finger();
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
        string path;
        cout<<"\nEnter file name to download: ";
        cin>>s;
        printf("\nGive path to save the file:");
        cin>>path;
        it=find(file_list_uploaded.begin(),file_list_uploaded.end(),s);
        if(it!=file_list_uploaded.end()){
          cout<<"\nThe file is already present";
        }else{
          string st="DOWNLOAD ";
          st=st+s+"$"+path;
          sendToAdr(st,sockfd,node.cur.first);
        }
      }
    }else{
      /*freopen("inp", "r", stdin);*/
      /*freopen("output", "w", stdout);*/
      string new_node_msg;
      while(1){
        /*printf("reeeeeeeeeeeeeeeeee");*/
        fflush(stdout);
        string s=recvDatagram(sockfd,&client_addr);
        fflush(stdout);
        cout<<"\n\n"<<s;
        node.print_finger();
        if(s[0]=='G'&&s[1]=='E'){
          strcpy(buffer,node.pred.first.first.c_str());
          strcat(buffer,"$");
          sprintf(buffer_2,"$%d$",node.pred.first.second);
          strcat(buffer,buffer_2);
          strcat(buffer,node.succ.first.first.c_str());
          sprintf(buffer_2,"$%d",node.succ.first.second);
          strcat(buffer,buffer_2);
          clilen=sizeof(client_addr);
          printf("\n\nResponse %s",buffer);
          nbytes=sendto(sockfd,buffer,strlen(buffer)+1,0,(struct sockaddr*)&client_addr,clilen);
        }
        else if(s[0]=='D'&&s[1]=='O'){
          string y=s.substr(9);
          char path[100];
          char *token;
          char m[1000];
          strcpy(m,y.c_str());

          char buffer_3[100];
          int sockfd_stream;
          sockfd_stream=socket(AF_INET, SOCK_STREAM, 0);
          if(sockfd_stream<0) 
              perror("ERROR opening socket");
          struct sockaddr_in serv_addr_stream;
          struct hostent *server_stream;
          int portno_stream;
          token=strtok(m,"$");
          y=string(token);
          token=strtok(NULL,"$");
          strcpy(path,token);
          address_pair* x = node.find(y,sockfd,file_map);
          if(x==NULL){
            printf("\nFile not available in any node");
            continue;
          }
          portno_stream=x->second;
          /*printf("\n\n I am here %d",portno_stream);*/
          fflush(stdout);
          server_stream=gethostbyname(x->first.c_str());
          bzero((char*)&serv_addr_stream,sizeof(serv_addr_stream));
          serv_addr_stream.sin_family=AF_INET;
          bcopy((char*)server_stream->h_addr,(char*)&serv_addr_stream.sin_addr.s_addr,server_stream->h_length);
          serv_addr_stream.sin_port=htons(portno_stream);
          while(connect(sockfd_stream,(struct sockaddr*)&serv_addr_stream,sizeof(serv_addr_stream))<0) {perror("Can't connect");}

            ;
          /*printf("\n\n I am here %d",portno_stream);*/
          strcpy(buffer_3,"REQUEST ");
          strcat(buffer_3,y.c_str());
          nbytes=write(sockfd_stream,buffer_3,strlen(buffer_3));
          if(nbytes<0){
            perror("writing: ");
          }
          fflush(stdout);
          nbytes=read(sockfd_stream,buffer,20);
          if(nbytes<0){
            perror("Can't read");
          }
          if(strcmp(buffer,"SUCCESS")==0){
            fflush(stdout);
            const char *filename=basename(y.c_str());
            int nwritten,nread;
            strcpy(buffer_3,path);
            if(buffer_3[strlen(buffer_3)-1]!='/'){
              strcat(buffer_3,"/");
            }
            strcat(buffer_3,filename);
            printf("\n\n%s\n",buffer_3);
            fflush(stdout);
            int fd_to=open(buffer_3,O_WRONLY|O_CREAT,0666);
            if(fd_to<0){
              perror("Can't write/create");
            }
            else{
              int i;
              while(nbytes=read(sockfd_stream,buffer,CHUNKSIZE),nbytes>0){
                /*printf("%s",buffer);*/
                /*fflush(stdout);*/
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
        //}

        }
        else if(s[0]=='F'&&s[1]=='I'){
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
          printf("\n\nResponse %s",buffer);
          nbytes=sendto(sockfd,buffer,strlen(buffer)+1,0,(struct sockaddr*)&client_addr,clilen);
        }else if(s[0]=='S'&&s[1]=='H'){
          string y=s.substr(6);
          char buffer[100];
          char m[100];
          char buffer_2[100];
          strcpy(m,y.c_str());
          token=strtok(m,"$");
          strcpy(buffer,token);
          token=strtok(NULL,"$");
          strcpy(buffer_2,token);
          token=strtok(NULL,"$");
          address_pair ad= make_pair(string(buffer_2),atoi(token));
          file_map.insert(make_pair(string(buffer),ad));
          print_file_share_info(file_map);
        }else if(s[0]=='M'&&s[1]=='S'){
          string y=s;
          char buffer[100];
          char m[100];
          char buffer_2[100];
          strcpy(m,y.c_str());
          token=strtok(m,"$");
          printf("\n\n %s",token);
          fflush(stdout);
          while((token=strtok(NULL,"$"))!=NULL){
            strcpy(buffer,token);
            token=strtok(NULL,"$");
            strcpy(buffer_2,token);
            token=strtok(NULL,"$");  
            address_pair ad= make_pair(string(buffer_2),atoi(token));
            file_map.insert(make_pair(string(buffer),ad));
          }
          print_file_share_info(file_map);
        }
        else if(s[0]=='F'&&s[1]=='S'){
          printf("\n %s",s.c_str());
          fflush(stdout);
          char buffer[100];
          char buffer_2[100];
          char m[100];
          string y=s.substr(5);
          strcpy(m,y.c_str());
          token=strtok(m,"$");
          strcpy(buffer,token);
          token=strtok(NULL,"$");
          strcpy(buffer_2,token);
          token=strtok(NULL,"$");
          address_pair target=make_pair(string(buffer_2),atoi(token));
          address_pair ad=node.find_successor(sockfd,atoi(buffer),s.c_str());
          printf("\n\n %d",atoi(buffer));
          printf("\n\n%s$%d",ad.first.c_str(),ad.second);
          sprintf(buffer,"%s$%d",ad.first.c_str(),ad.second);
          sendToAdr(string(buffer),sockfd,target);
        }
        else if(s[0]=='J'&&s[1]=='O'){
          printf("\n\nJoiniiiiiing");
          fflush(stdout);
          node.add_node(sockfd,client_addr,file_map);
        }
        else if(s[0]=='N'&&s[1]=='E'){
          /*printf("newwwwwwwwwww");*/
          fflush(stdout);
          if(node.cur.first.second==9004){
            continue;
          }
          if(new_node_msg.compare(s)==0){
            printf("\n stopping");
            continue;;
          }
          /*printf("Newwwwwwwwwwwwwwwwwwwwwwwww");*/
          fflush(stdout);
          new_node_msg=s;
          string y=s.substr(9);
          /*cout<<"\n"<<y;*/
          fflush(stdout);
          char buffer[100];
          char m[100];
          char buffer_2[100];
          strcpy(m,y.c_str());
          char *token;
          strcpy(m,y.c_str());
          token=strtok(m," ");
          strcpy(buffer,token);
          token=strtok(NULL," ");
          strcpy(buffer_2,token);
          address_pair x=make_pair(string(buffer),atoi(buffer_2));  
          sendToAdr(s,sockfd,node.succ.first);
          /*printf("Newwwwwwwwwwwwwwwwwwwwwwwww");*/
          fflush(stdout);
          node.handle_new_node(x,sockfd,file_map);
          /*node.init_finger_table(sockfd);*/
          node.print_finger();
          fflush(stdout);

        }
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

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<errno.h>
#include<unistd.h>
#include"user.pb.h"
#include"user.pb.cc"
#include"flag.pb.h"
#include"flag.pb.cc"
#include"player.pb.h"
#include"player.pb.cc"
#include<iostream>
#include<fstream>
#include<sys/epoll.h>
#include<sys/types.h>
#include<errno.h>
#include<unistd.h>
#include<vector>
#include<set>
#define MAXEVENTS 64
using namespace std;
#define BUFF_SIZE 1024 //为2的指数
#define HEAD_LENGTH 3

#define  flagh 1
#define  userh 2
#define  playerh 3
#define filename "UserConfig.txt"




struct MessageBuff
{
    int tail;
    int head;
    char data[BUFF_SIZE];
    MessageBuff()
    {
        tail=0;
        head=0;
    }
};

struct ClientMessage
{
    vector<User> users;
    vector<Flag> Flags;
    vector<Player> players;
    ClientMessage()
    {
        users.clear();
        Flags.clear();
        players.clear();
    }
};



map <int,MessageBuff> RecvMessage;

map<int,ClientMessage> pedMessage;//被加工过的信息

set<int> clients;




User getUserMessageFromBuff(char *data,int head,int len,int mflag)
{
    User user;
    if(mflag)    //消息循环队列中的消息被截断
    {
        char *p;
        p = (char *)malloc(len*sizeof(char));
        for(int i=0;i<len;i++)
        {
            p[i]=data[(head+i)%BUFF_SIZE];
        }
        if(!user.ParseFromArray(p,len))
        {
            printf("接受User信息时反序列化失败\n");
        }
        free(p);
    }
    else
    {
        if(!user.ParseFromArray(data+head,len))
        {
            printf("接受User信息时反序列化失败\n");
        }
    }
    return user;
}

Player getPlayerrMessageFromBuff(char *data,int head,int len,int mflag)
{
    Player player;
    if(mflag)    //消息循环队列中的消息被截断
    {
        char *p;
        p = (char *)malloc(len*sizeof(char));
        for(int i=0;i<len;i++)
        {
            p[i]=data[(head+i)%BUFF_SIZE];
        }
        if(!player.ParseFromArray(p,len))
        {
            printf("接受Player信息时反序列化失败\n");
        }
        free(p);
    }
    else
    {
        if(!player.ParseFromArray(data+head,len))
        {
            printf("接受Player信息时反序列化失败\n");
        }
    }
    return player;
}

void CloseClientSocket(int fd)
{
    close(fd);
    clients.erase(fd);
    printf("socket:%d,close\n",fd);
    printf("there are %d clients remain\n",clients.size());                
}

void AllocNewClientConfig(int fd)
{
    clients.insert(fd);
    RecvMessage[fd]= MessageBuff();
    pedMessage[fd]=ClientMessage();
}


int Strtol(const char *nptr, int base) //检测输入的是否是数字
{
    char *new_endptr;
    int val = strtol(nptr,&new_endptr,base);
    if(new_endptr==nptr)
    {
        printf("No digits were found\n ");
        exit(EXIT_FAILURE);
    }
     return val;
}

 int Socket(int domain, int type, int protocol) 
{
    int val=socket(domain,type,protocol);
    if (val == -1)
    {
         printf("new socket error, errno = %d\n", errno);
         exit(EXIT_FAILURE);                           
    }
    
    return val;
}

int Listen(int __fd,int __n)
{
    int val=listen(__fd,__n);
    if (val < 0)
    {
        printf("listen ret:%d, error:%d\n", val, errno); 
        
    }
    else
    {
        printf("start listening on socket fd [%d] ...\n", val);                    
    }
    return val;
}
void Setsockopt(int __fd,int __level,int __optname,const void *__optval,socklen_t __optlen)
{
   if(setsockopt(__fd,__level,__optname,__optval,__optlen)<0)
   {
        printf("setsockopt failed\n");
        exit(EXIT_FAILURE);      
   }
}
int Accept(int listen_fd)
{
    struct sockaddr client_addr;
    socklen_t client_addr_len=sizeof(client_addr);
    int newsocket=accept(listen_fd,&client_addr,&client_addr_len);
    printf("new socket:%d\n",newsocket);
    printf("there are %d clients\n",clients.size());
    if(newsocket<0)
    {
        printf("new socket error\n");
    }
    return newsocket;
}
 
int Send(int fd,const void *buf,size_t len)
{
    int ret=send(fd,buf,len,0);
    if(ret<0)
    {
        CloseClientSocket(fd);
    }
    return ret;
}

//class RecvMessageManage
//{
    char retdata[BUFF_SIZE];

    void WriteUserConfig(User user)
    {
        fstream output(filename,ios::out|ios::app|ios::binary);
        output<<user.name()<<' '<<user.password()<<' ';
        output.close();
    }

    int CheckUserConfig(User user)
    {
        fstream input(filename,ios::in|ios::binary);
        string name,password;
        while(input>>name>>password)
        {
            if(user.name()==name)
            {
                if(user.password()==password)
                {
                    return 1;//user exist and password right;
                }
                else
                {
                    return 2;//user exist but password wrong;
                }
            }
        }
        return 3;//user does not exist;
    }

    void AddHead(char *data,int tag,int l)
    {
        data[0]=tag;
        data[1]=l/100;
        data[2]=l%100;
    }
//public:
    void HandleUserMessage(int fd,User user)
    {
        Flag f;
        if(user.type()==0)  //enter
        {
            int i=CheckUserConfig(user);
            f.set_flag(i);
            f.set_type(2);
            if(i==1)
            {
                printf("you can get in\n");
            }
            else if(i==2)
            {
                printf("password wrong\n");
            }
            else
            {
                printf("no user exist\n");
            }
            cout<<user.name()<<endl;
            cout<<user.password()<<endl;
        }
        else if(user.type()==1)    //register
        {
            int i=CheckUserConfig(user);
            f.set_flag(i);
            f.set_type(1);
            if(i==1||i==2)
            {
                printf("user exist\n");
            }
            else
            {
                WriteUserConfig(user);
                printf("register succees\n");
            }
            cout<<user.name()<<endl;
            cout<<user.password()<<endl;
        }
        
        if(!f.SerializeToArray(retdata+HEAD_LENGTH,BUFF_SIZE))
        {
            printf("Flag信息序列化失败\n");
        }
        AddHead(retdata,flagh,f.ByteSize());
        send(fd,retdata,f.ByteSize()+HEAD_LENGTH,0);
    }
    void HandlePlayerMessage(Player player)
    {
        cout<<player.name()<<' '<<player.x()<<' '<<player.y()<<' '<<player.z()<<endl;
        player.SerializeToArray(retdata+HEAD_LENGTH,BUFF_SIZE);
        AddHead(retdata,playerh,player.ByteSize());
        for(set<int>::iterator it =clients.begin();it!=clients.end();)
        {    
            Send(*it++,retdata,player.ByteSize()+HEAD_LENGTH);  //存疑!!!
        }
    }
//};

void Recv(int fd)
{
    MessageBuff &mBuff=RecvMessage[fd];
    int ret;
    if(mBuff.head==(mBuff.tail+1)%BUFF_SIZE)
    {
        printf("there is too many message from socket[%d]\n",fd);
        return ;
    }
    else if(mBuff.tail<mBuff.head)
    {
        ret=recv(fd,mBuff.data+mBuff.tail,mBuff.head-mBuff.tail-1,0);
    }
    else
    {
        ret=recv(fd,mBuff.data+mBuff.tail,BUFF_SIZE-mBuff.tail,0);
    }

    if(ret==0)
    {
        CloseClientSocket(fd);
    }
    else if(ret==-1)
    {
        printf("recv failed:socket[%d]\n",ret);
    }
    else
    {
        mBuff.tail=(mBuff.tail+ret)%BUFF_SIZE;
    }
}

void sortRecvMessage(int fd)
{
    MessageBuff &mBuff=RecvMessage[fd];
    ClientMessage &cMessage=pedMessage[fd];
    int l=mBuff.head;
    int r=mBuff.tail;
    if(r<l) r+=BUFF_SIZE;
    
    while(l+3<=r)
    {
        int tag=mBuff.data[l%BUFF_SIZE];
        int len=mBuff.data[(l+1)%BUFF_SIZE]*100+mBuff.data[(l+2)%BUFF_SIZE];
        if(l+len>r) break;
        int mflag=0;
        if(l<BUFF_SIZE&&l+len>=BUFF_SIZE)   
            mflag=1;
        switch(tag)
        {
            case userh:
            cMessage.users.push_back(getUserMessageFromBuff(mBuff.data,l%BUFF_SIZE,len,mflag));
            case playerh:
            cMessage.players.push_back(getPlayerrMessageFromBuff(mBuff.data,l%BUFF_SIZE,len,mflag));
            break;
        }
    }
}

void HandleMessageWhenReceive(int fd)
{
    ClientMessage &cMessage=pedMessage[fd];
    for(int i=0;i<cMessage.users.size();i++)
    {
        HandleUserMessage(fd,cMessage.users[i]);
    }
    if(cMessage.players.size()>0)
    HandlePlayerMessage(*cMessage.players.end());
    cMessage.users.clear();
    cMessage.players.clear();
}

int main(int argc,char **argv)
{
    if(argc<2)
    {
        printf("input port\n");
        return 0;
    }
    //input port and check   
    int port= Strtol(argv[1],10);  
    //init listen fd ans check
    int listen_fd=Socket(AF_INET,SOCK_STREAM,0);  
    struct sockaddr_in listen_addr;
    listen_addr.sin_family=AF_INET;
    listen_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    listen_addr.sin_port=htons(port);
    
    int value = 1;
    Setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&value, sizeof(value));

    int bind_ret = bind(listen_fd, (struct sockaddr*)&listen_addr, sizeof(listen_addr));
    if (bind_ret < 0)
    { 
        printf("bind ret:%d, error:%d\n", bind_ret, errno); 
        return 0;
                        
    }
    else
    {
        printf("bind [0.0.0.0:%d] ok!\n", port);
    }
    int listen_ret = Listen(listen_fd, 128);
    
    int epoll_fd=epoll_create(10);
    
    struct epoll_event event;
    event.data.fd=listen_fd;
    event.events=EPOLLIN;
    
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event);
    if (-1 == ret)
    {
        printf("epoll ctl error\n");
        return 0;                                
    }
    
    struct epoll_event* events = (struct epoll_event*)malloc(sizeof(event)*MAXEVENTS);
       
    while(1)
    {
        int n=epoll_wait(epoll_fd,events,MAXEVENTS,-1);
        for(int i=0;i<n;i++)
        {
            int fd=events[i].data.fd;
            int fd_events=events[i].events;
            if ((fd_events & EPOLLERR) ||
                 (fd_events & EPOLLHUP) ||
               (!(fd_events & EPOLLIN)))
                {
                     printf("fd:%d error\n", fd);
                     close(fd);
                     continue;                                                           
                }
            else if(listen_fd==fd)
            {
                int newsocket=Accept(fd);
                if(newsocket<0) continue;

                AllocNewClientConfig(newsocket);

                event.data.fd=newsocket;
                event.events = EPOLLIN;
                int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newsocket, &event);
                if (-1 == ret)
                {
                    printf("epoll ctl error\n");
                    continue;
                }
            }
            else
            {
                Recv(fd);
                sortRecvMessage(fd);
                HandleMessageWhenReceive(fd);
            }
        }
    }
    return 0; 
}

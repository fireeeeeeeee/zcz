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
#define MAXEVENTS 64
using namespace std;
#define BUFF_SIZE 1024

#define  flagh 1
#define  userh 2
#define  playerh 3



#define filename "UserConfig.txt"

int Strtol(const char *nptr, char **endptr, int base) //检测输入的是否是数字
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
}

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

int Setsockopt(int __fd,int __level,int __optname,const void *__optval,socklen_t __optlen)
{
   if(setsockopt(__fd,__level,__optname,__optval,__optlen)<0)
   {
        printf("setsockopt failed\n");
        exit(EXIT_FAILURE);      
   }
}

User user;
Flag flag;
Player player;
char data[BUFF_SIZE]={0};

int getData()
{
    int l=data[1]*100+data[2];
    int tag=data[0];
    switch(tag)
    {
        case 1:
        flag.ParseFromArray(data+3,l);
        break;
    case 2:
        user.ParseFromArray(data+3,l);
        break;
    case 3:
        player.ParseFromArray(data+3,l);
        break;
    }
    //memset(data,0,BUFF_SIZE);
    return tag;
}

char retdata[BUFF_SIZE]={0};

void Send(int fd,int tag,char *retdata)
{
    char a[BUFF_SIZE];
    memset(a,0,sizeof(a));
    a[0]=tag;
    int l=strlen(retdata);
    a[1]=l/100;
    a[2]=l%100;
    for(int i=0;i<l;i++)
    {
        a[i+3]=retdata[i];
    }
    send(fd,a,l+3,0);
}

 vector<int> client; 

void getUserMessage(int fd)
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
        int flag=CheckUserConfig(user);
        f.set_flag(flag);
        f.set_type(1);
        if(flag==1||flag==2)
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
    f.SerializeToArray(retdata,BUFF_SIZE);
    Send(fd,1,retdata);
    memset(retdata,0,sizeof(retdata));
    memset(data,0,BUFF_SIZE);
}

void getPlayerMessage(int fd)
{
    cout<<player.name()<<' '<<player.x()<<' '<<player.y()<<' '<<player.z()<<endl;
    player.SerializeToArray(retdata,BUFF_SIZE);
    for(int i=0;i<client.size();i++)
    {
        if(client[i]==fd)   continue;
        Send(client[i],playerh,retdata);
    }
    memset(retdata,0,sizeof(retdata));
}


int main(int argc,char **argv)
{
    if(argc<2)
    {
        printf("input port\n");
        return 0;
    }
    //input port and check   
    int port= Strtol(argv[1],NULL,10);  
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
        printf("bind ret:%d, error:%d\n", bind_ret, errno); return 0;
                        
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
                struct sockaddr client_addr;
                socklen_t client_addr_len=sizeof(client_addr);
                int newsocket=accept(listen_fd,&client_addr,&client_addr_len);
                client.push_back(newsocket);
                printf("new socket:%d\n",newsocket);
                printf("there are %d clients\n",client.size());
                if(newsocket<0)
                {
                    printf("new socket error\n");
                }
                event.data.fd=newsocket;
                event.events = EPOLLIN;
                int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newsocket, &event);
                if (-1 == ret)
                {
                    printf("epoll ctl error\n");
                    return 0;
                }
            }
            else
            {
                int ret=-1;
                if(ret=recv(fd,data,BUFF_SIZE,0)>0)
                {
                    int tag=getData();
                    if(tag==userh)
                    {
                        getUserMessage(fd);
                    }
                    else if(tag==playerh)
                    {
                        getPlayerMessage(fd);
                    }
                }
                else if(ret==0)
                {
                    printf("socket:%d,close\n",fd);
                    client.erase(std::remove(client.begin(),client.end(),fd),client.end());
                    printf("there is %d clients remain\n",client.size());
                    close(fd);
                }
                else
                {
                    printf("recv failed\n");
                }
            }
        }
    }
    return 0; 
}

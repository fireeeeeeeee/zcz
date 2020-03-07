#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#define BUFF_SIZE 1024

int main(int argc, char** argv)
{
        if (argc < 2)
        {
                    printf("input port\n"); return 0;
                        
        }

            int port = atoi(argv[1]);

                int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
                    printf("listen:%d\n", listen_fd);
                        if (listen_fd == -1)
                        {
                                    printf("new socket error, errno = %d\n", errno); return 0;
                                        
                        }
                            struct sockaddr_in listen_addr;
                                listen_addr.sin_family = AF_INET;
                                    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                                        listen_addr.sin_port = htons(port);

                                            int value = 1;
                                                setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&value, sizeof(value));

                                                    int bind_ret = bind(listen_fd, (struct sockaddr*)&listen_addr, sizeof(listen_addr));
                                                        if (bind_ret < 0)
                                                        {
                                                                    printf("bind ret:%d, error:%d\n", bind_ret, errno); return 0;
                                                                        
                                                        }
                                                        else {
                                                                    printf("bind [0.0.0.0:%d] ok!\n", port);
                                                                        
                                                        }

                                                            int listen_ret = listen(listen_fd, 128);
                                                                if (listen_ret < 0)
                                                                {
                                                                            printf("listen ret:%d, error:%d\n", listen_ret, errno); return 0;
                                                                                
                                                                }
                                                                else {
                                                                            printf("start listening on socket fd [%d] ...\n", listen_fd);
                                                                                
                                                                }

                                                                    
                                                                    while(1){
                                                                                struct sockaddr client_addr;
                                                                                        socklen_t client_addr_len = sizeof(client_addr);
                                                                                                int newsocket = accept(listen_fd, &client_addr, &client_addr_len);
                                                                                                        printf("new socket:%d\n", newsocket);

                                                                                                                int ret = -1;
                                                                                                                        char data[BUFF_SIZE] = { 0  };
                                                                                                                                while (ret = recv(newsocket, data, BUFF_SIZE, 0) > 0)
                                                                                                                                {
                                                                                                                                                printf("fd:%d recv:%s\n", newsocket, data);
                                                                                                                                                            send(newsocket, data, strlen(data), 0);
                                                                                                                                                                        memset(data, 0, BUFF_SIZE);
                                                                                                                                                                                
                                                                                                                                }

                                                                                                                                        if (ret == 0)
                                                                                                                                        {
                                                                                                                                                        printf("socket:%d, closed\n", newsocket);
                                                                                                                                                                
                                                                                                                                        }
                                                                                                                                                else if (ret == -1)
                                                                                                                                                {
                                                                                                                                                                printf("recv failed\n");
                                                                                                                                                                        
                                                                                                                                                }
                                                                                                                                                    
                                                                    
                                                                    }
                                                                        return 0;
                                                                        
};


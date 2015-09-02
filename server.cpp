#include <stdio.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <vector>
#include <assert.h>
#include <unistd.h>

#include "net_protocol.hpp"

using namespace std;

// TODO Handle SIGFPIPE

int listen_fd = -1;

struct pollfd fds[100]; // TODO MAX FD PER PROCESSUS?
size_t sz = 0;

int main()
{
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd == -1)
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    printf("listen fd = %d.\n", listen_fd);
    fds[sz++] = { listen_fd, POLLIN, 0 };

    int optval = 1;
    int res = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
                         &optval, sizeof(optval));
    if(res == -1)
    {
        perror("setsockopt(SO_REUSEADDR)");
        exit(EXIT_FAILURE);
    }

    res = setsockopt(listen_fd, IPPROTO_TCP, TCP_NODELAY,
                     &optval, sizeof(optval));
    if(res == -1)
    {
        perror("setsockopt(TCP_NODELAY)");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(55555);
    listen_addr.sin_addr.s_addr = INADDR_ANY;

    res = bind(listen_fd, (sockaddr*)&listen_addr, sizeof(listen_addr));
    if(res == -1)
    {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    res = listen(listen_fd, 10/*backlog*/);
    if(res == -1)
    {
        perror("listen()");
        exit(EXIT_FAILURE);
    }


    while(true)
    {
        int nfds = poll(&fds[0], sz, -1/*timeout*/);

        if(nfds == -1)
        {
            perror("poll()");
            exit(EXIT_FAILURE);
        }

        for(size_t i = 0; i < sz; i++)
        {
            struct pollfd pollfd = fds[i];
            int fd = pollfd.fd;
            if(pollfd.revents != 0)
            {
                if(fd == listen_fd)
                {
                    // TODO Read man page for accept4(). And possible errors.
                    int new_fd = accept(fd, nullptr, nullptr);
                    if(new_fd == -1)
                    {
                        perror("accept()");
                        exit(EXIT_FAILURE);
                    }
                    printf("accepted fd = %d.\n", new_fd);
                    fds[sz++] = {new_fd, POLLIN|POLLHUP, 0};
                }
                else
                {
                    assert(pollfd.revents & (POLLIN));
                    char buf[1024];
                    int n = recv(fd, buf, sizeof(buf), 0);
                    if(n == -1)
                    {
                        perror("recv()");
                        exit(EXIT_FAILURE);
                    }
                    else if(n == 0)
                    {
                        printf("close fd = %d.\n", fd);
                        res = close(fd);
                        if(res == -1)
                        {
                            perror("close()");
                            exit(EXIT_FAILURE);
                        }
                        fds[i] = {-1,0,0};
                    }
                    else
                    {
                        printf("recv = %s.\n", buf);
                    }
                }
                nfds--;
            }
        }

        // remove(fds, fds+sz, fd);
        // sz--;

    }
}

#include <stdio.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

// TODO Handle SIGFPIPE

int listen_fd = -1;

int main()
{
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd == -1)
    {
        perror("socket():");
        exit(EXIT_FAILURE);
    }

    int optval = 1;
    int res = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
                         &optval, sizeof(optval));
    if(res == -1)
    {
        perror("setsockopt(SO_REUSEADDR):");
        exit(EXIT_FAILURE);
    }

    res = setsockopt(listen_fd, IPPROTO_TCP, TCP_NODELAY,
                     &optval, sizeof(optval));
    if(res == -1)
    {
        perror("setsockopt(TCP_NODELAY):");
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
        perror("bind():");
        exit(EXIT_FAILURE);
    }

    res = listen(listen_fd, 10/*backlog*/);
    if(res == -1)
    {
        perror("listen():");
        exit(EXIT_FAILURE);
    }

    // TODO Read man page for accept4(). And possible errors.
    int white_fd = accept(listen_fd, nullptr, nullptr);
    if(white_fd == -1)
    {
        perror("accept():");
        exit(EXIT_FAILURE);
    }

    int black_fd = accept(listen_fd, nullptr, nullptr);
    if(black_fd == -1)
    {
        perror("accept():");
        exit(EXIT_FAILURE);
    }
}

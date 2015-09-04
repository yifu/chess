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

enum color last_color = color::black;

void process_login(int fd, struct login *login)
{
    printf("login={msg type=%d, username=%.*s}\n",
           login->msg_type,
           (int)sizeof(login->username), login->username);

    // TODO POLLOUT or not POLLOUT?
    struct login_ack login_ack;
    login_ack.msg_type = msg_type::login_ack;

    if(last_color == color::white)
    {
        login_ack.player_color = color::black;
        last_color = color::black;
    }
    else
    {
        login_ack.player_color = color::white;
        last_color = color::white;
    }

    int n = send(fd, &login_ack, sizeof(login_ack), 0);
    if(n == -1)
    {
        perror("send()");
        exit(EXIT_FAILURE);
    }
    printf("login ack sent!\n");
    fflush(stdout);
}

int find_opponent_fd(int fd)
{
    for(size_t i = 0; i < sz; i++)
    {
        if(fds[i].fd == listen_fd)
            continue;
        if(fds[i].fd == fd)
            continue;
        return fds[i].fd;
    }
    assert(false);
    return -1;
}

void process_move(int fd, struct move_msg *move_msg)
{
    printf("move_msg={src={row=%d,col=%d},dst={row=%d,col=%d}}\n",
           move_msg->src_row, move_msg->dst_col,
           move_msg->dst_row, move_msg->dst_col);
    int opponent_fd = find_opponent_fd(fd);
    assert(opponent_fd != -1);
    int n = send(opponent_fd, move_msg, sizeof(*move_msg), 0);
    if(n == -1)
    {
        perror("send()");
        exit(EXIT_FAILURE);
    }
}

void process_listen_fd(int fd)
{
    // TODO Read man page for accept4(). And possible errors.
    int new_fd = accept(fd, nullptr, nullptr);
    if(new_fd == -1)
    {
        perror("accept()");
        exit(EXIT_FAILURE);
    }
    printf("accepted fd = %d.\n", new_fd);
    fds[sz++] = {new_fd, POLLIN, 0};
}

void process_player_fd(int i, struct pollfd pollfd)
{
    assert(pollfd.revents & (POLLIN));

    int fd = pollfd.fd;
    char buf[1024];

    // TODO Replace with while(n = recv(...)) {...}
    int n = recv(fd, buf, sizeof(buf), 0);
    if(n == -1)
    {
        perror("recv()");
        exit(EXIT_FAILURE);
    }
    else if(n == 0)
    {
        printf("close fd = %d.\n", fd);
        int res = close(fd);
        if(res == -1)
        {
            perror("close()");
            exit(EXIT_FAILURE);
        }
        fds[i] = {-1,0,0};
    }
    else
    {
        enum msg_type type = (enum msg_type)buf[0];
        printf("recv=%d, msg_type=%d.\n", n, type);
        switch(type)
        {
        case msg_type::login:
            process_login(fd, (struct login*)buf);
            break;
        case msg_type::move_msg:
            process_move(fd, (struct move_msg*)buf);
            break;
        default:
            printf("Unknown msg type=%d.\n", type);
            break;
        }
    }
}

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
                    process_listen_fd(fd);
                }
                else
                {
                    process_player_fd(i, pollfd);
                }
                nfds--;
            }
        }

        // remove(fds, fds+sz, fd);
        // sz--;

    }
}

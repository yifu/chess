#include <stdio.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <vector>
#include <assert.h>
#include <unistd.h>
#include <algorithm>

#include "net_protocol.hpp"
#include "game.hpp"

using namespace std;

// TODO Handle SIGFPIPE

#define arraysize(array)    (sizeof(array)/sizeof(array[0]))

int listen_fd = -1;

// ulimit -n
// sysctl -w fs.file-max=100000
// cat /proc/sys/fs/file-nr
// getrlimit/setrlimit RLIMIT_NOFILE
struct pollfd fds[100]; // TODO MAX FD PER PROCESSUS?
size_t sz = 0;

vector<int/*fd*/> waiting_list;

enum color last_color = color::black;

struct srv_game
{
    struct game game;
    int white_fd = -1, black_fd = -1;
};

struct srv_game current_game;

void match_players()
{
    while(waiting_list.size() > 1)
    {
        int white_fd = waiting_list[0];
        int black_fd = waiting_list[1];

        struct srv_game new_game;
        new_game.game.pieces = initial_board;
        new_game.white_fd = white_fd;
        new_game.black_fd = black_fd;
        current_game = new_game;

        struct new_game_msg new_game_msg;
        new_game_msg.msg_type = msg_type::new_game_msg;
        new_game_msg.player_color = color::white;

        ssize_t n = send(white_fd, &new_game_msg, sizeof(new_game_msg),0);
        if(n == -1)
        {
            perror("send()");
            assert(false);
            exit(EXIT_FAILURE);
        }

        new_game_msg.player_color = color::black;
        n = send(black_fd, &new_game_msg, sizeof(new_game_msg),0);
        if(n == -1)
        {
            perror("send()");
            assert(false);
            exit(EXIT_FAILURE);
        }

        waiting_list.erase(waiting_list.begin(), waiting_list.begin()+2);
    }
}

void process_login(int fd, struct login *login)
{
    printf("login={msg type=%d, username=%.*s}\n",
           login->msg_type,
           (int)sizeof(login->username), login->username);

    // TODO POLLOUT or not POLLOUT?
    struct login_ack login_ack;
    login_ack.msg_type = msg_type::login_ack;

    waiting_list.push_back(fd);

    int n = send(fd, &login_ack, sizeof(login_ack), 0);
    if(n == -1)
    {
        perror("send()");
        assert(false);
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
    assert(current_game.white_fd != -1 && current_game.black_fd != -1);
    int opponent_fd = -1;
    if(fd == current_game.white_fd)
        opponent_fd = current_game.black_fd;
    else if(fd == current_game.black_fd)
        opponent_fd = current_game.white_fd;
    else
        assert(false);
    assert(opponent_fd != -1);

    struct move candidate_move = {{move_msg->src_row, move_msg->src_col},
                                  {move_msg->dst_row, move_msg->dst_col}};
    vector<struct move> valid_moves = next_valid_moves(current_game.game);
    auto found = find(valid_moves.begin(), valid_moves.end(), candidate_move);
    if(found != valid_moves.end())
    {
        current_game.game = apply_move(current_game.game, candidate_move);

        ssize_t n = send(opponent_fd, move_msg, sizeof(*move_msg), 0);
        if(n == -1)
        {
            perror("send()");
            assert(false);
            exit(EXIT_FAILURE);
        }

        if(next_valid_moves(current_game.game).size() == 0)
        {
            if(is_king_checked(current_game.game))
                printf("CHECKMATE!!\n");
            else
                printf("STALEMATE!!\n");
            // TODO The game end here. Mark it into the game. As a
            // result when we receive a new move from one of the
            // opponent we must check the party is still open and
            // reject any move.
        }
    }
    else
    {
        printf("reject move.\n");
        struct reject_move_msg reject_move_msg;
        reject_move_msg.msg_type = msg_type::reject_move_msg;
        ssize_t n = send(fd, &reject_move_msg, sizeof(reject_move_msg), 0);
        if(n == -1)
        {
            perror("send()");
            assert(false);
            exit(EXIT_FAILURE);
        }
    }
}

void process_listen_fd(int fd)
{
    // TODO Read man page for accept4(). And possible errors.
    int new_fd = accept(fd, nullptr, nullptr);
    if(new_fd == -1)
    {
        perror("accept()");
        assert(false);
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
        assert(false);
        exit(EXIT_FAILURE);
    }
    else if(n == 0)
    {
        printf("close fd = %d.\n", fd);
        int res = close(fd);
        if(res == -1)
        {
            perror("close()");
            assert(false);
            exit(EXIT_FAILURE);
        }

        int opponent_fd = -1;
        if(fd == current_game.white_fd)
        {
            opponent_fd = current_game.black_fd;
        }
        else if(fd == current_game.black_fd)
        {
            opponent_fd = current_game.white_fd;
        }

        if(opponent_fd != -1)
        {
            struct game_evt_msg msg;
            msg.msg_type = msg_type::game_evt_msg;
            msg.game_evt_type = game_evt_type::opponent_resigned;
            ssize_t n = send(opponent_fd, &msg, sizeof(msg), 0);
            if(n == -1)
            {
                perror("send()");
                assert(false);
                exit(EXIT_FAILURE);
            }
            waiting_list.push_back(opponent_fd);
        }

        fds[i] = {-1,0,0};

        auto end = remove(waiting_list.begin(), waiting_list.end(), fd);
        waiting_list.erase(end, waiting_list.end());

        current_game.white_fd = -1;
        current_game.black_fd = -1;
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
        assert(false);
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
        assert(false);
        exit(EXIT_FAILURE);
    }

    res = setsockopt(listen_fd, IPPROTO_TCP, TCP_NODELAY,
                     &optval, sizeof(optval));
    if(res == -1)
    {
        perror("setsockopt(TCP_NODELAY)");
        assert(false);
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
        assert(false);
        exit(EXIT_FAILURE);
    }

    res = listen(listen_fd, 10/*backlog*/);
    if(res == -1)
    {
        perror("listen()");
        assert(false);
        exit(EXIT_FAILURE);
    }

    while(true)
    {
        int nfds = poll(&fds[0], sz, -1/*timeout*/);
        if(nfds == -1)
        {
            perror("poll()");
            assert(false);
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

        match_players();

        // remove(fds, fds+sz, fd);
        // sz--;

    }
}

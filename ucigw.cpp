#include <iostream>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <sstream>
#include <assert.h>
#include <algorithm>
#include <vector>

#include "utils.hpp"
#include "game.hpp"
#include "net_protocol.hpp"

using namespace std;

extern FILE *dbg;

void exit_failure()
{
    exit(EXIT_FAILURE);
}

int connect_game_server(string ip, string port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1)
    {
        perror("socket()");
        exit_failure();
    }

    int optval = 1;
    int res = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                         &optval, sizeof(optval));
    if(res == -1)
    {
        perror("setsockopt(TCP_NODELAY)");
        exit_failure();
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port.c_str()));
    res = inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    if(res != 1)
    {
        perror("inet_pton()");
        exit_failure();
    }

    // TODO Handle SIGFPIPE.
    res = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if(res == -1)
    {
        perror("connect()");
        exit_failure();
    }

    string username = "gnuchess";

    struct login login;
    login.msg_type = msg_type::login;
    strncpy(login.username, username.c_str(), sizeof(login.username));
    res = send(fd, &login, sizeof(login), 0);
    if(res == -1)
    {
        perror("send()");
        exit_failure();
    }

    // TODO Move that recv() call into process_server_fd().
    struct login_ack login_ack;
    int n = recv(fd, &login_ack, sizeof(login_ack), 0);
    if(n == -1)
    {
        perror("recv()");
        exit_failure();
    }

    struct play_online_msg msg;
    msg.msg_type = msg_type::play_online;
    n = write(fd, &msg, sizeof(msg));
    if(n == -1)
    {
        perror("write()");
        exit_failure();
    }

    struct players_list_msg player_list;
    n = recv(fd, &player_list, sizeof(player_list), 0);
    if(n == -1)
    {
        perror("recv()");
        exit_failure();
    }

    return fd;
}

string square2ucistr(struct square square)
{
    string result;
    result.push_back(square.col + 'a');
    result.push_back(square.row + '1');
    return result;
}

string type2ucistr(piece_type t)
{
    string result;
    switch(t)
    {
    case piece_type::none:
        assert(false);
        break;
    case piece_type::pawn:
        assert(false);
        break;
    case piece_type::king:
        assert(false);
        break;
    case piece_type::queen:
        result = "q";
        break;
    case piece_type::knight:
        result = "n";
        break;
    case piece_type::bishop:
        result = "b";
        break;
    case piece_type::rook:
        result = "r";
        break;
    default:
        assert(false);
        break;
    }
    return result;
}

string move2ucistr(struct move move)
{
    string result;
    result += square2ucistr(move.src);
    result += square2ucistr(move.dst);
    if(move.promotion != piece_type::none)
        result += type2ucistr(move.promotion);
    return result;
}

string request_gnuchess_for_next_move(struct game game, int gnuchessfd)
{
    ostringstream cmd;
    cmd << "position fen ";
    vector<struct piece> pieces(begin(initial_board), end(initial_board));
    cmd << board2fen(pieces);

    if(not game.moves.empty())
    {
        cmd << "moves ";
        for(auto move : game.moves)
        {
            cmd << move2ucistr(move) << " ";
       }
    }

    cmd << "\n";
    // TODO Update times.
    cmd << "go wtime 122000 btime 120000 winc 2000 binc 2000\n";
    ssize_t n = write(gnuchessfd, cmd.str().c_str(), cmd.str().length());
    if(n == -1)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }

    string result, output;
    char buf[1024];
    while((n = read(gnuchessfd, buf, sizeof(buf))) != -1)
    {
        printf("Retrieved line of length %zu :\n", n);
        output += string(buf, n);
        size_t pos = output.find("bestmove");
        if(pos == string::npos)
            continue;
        if(output.length() < string("bestmove a1a1").length())
            continue;
        output.erase(0, pos);
        cout << output << endl;

        size_t beg = output.find_first_of(" \n");
        size_t end = output.find_first_of(" \n", beg+1);
        size_t len = end == (size_t)-1 ? -1 : end - beg - 1;
        printf("%lu %lu %lu\n", beg, end, len);
        result = output.substr(beg+1, len);
        assert(result != "");
        break;
    }

    assert(result != "");
    return result;
}

struct move uci2move(string move)
{
    printf("uci2move(\"%s\")\n", move.c_str());
    assert(move.size() == 4 || move.size() == 5);
    assert(move[0] >= 'a' && move[0] <= 'h');
    assert(move[2] >= 'a' && move[2] <= 'h');
    assert(move[1] >= '1' && move[1] <= '8');
    assert(move[3] >= '1' && move[3] <= '8');
    if(move.size() == 5)
    {
        bool found = false;
        for(char p : {'q', 'r', 'n', 'b'})
            if(p == move[4])
                found = true;
        assert(found);
    }
    struct move mv;
    mv.src.row = move[1] - '1';
    mv.src.col = move[0] - 'a';
    mv.dst.row = move[3] - '1';
    mv.dst.col = move[2] - 'a';
    mv.promotion = piece_type::none;
    printf("move=");
    print_move(mv);
    return mv;
}

struct move_msg uci2movemsg(string move)
{
    printf("uci2move(\"%s\")\n", move.c_str());
    assert(move.size() == 4 || move.size() == 5);
    assert(move[0] >= 'a' && move[0] <= 'h');
    assert(move[2] >= 'a' && move[2] <= 'h');
    assert(move[1] >= '1' && move[1] <= '8');
    assert(move[3] >= '1' && move[3] <= '8');
    if(move.size() == 5)
    {
        bool found = false;
        for(char p : {'q', 'r', 'n', 'b'})
            if(p == move[4])
                found = true;
        assert(found);
    }
    struct move_msg msg;
    msg.msg_type = msg_type::move_msg;
    msg.src_row = move[1] - '1';
    msg.src_col = move[0] - 'a';
    msg.dst_row = move[3] - '1';
    msg.dst_col = move[2] - 'a';
    printf("move={src_row=%d, src_col=%d, dst_row=%d, dst_col=%d}\n",
           msg.src_row, msg.src_col, msg.dst_row, msg.dst_col);
    char *p = (char*)&msg;
    for(size_t i = 0; i < sizeof(msg); i++)
    {
        printf("0x%X, ", *p);
        p++;
    }
    printf("\n");
    return msg;
}

struct game play_next_move(int fd, struct game game, int gnuchessfd)
{
    string new_move = request_gnuchess_for_next_move(game, gnuchessfd);
    fflush(stdout);
    struct move_msg msg = uci2movemsg(new_move);

    ssize_t n = write(fd, &msg, sizeof(msg));
    if(n == -1)
    {
        perror("write()");
        exit(EXIT_FAILURE);
    }

    struct move mv = uci2move(new_move);
    game = apply_move(game, mv);
    return game;
}

int main()
{
    printf("PID=%d.\n", getpid());
    // enum color player_color = color::white;
    struct game game;
    game.pieces = initial_board;

    int sv[2];
    int n = socketpair(AF_UNIX, SOCK_STREAM, 0/*protocol*/, sv);
    if(n == -1)
    {
        perror("socketpair");
        exit(EXIT_FAILURE);
    }
    assert(sv[0] != sv[1]);

    pid_t pid = fork();
    if(pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if(pid == 0)
    {
        close(0);
        close(1);
        close(2);
        close(sv[0]);
        sv[0] = 0;
        dup(sv[1]);
        dup(sv[1]);
        dup(sv[1]);

        char prog[] = "gnuchess";
        const char * arg[] = { prog, "--uci", NULL };
        execvp(prog, (char**)arg);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    else
    {
        close(sv[1]);
        sv[1] = 0;

        ostringstream cmd;
        cmd << "uci\n";
        cmd << "setoption name Hash value 32\n";
        cmd << "isready\n";
        cmd << "ucinewgame\n";
        ssize_t n = write(sv[0], cmd.str().c_str(), cmd.str().size());
        if(n == -1)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }
    }

    int fd = connect_game_server("195.154.72.36", "55555");

    while(true)
    {
        struct pollfd fds[] = {{fd, POLLIN, 0}};
        int nfds = poll(&fds[0], arraysize(fds), -1/*timeout*/);
        if(nfds == -1)
        {
            perror("poll()");
        }

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
            close(fd);
            exit(EXIT_SUCCESS);
        }
        else
        {
            enum msg_type type = (enum msg_type)buf[0];
            printf("recv=%d, msg_type=%d.\n", n, type);
            switch(type)
            {
            case msg_type::new_game_msg:
            {
                struct game new_game;
                game = new_game;
                game.pieces = initial_board;
                if(((new_game_msg*)buf)->player_color == color::white)
                    game = play_next_move(fd, game, sv[0]);

                char cmd[] = "ucinewgame\n";
                ssize_t n = write(sv[0], cmd, strlen(cmd));
                if(n == -1)
                {
                    perror("write");
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case msg_type::move_msg:
            {
                struct move_msg *move_msg = (struct move_msg*)buf;
                struct move candidate_move = {{move_msg->src_row, move_msg->src_col},
                                              {move_msg->dst_row, move_msg->dst_col},
                                              piece_type::none/*type::none*/};
                fprintf(dbg, "candidate move: ");
                print_move(candidate_move);

                vector<struct move> valid_moves = next_valid_moves(game);
                fprintf(dbg, "valid moves:\n");
                for(auto move : valid_moves)
                {
                    print_move(move);
                }
                fprintf(dbg, "\n");

                auto found = find(valid_moves.begin(), valid_moves.end(), candidate_move);
                assert(found != valid_moves.end());

                game = apply_move(game, candidate_move);
                // TODO Process end of game here and in play_next_move().
                game = play_next_move(fd, game, sv[0]);
                break;
            }
            default:
                printf("Unknown msg type=%d.\n", type);
                break;
            }

        }

        // TODO
        // 1- popen(). The cmd should be: "echo \"*full gnu chess input*\" | gnuchess -uci"
        // 2- Listen to gnuchess output. "grep" the bestmove line.
        // 3- Extract the new bestmove and translate it into a netprotocol's move.
        // 4- Apply it to the current game. Send it over the network.

//         string cmd = "cat << EOF |
// uci
// EOF
// gnuchess --uci
// ";
    }
}

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

string request_gnuchess_for_next_move(struct game game)
{
    ostringstream cmd;
    cmd << "cat << EOF | \n";
    cmd << "uci\n";
    cmd << "setoption name Hash value 32\n";
    cmd << "isready\n";
    cmd << "ucinewgame\n";
    cmd << "position fen ";
    cmd << game2fen(game);

    // TODO better to generate the board position from the
    // "initial_board"! And generate the moves history anyway...

    // if(not game.moves.empty())
    // {
    //     cmd << "moves ";
    //     for(auto move : game.moves)
    //     {
    //         cmd << move2ucistr(move) << " ";
    //    }
    // }

    cmd << "\n";
    cmd << "go wtime 122000 btime 120000 winc 2000 binc 2000\n";
    cmd << "EOF\n";
    cmd << "gnuchess --uci\n";

    printf("cmd [%s].\n", cmd.str().c_str());

    FILE *f = popen(cmd.str().c_str(), "r");
    if (f == NULL)
    {
        perror("popen()");
        exit(EXIT_FAILURE);
    }
    fflush(f);

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    string result;

    while ((read = getline(&line, &len, f)) != -1) {
        printf("Retrieved line of length %zu :\n", read);
        printf("%s\n", line);
        fflush(stdout);
        string l = line;
        if(l.find("bestmove") == string::npos)
            continue;
        cout << l << endl;

        size_t beg = l.find_first_of(" \n");
        size_t end = l.find_first_of(" \n", beg+1);
        size_t len = end == (size_t)-1 ? -1 : end - beg - 1;
        printf("%lu %lu %lu\n", beg, end, len);
        result = l.substr(beg+1, len);
        assert(result != "");
        break;
    }

    assert(result != "");
    free(line);
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

struct game play_next_move(int fd, struct game game)
{
    string new_move = request_gnuchess_for_next_move(game);
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
                    game = play_next_move(fd, game);
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
                game = play_next_move(fd, game);
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

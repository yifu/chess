#include <iostream>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <stdint.h>
#include <vector>
#include <algorithm>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "utils.hpp"
#include "game.hpp"

using namespace std;

bool quit = false;

SDL_Window *display = nullptr;
SDL_Renderer *ren = nullptr;

SDL_Surface *icon = nullptr;

SDL_Texture *white_pawn_texture = nullptr;
SDL_Texture *white_rook_texture = nullptr;
SDL_Texture *white_knight_texture = nullptr;
SDL_Texture *white_bishop_texture = nullptr;
SDL_Texture *white_queen_texture = nullptr;
SDL_Texture *white_king_texture = nullptr;

SDL_Texture *black_pawn_texture = nullptr;
SDL_Texture *black_rook_texture = nullptr;
SDL_Texture *black_knight_texture = nullptr;
SDL_Texture *black_bishop_texture = nullptr;
SDL_Texture *black_queen_texture = nullptr;
SDL_Texture *black_king_texture = nullptr;

int square_width, square_heigh;

SDL_Rect out_of_view_rect = { -square_width, -square_heigh, square_width, square_heigh };

bool operator != (SDL_Rect l, SDL_Rect r)
{
    return l.x != r.x || l.y != r.y || l.w != r.w || l.h != r.h;
}

struct square detect_square(Sint32 x, Sint32 y)
{
    // TODO If player is black, then the coordinates are inversed.

    struct square result;
    result.row = y / square_heigh;
    result.col = x / square_width;
    assert(result.row <= 7);
    assert(result.col <= 7);
    return result;
}

SDL_Rect square2rect(struct square square)
{
    assert(square.row <= 7);
    assert(square.col <= 7);

    // TODO A variable must tell what is the color of the end user of
    // this application instance. His color must appear bottom side.
    SDL_Rect rect;
    rect.x = square.col * square_width;
    rect.y = square.row * square_heigh;
    rect.w = square_width;
    rect.h = square_heigh;
    return rect;
}

struct sprite
{
    SDL_Texture *tex = nullptr;
    SDL_Rect rect = {0,0,0,0};
    bool is_dragged = false;
    size_t piece_pos = -1;
};

void print_timespec(struct timespec t)
{
    printf("tv_sec=%lu, tv_nsec=%lu.\n", t.tv_sec, t.tv_nsec);
}

void print_mouse_motion(SDL_Event e)
{
    printf("e.motion={type=%u,timestamp=%u,winid=%u,which=%u,state=%u,x=%d,y=%d,xrel=%d,yrel=%d}\n",
           e.motion.type,
           e.motion.timestamp,
           e.motion.windowID,
           e.motion.which,
           e.motion.state,
           e.motion.x,
           e.motion.y,
           e.motion.xrel,
           e.motion.yrel);
}

void print_mouse_button_event(SDL_Event e)
{
    printf("e.button={type=%u,timestamp=%u,winid=%u,which=%u,button=%d,state=%u,clicks=%d,x=%d,y=%d}\n",
           e.button.type,
           e.button.timestamp,
           e.button.windowID,
           e.button.which,
           e.button.button,
           e.button.state,
           e.button.clicks,
           e.button.x,
           e.button.y);
}

uint64_t substract_time(struct timespec l, struct timespec r)
{
    assert(l.tv_sec > r.tv_sec ||
           (l.tv_sec == r.tv_sec && l.tv_nsec > r.tv_nsec));
    assert(l.tv_nsec < 1000000000);
    assert(r.tv_nsec < 1000000000);

    uint64_t result = 0;
    if(r.tv_sec == r.tv_sec)
    {
        result = l.tv_nsec - r.tv_nsec;
    }
    else
    {
        assert(l.tv_sec > r.tv_sec);
        uint64_t sec = l.tv_sec - r.tv_sec;
        uint64_t nsec = 1000000000 * sec;
        nsec += l.tv_nsec;
        nsec += (1000000000 - r.tv_nsec);
        result = nsec;
    }
    return result;
}

void assertInvariants(vector<struct sprite> sprites, struct game game)
{
    int dragged_sprite_cnt = 0;
    for(auto sprite : sprites)
    {
        assert(sprite.piece_pos < game.pieces.size());
        struct piece piece = game.pieces[sprite.piece_pos];

        if(sprite.is_dragged)
        {
            assert(!piece.is_captured);
            assert(sprite.rect != out_of_view_rect);
            dragged_sprite_cnt++;
            continue;
        }

        if(piece.is_captured)
        {
            assert(sprite.rect == out_of_view_rect);
        }
        else
        {
            assert(sprite.rect != out_of_view_rect);
            SDL_Rect rect = square2rect(piece.square);
            assert(sprite.rect == rect);
        }
    }
    assert(dragged_sprite_cnt == 0 || dragged_sprite_cnt == 1);
}

void clean_up()
{
    if(icon)
        SDL_FreeSurface(icon);
    if(white_pawn_texture)
        SDL_DestroyTexture(white_pawn_texture);
    if(white_rook_texture)
        SDL_DestroyTexture(white_rook_texture);
    if(white_knight_texture)
        SDL_DestroyTexture(white_knight_texture);
    if(white_bishop_texture)
        SDL_DestroyTexture(white_bishop_texture);
    if(white_queen_texture)
        SDL_DestroyTexture(white_queen_texture);
    if(white_king_texture)
        SDL_DestroyTexture(white_king_texture);

    if(black_pawn_texture)
        SDL_DestroyTexture(black_pawn_texture);
    if(black_rook_texture)
        SDL_DestroyTexture(black_rook_texture);
    if(black_knight_texture)
        SDL_DestroyTexture(black_knight_texture);
    if(black_bishop_texture)
        SDL_DestroyTexture(black_bishop_texture);
    if(black_queen_texture)
        SDL_DestroyTexture(black_queen_texture);
    if(black_king_texture)
        SDL_DestroyTexture(black_king_texture);

    if(ren)
        SDL_DestroyRenderer(ren);

    if(display)
        SDL_DestroyWindow(display);
    SDL_Quit();
}

void exit_success()
{
    clean_up();
    exit(EXIT_SUCCESS);
}

void exit_failure()
{
    clean_up();
    exit(EXIT_FAILURE);
}

SDL_Texture *init_texture(string filename)
{
    SDL_Surface *surface = IMG_Load(filename.c_str());
    if(!surface) {
        cerr << "IMG_Load() error : " << IMG_GetError() << endl;
        exit_failure();
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(ren, surface);
    if(!texture){
        cerr << "SDL_CreateTextureFromSurface() error : " << SDL_GetError() << "." << endl;
        exit_failure();
    }
    SDL_FreeSurface(surface);
    return texture;
}

void init_textures()
{
    white_pawn_texture = init_texture("./Chess_plt60.png");
    white_rook_texture = init_texture("./Chess_rlt60.png");
    white_knight_texture = init_texture("./Chess_nlt60.png");
    white_bishop_texture = init_texture("./Chess_blt60.png");
    white_queen_texture =  init_texture("./Chess_qlt60.png");
    white_king_texture = init_texture("./Chess_klt60.png");

    black_pawn_texture = init_texture("./Chess_pdt60.png");
    black_rook_texture = init_texture("./Chess_rdt60.png");
    black_knight_texture = init_texture("./Chess_ndt60.png");
    black_bishop_texture = init_texture("./Chess_bdt60.png");
    black_queen_texture =  init_texture("./Chess_qdt60.png");
    black_king_texture = init_texture("./Chess_kdt60.png");
}

SDL_Texture *deduct_texture(struct piece piece)
{
    if(piece.color == color::white)
    {
        switch(piece.type)
        {
        case type::pawn: return white_pawn_texture;
        case type::rook: return white_rook_texture;
        case type::knight: return white_knight_texture;
        case type::bishop: return white_bishop_texture;
        case type::queen: return white_queen_texture;
        case type::king: return white_king_texture;
        }
    }
    else
    {
        switch(piece.type)
        {
        case type::pawn: return black_pawn_texture;
        case type::rook: return black_rook_texture;
        case type::knight: return black_knight_texture;
        case type::bishop: return black_bishop_texture;
        case type::queen: return black_queen_texture;
        case type::king: return black_king_texture;
        }
    }
    assert(false);
    return nullptr;
}

struct sprite init_sprite(size_t piece_pos, struct game game, SDL_Texture *texture)
{
    assert(piece_pos < game.pieces.size());
    assert(texture);

    struct piece piece = game.pieces[piece_pos];
    struct sprite sprite;
    sprite.tex = texture;
    sprite.rect = piece.is_captured ? out_of_view_rect : square2rect(piece.square);
    sprite.piece_pos = piece_pos;
    // print_rect(sprite.rect);
    return sprite;
}

vector<struct sprite> init_sprites(struct game game)
{
    vector<struct sprite> sprites;

    for(size_t i = 0; i < game.pieces.size(); i++)
    {
        SDL_Texture *texture = deduct_texture(game.pieces[i]);
        // TODO Refactor init_sprite(). It should take only a struct piece as a parameter.
        struct sprite sprite = init_sprite(i, game, texture);
        sprites.push_back(sprite);
    }
    // printf("init pieces=%lu, sprites = %lu.\n",
    //     pieces.size(), sprites.size());

    return sprites;
}

void process_input_events(vector<struct sprite>& sprites, struct game& game)
{
    SDL_Event e;

    if(SDL_WaitEvent(&e))
    {
        switch(e.type)
        {
        case SDL_QUIT:
        {
            printf("quit\n");
            quit = true;
            break;
        }
        case SDL_KEYDOWN:
        {
            printf("key down %d\n", e.key.keysym.sym);
            if(e.key.keysym.sym == SDLK_ESCAPE)
                quit = true;
            break;
        }
        case SDL_MOUSEMOTION:
        {
            // print_mouse_motion(e);
            bool found = false; // TODO int count?
            for(size_t i = 0; i < sprites.size(); i++)
            {
                if(sprites[i].is_dragged)
                {
                    assert(!found);
                    found = true;
                    // TODO Implement a function: updateDraggedXY() or updateDraggedSpritePos()...
                    sprites[i].rect.x = e.motion.x - square_width / 2;
                    sprites[i].rect.y = e.motion.y - square_heigh / 2;
                }
            }


            break;
        }
        case SDL_MOUSEBUTTONDOWN:
        {
            // print_mouse_button_event(e);
            for(size_t i = 0; i < sprites.size(); i++)
            {
                if(is_hitting_rect(sprites[i].rect, e.button.x, e.button.y))
                {
                    assert(sprites[i].piece_pos < game.pieces.size());
                    if(game.pieces[sprites[i].piece_pos].color != game.cur_player)
                        break;

                    sprites[i].rect.x = e.motion.x - square_width / 2;
                    sprites[i].rect.y = e.motion.y - square_heigh / 2;
                    sprites[i].is_dragged = true;
                }
            }
            break;
        }
        case SDL_MOUSEBUTTONUP:
        {
            // print_mouse_button_event(e);
            bool found = false;
            for(size_t i = 0; i < sprites.size(); i++)
            {
                if(sprites[i].is_dragged)
                {
                    assert(!found);
                    found = true;

                    assert(sprites[i].piece_pos < game.pieces.size());

                    struct piece& piece = game.pieces[sprites[i].piece_pos];
                    struct square src = piece.square;
                    struct square dst = detect_square(e.button.x, e.button.y);
                    struct move candidate_move = {src, dst};
                    // printf("Candidate: ");
                    // print_move(candidate_move);

                    // TODO Refactor the following. Must be moved into game.cpp file?

                    vector<struct move> valid_moves = next_valid_moves(game);
                    auto found = find(valid_moves.begin(), valid_moves.end(), candidate_move);
                    if(found != valid_moves.end())
                    {
                        // printf("found one move!\n");
                        // TODO We may overwrite with the candidate_game above.
                        game = apply_move(game, candidate_move);
                    }
                    // TODO Better to call that from the top of the
                    // event loop. In the paint_sprites() call. We may
                    // need to define a separated 'dragged piece'.
                    sprites = init_sprites(game);
                    // print_game(game);
                    if(next_valid_moves(game).size() == 0)
                    {
                        if(is_king_checked(game))
                            printf("CHECKMATE!!\n");
                        else
                            printf("STALEMATE!!\n");
                    }
                }
            }
            break;
        }
        default:
        {
            // printf("e.type=%d\n", e.type);
            break;
        }
        }
    }
}

void paint_chess_board()
{
    SDL_Rect dst;
    // TODO BIG REFACTORING. We must use the list from the game, and
    // re-generate the sprites from this list, at each turn of the
    // game loop.

    dst.x = dst.y = 0;
    dst.w = square_width;
    dst.h = square_heigh;

    for(int c = 0; c < 8; c++)
    {
        dst.x = 0;
        for(int r = 0; r < 8; r++)
        {
            if((c % 2 == 0 && r % 2 == 0) || (c % 2 == 1 && r % 2 == 1))
                SDL_SetRenderDrawColor(ren, 200, 200, 216, SDL_ALPHA_OPAQUE); // White square
            else
                SDL_SetRenderDrawColor(ren, 100, 100, 140, SDL_ALPHA_OPAQUE); // Dark square

            // printf("dst.x=%d, dst.y=%d, dst.w=%d, dst.h=%d.\n",
            //        dst.x, dst.y, dst.w, dst.h);
            SDL_RenderFillRect(ren, &dst);
            dst.x += dst.w;
        }
        dst.y += dst.h;
    }
}

void paint_sprites(const vector<struct sprite>& sprites)
{
    // printf("sprites.size()=%lu.\n", sprites.size());
    for(auto sprite : sprites)
    {
        if(sprite.is_dragged)
            continue;

        SDL_RenderCopy(ren, sprite.tex, nullptr, &sprite.rect);
    }

    for(auto sprite : sprites)
    {
        if(!sprite.is_dragged)
            continue;

        SDL_RenderCopy(ren, sprite.tex, nullptr, &sprite.rect);
    }
}

void initWindowIcon()
{
    icon = IMG_Load("./Chess_ndt60.png");
    if(!icon) {
        cerr << "IMG_Load() error : " << IMG_GetError() << endl;
        exit_failure();
    }
    // printf("Set Window icon.\n");
    SDL_SetWindowIcon(display, icon);
}

void paint_screen(vector<struct sprite> sprites)
{
    assert(display);

    SDL_SetRenderDrawColor(ren, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(ren);

    paint_chess_board();
    paint_sprites(sprites);

    SDL_RenderPresent(ren);
    SDL_UpdateWindowSurface(display);
}

void init_sdl()
{
    constexpr int screenwidth = 640;
    constexpr int screenheigh = 640;

    if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        cerr << "SDL_Init error: " << SDL_GetError() << "." << endl;
        exit_failure();
    }

    display = SDL_CreateWindow("Chess",
                               SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               screenwidth, screenheigh, SDL_WINDOW_HIDDEN);
    if(!display) {
        cerr << "SDL_CreateWindow() error : " << SDL_GetError() << "." << endl;
        exit_failure();
    }

    constexpr Uint32 renderer_flags = SDL_RENDERER_ACCELERATED;
    ren = SDL_CreateRenderer(display, -1/*index*/, renderer_flags);
    if(!ren) {
        cerr << "SDL_CreateRenderer() error :" << SDL_GetError() << "." << endl;
        exit_failure();
    }

    SDL_Rect viewport;
    SDL_RenderGetViewport(ren, &viewport);
    square_width = viewport.w / 8;
    square_heigh = viewport.h / 8;

    initWindowIcon();
    SDL_ShowWindow(display);

    init_textures();
}

int main()
{
    init_sdl();

    struct game game;
    game.pieces = initial_board;

    vector<struct sprite> sprites = init_sprites(game);

    while(!quit)
    {
        assertInvariants(sprites, game);
        process_input_events(sprites, game);
        assertInvariants(sprites, game);

        assertInvariants(sprites, game);
        paint_screen(sprites);
        assertInvariants(sprites, game);
    }
    printf("bye!\n");

    exit_success();
}

#include "utils.hpp"
#include <assert.h>

bool operator == (SDL_Rect l, SDL_Rect r)
{
    return l.x == r.x && l.y == r.y &&
	l.w == r.w && l.h == r.h;
}

bool is_hitting_rect(SDL_Rect rect, Sint32 x, Sint32 y)
{
    return x >= rect.x &&
        x <= rect.x + rect.w &&
        y >= rect.y &&
        y <= rect.y + rect.h;
}

void print_rect(SDL_Rect r)
{
    printf("r.x=%d, r.y=%d, r.w=%d, r.h=%d.\n",
           r.x, r.y, r.w, r.h);
}

uint64_t substract_time(struct timespec l, struct timespec r)
{
    assert(l.tv_sec > r.tv_sec ||
           (l.tv_sec == r.tv_sec && l.tv_nsec > r.tv_nsec));
    assert(l.tv_nsec < 1000000000);
    assert(r.tv_nsec < 1000000000);

    // printf("l.tv_sec=%" PRIu64 ", r.tv_sec=%" PRIu64 ".\n", l.tv_sec, r.tv_sec);

    uint64_t result = 0;
    if(l.tv_sec == r.tv_sec)
    {
        result = l.tv_nsec - r.tv_nsec;
    }
    else
    {
        assert(l.tv_sec > r.tv_sec);
        uint64_t sec = l.tv_sec - r.tv_sec - 1;
        uint64_t nsec = 1000000000 * sec;
        nsec += l.tv_nsec;
        nsec += (1000000000 - r.tv_nsec);
        result = nsec;
    }
    return result;
}

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

uint64_t to_uint64(struct timespec t)
{
    return t.tv_sec * 1000000000 + t.tv_nsec;
}

struct timespec operator - (struct timespec l, struct timespec r)
{
    assert(l.tv_sec > r.tv_sec ||
           (l.tv_sec == r.tv_sec && l.tv_nsec > r.tv_nsec));
    assert(l.tv_nsec < 1000000000);
    assert(r.tv_nsec < 1000000000);

    // printf("l.tv_sec=%" PRIu64 ", r.tv_sec=%" PRIu64 ".\n", l.tv_sec, r.tv_sec);

    timespec result;
    if(l.tv_sec == r.tv_sec)
    {
        result.tv_sec = 0;
        result.tv_nsec = l.tv_nsec - r.tv_nsec;
    }
    else
    {
        assert(l.tv_sec > r.tv_sec);
        result.tv_sec = l.tv_sec - r.tv_sec - 1;
        result.tv_nsec = l.tv_nsec;
        result.tv_nsec += (1000000000 - r.tv_nsec);
        if(result.tv_nsec >= 1000000000)
        {
            result.tv_sec++;
            result.tv_nsec -= 1000000000;
        }
    }
    assert(result.tv_nsec <= 1000000000);
    return result;
}

struct timespec operator + (struct timespec l, struct timespec r)
{
    struct timespec result;

    result.tv_sec = l.tv_sec+r.tv_sec;
    result.tv_nsec = l.tv_nsec+r.tv_nsec;
    if(result.tv_nsec >= 1000000000)
    {
        result.tv_sec++;
        result.tv_nsec -= 1000000000;
    }
    assert(result.tv_nsec < 1000000000);
    return result;
}

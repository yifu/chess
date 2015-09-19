#include <stdio.h>
#include <time.h>
#include "utils.hpp"
#include <assert.h>

using namespace std;

int main()
{
    struct timespec beg = {46869, 992649837};
    struct timespec end = {46872, 992649837};
    uint64_t result = to_uint64(end - beg);

    printf("result = %" PRIu64 ".\n", result);
    assert(result == 3000000000);

    beg = {46869, 992649837};
    end = {46872, 992649838};
    result = to_uint64(end - beg);
    printf("result = %" PRIu64 ".\n", result);
    assert(result == 3000000001);

    beg = {0,0};
    end = {0,900};
    result = to_uint64(beg+end);
    printf("result = %" PRIu64 ".\n", result);
    assert(result == 900);
}

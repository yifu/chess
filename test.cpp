#include <stdio.h>
#include <time.h>
#include "utils.hpp"
#include <assert.h>

using namespace std;

int main()
{
    struct timespec beg = {46869, 992649837};
    struct timespec end = {46872, 992649837};
    uint64_t result = substract_time(end, beg);

    printf("result = %" PRIu64 ".\n", result);
    assert(result == 3000000000);

    beg = {46869, 992649837};
    end = {46872, 992649838};
    result = substract_time(end, beg);
    printf("result = %" PRIu64 ".\n", result);
    assert(result == 3000000001);
}

#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>

uint64_t get_epoch_ms()
{
    uint64_t            ms; // Milliseconds
    time_t          s;  // Seconds
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);

    ms = spec.tv_sec*1000 + spec.tv_nsec/1000;
    ms = spec.tv_sec*1000;

    return ms;
}


int main()
{
    uint64_t first, last;
    uint64_t a, i;
    first = get_epoch_ms();

    printf("First: %"PRIu64"\n", first);
    for(i = 0; i < 10000000000; i++)
        a = 1;

    last =  get_epoch_ms();
    printf("Last: %"PRIu64"\n", last);
    printf("Difference: %"PRIu64"\n", last-first);


    return 0;
}
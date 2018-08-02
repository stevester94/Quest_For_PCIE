#include <stdint.h>

static uint64_t rdtsc(void)
{
	uint64_t var;
	uint32_t hi, lo;

	__asm volatile
	    ("rdtsc" : "=a" (lo), "=d" (hi));

	var = ((uint64_t)hi << 32) | lo;
	return (var);
}


int main()
{
	return rdtsc();
}

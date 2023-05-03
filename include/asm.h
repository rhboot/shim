// SPDX-License-Identifier: BSD-2-Clause-Patent

#ifndef SHIM_ASM_H_
#define SHIM_ASM_H_

#define __stringify_1(x...)     #x
#define __stringify(x...)       __stringify_1(x)

static inline uint64_t read_counter(void)
{
        uint64_t val;
#if defined (__x86_64__)
        unsigned long low, high;
        __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high));
        val = (low) | (high) << 32;
#elif defined(__i386__) || defined(__i686__)
        __asm__ __volatile__("rdtsc" : "=A" (val));
#elif defined(__aarch64__)
        __asm__ __volatile__ ("mrs %0, pmccntr_el0" : "=r" (val));
#elif defined(__arm__)
        __asm__ __volatile__ ("mrc p15, 0, %0, c9, c13, 0" : "=r" (val));
#else
#error unsupported arch
#endif
        return val;
}

#if defined(__x86_64__) || defined(__i386__) || defined(__i686__)
static inline void wait_for_debug(void)
{
	__asm__ __volatile__("pause");
}
#elif defined(__aarch64__)
static inline void wait_for_debug(void)
{
		__asm__ __volatile__("wfi");
}
#else
static inline void wait_for_debug(void)
{
        uint64_t a, b;
        int x;
        extern void usleep(unsigned long usecs);

        a = read_counter();
        for (x = 0; x < 1000; x++) {
                usleep(1000);
                b = read_counter();
                if (a != b)
                        break;
        }
}
#endif

#endif /* !SHIM_ASM_H_ */
// vim:fenc=utf-8:tw=75:et

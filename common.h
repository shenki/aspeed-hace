#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>

typedef uint32_t	u32;
typedef uint8_t		u8;

#define BIT(__x)	(1UL << (__x))

extern  void writel(u32 val, u32 addr);

extern u32 readl(u32 addr);

static void mdelay(int num)
{
}

static void udelay(int num)
{
}

#define readl_poll_timeout(addr, val, cond, timeout) \
({ \
 	while (1) { \
 		val = readl(addr); \
		printf("isr: %08x\n", val); \
		if (cond) \
			break; \
	} \
	(cond) ? 0 : -ETIMEDOUT; \
 })

#define debug(...) printf(__VA_ARGS__)

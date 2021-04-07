#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#define RAM_BASE 	((void *)0x90000000)
#define RAM_SIZE 	0x08000000
#define DEST_BUF 	(RAM_BASE + RAM_SIZE - 64)
#define SG_BUF_OFFSET 	(RAM_SIZE - 0x1000)
#define SG_BUF 		(RAM_BASE + SG_BUF_OFFSET)

typedef uint32_t	u32;
typedef uint8_t		u8;
typedef uint8_t		uchar;

#define USE_HOSTCC
#include "hash.h"
#include "hw_sha.h"

#define BIT(__x)	(1UL << (__x))

extern void writel(u32 val, u32 addr);
extern u32 readl(u32 addr);

#define mdelay(...) do {} while(0);
#define udelay(...) do {} while(0);

#define readl_poll_timeout(addr, val, cond, timeout) \
({ \
 	while (1) { \
 		val = readl(addr); \
		if (cond) \
			break; \
	} \
	(cond) ? 0 : -ETIMEDOUT; \
 })

#define debug(...) printf(__VA_ARGS__)

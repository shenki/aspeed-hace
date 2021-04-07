#include <err.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "aspeed_hace.h"

#define ASPEED_IO_BASE 0x10000000
#define ASPEED_IO_SZ   0x0f000000

void hw_sha1(const unsigned char *pbuf, unsigned int buf_len,
	       unsigned char *pout, unsigned int chunk_size);
void hw_sha256(const unsigned char *pbuf, unsigned int buf_len,
	       unsigned char *pout, unsigned int chunk_size);
void hw_sha512(const unsigned char *pbuf, unsigned int buf_len,
	       unsigned char *pout, unsigned int chunk_size);

static void *map;

uint32_t readl(uint32_t addr)
{
	return *(uint32_t *)(map + (addr & 0x0FFFFFFF));
}

void writel(uint32_t val, uint32_t addr)
{
	*(uint32_t *)(map + (addr & 0x0FFFFFFF)) = val;
}


static const uint8_t test_input_deadbeef[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF };

#define RAM_BASE 0x90000000
#define RAM_SIZE 0x08000000
#define DEST_BUF (RAM_BASE + RAM_SIZE - 64)

#define SG_BUF_OFFSET (RAM_SIZE - 0x1000)
#define SG_BUF (RAM_BASE + SG_BUF_OFFSET)
// sizeof 272

static int do_sha_sg(size_t length, void *buf) {
	struct hash_algo algo = { .name = "sha512"};
	void *ctx;
	int rc;
	rc = hw_sha_init(&algo, &ctx);
	if (rc) {
		printf("error in hw_sha_init: %s\n", strerror(rc));
		return -1;
	}
#if 0
	printf("Moving ctx from %p to %p\n", ctx, buf + SG_BUF_OFFSET);
	memcpy(buf + SG_BUF_OFFSET, ctx, sizeof(struct aspeed_hash_ctx));
	ctx = buf + SG_BUF_OFFSET;
#endif
	hw_sha_update(&algo, ctx, (void *)RAM_BASE, length, 0);
	if (rc) {
		printf("error in hw_sha_update: %s\n", strerror(rc));
		return -1;
	}
	hw_sha_update(&algo, ctx, (void *)RAM_BASE+length*2, length, 1);
	if (rc) {
		printf("error in hw_sha_update: %s\n", strerror(rc));
		return -1;
	}

	memcpy(buf + SG_BUF_OFFSET, ctx, sizeof(struct aspeed_hash_ctx));
	hw_sha_finish(&algo, ctx, (void *)DEST_BUF, 64);
	if (rc) {
		printf("error in hw_sha_finish: %s\n", strerror(rc));
		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	int i;
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	int file_fd;
	struct stat st;
	uint8_t *buf;
	const void *test_input;
	size_t length;

	if (0 && argc == 2) {
		printf("Using file '%s' as test vector\n", argv[1]);
		file_fd = open(argv[1], O_RDONLY);
		if (file_fd == -1)
			errx(1, "opening '%s' failed", argv[1]);
		if (fstat(file_fd, &st) == -1)
			errx(1, "fstat failed");
		test_input = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, file_fd, 0);
		if (test_input == MAP_FAILED)
			errx(1, "mmap of file failed");
		length = st.st_size;
	} else {
		printf("Using '0xDEADBEEF' as test vector\n");
		test_input = test_input_deadbeef;
		length = sizeof(test_input_deadbeef);
	}

	// map the io registers
	map = mmap(NULL, ASPEED_IO_SZ,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			fd, ASPEED_IO_BASE);
	if (map == MAP_FAILED)
		errx(1, "io registers map failed");

	// do bad things to dram
	buf = mmap(NULL, RAM_SIZE,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			fd, RAM_BASE);
	if (buf == MAP_FAILED)
		errx(1, "ram map failed");
	printf("copying %d bytes from %p to %p\n", length, test_input, buf);
	memcpy(buf, test_input, length);
	memcpy(buf+length, test_input, length);
	memcpy(buf+length*2, test_input, length);
	memcpy(buf+length*3, test_input, length);

	if (argc > 1) {
		if (strcmp(argv[argc - 1], "--sg") == 0) {
			printf("Running in scatter-gather mode.\n");
			do_sha_sg(length, buf);
		}
	} else {
		printf("Running in direct mode.\n");
		hw_sha512((void *)RAM_BASE, length, (void *)DEST_BUF, 1024);
	}

	printf("Result:\n");
	for (i = 0; i < 64; i++) {
		printf("%02x", buf[RAM_SIZE - 64 +i]);
	}
	puts("\n");

	return 0;
}

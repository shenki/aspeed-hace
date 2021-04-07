#include <err.h>
#include <fcntl.h>
#include <getopt.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "common.h"
#include "aspeed_hace.h"

#define ASPEED_IO_BASE 0x10000000
#define ASPEED_IO_SZ   0x0f000000

#define ROUND_UP(n, d) (((n) + (d) - 1) & -(0 ? (n) : (d)))

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

static void put_in_ram(void *dest, const void *src, size_t len)
{
	printf("Copying %d bytes from %p to %p\n", len, src, dest);
	memcpy(dest, src, len);
}

static const uint8_t test_input_deadbeef[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF };

static struct option options[] = {
	{"filename", 	required_argument, 	NULL, 'f'},
	{"sg", 		no_argument, 		NULL, 's'},
	{"help", 	no_argument, 		NULL, 'h'},
	{ 0 }
};

int main(int argc, char **argv)
{
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	int file_fd;
	uint8_t *buf;
	const void *test_input;
	const char *filename = NULL;
	bool sg_mode = false;
	size_t length;

	while (1) {
		int c = getopt_long(argc, argv, "f:sh", options, NULL);
		if (c == -1)
			break;
		switch (c) {
		case 'f':
			filename = optarg;
			break;
		case 's':
			sg_mode = true;
			break;
		default:
		case 'h':
			printf("usage: %s: [-f <filename>] [-s|--sg]\n", argv[0]);
			exit(0);
		}
	}

	if (filename) {
		struct stat st;
		printf("Using file '%s' as test vector: ", filename);
		file_fd = open(filename, O_RDONLY);
		if (file_fd == -1)
			errx(1, "opening '%s' failed", argv[1]);
		if (fstat(file_fd, &st) == -1)
			errx(1, "fstat failed");
		test_input = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, file_fd, 0);
		if (test_input == MAP_FAILED)
			errx(1, "mmap of file failed");
		length = st.st_size;
	} else {
		printf("Using '0xDEADBEEF' as test vector: ");
		test_input = test_input_deadbeef;
		length = sizeof(test_input_deadbeef);
	}
	printf("%d bytes\n", length);

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
			fd, (unsigned long)RAM_BASE);
	if (buf == MAP_FAILED)
		errx(1, "ram map failed");

	if (sg_mode) {
		printf("Running in scatter-gather mode.\n");

		struct hash_algo algo = { .name = "sha512"};
		void *ctx;

		size_t first = length / 2;
		size_t second = length - first;
		size_t offset = ROUND_UP(first, 8);

		printf("Two chunks: %d and %d\n", first, second);

		put_in_ram(buf, test_input, first);
		put_in_ram(buf+offset, ((uint8_t *)test_input)+second, second);

		hw_sha_init(&algo, &ctx);
		hw_sha_update(&algo, ctx, (void *)RAM_BASE, first, 0);
		hw_sha_update(&algo, ctx, (void *)RAM_BASE + offset, second, 1);

		printf("Coping sg table into ram\n");
		memcpy(buf + SG_BUF_OFFSET, ctx, sizeof(struct aspeed_hash_ctx));
		hw_sha_finish(&algo, ctx, (void *)DEST_BUF, 64);

	} else {
		printf("Running in direct mode.\n");
		put_in_ram(buf, test_input, length);
		hw_sha512((void *)RAM_BASE, length, (void *)DEST_BUF, 1024);
	}

	printf("Result:\n");
	for (int i = 0; i < 64; i++) {
		printf("%02x", buf[RAM_SIZE - 64 +i ]);
	}
	puts("\n");

	return 0;
}

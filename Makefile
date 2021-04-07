
CFLAGS	= -Wall -O3 -ggdb
CC	= arm-linux-gnueabi-gcc
LDFLAGS = -static

OBJ = aspeed_hace.o main.o

aspeed_hace: $(OBJ)

.PHONY:
upload: aspeed_hace
	strip aspeed_hace && sshpass -p 123123 scp aspeed_hace qemu:

.PHONY:
clean:
	$(RM) aspeed_hace $(OBJ)

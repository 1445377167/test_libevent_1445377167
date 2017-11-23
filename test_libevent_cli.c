#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
	int fd = 0;
	
	const char *str = "hello event";

	fd = open("event.fifo", O_RDWR);

	while (1) {
		write(fd, str, strlen(str));
		sleep(1);
	}

	close(fd);
	return 0;
}

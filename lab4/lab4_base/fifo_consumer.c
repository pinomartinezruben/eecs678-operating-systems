#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define FIFO_NAME "file.fifo"
#define MAX_LENGTH 1000

int main() {
    char str[MAX_LENGTH];
    int fd, num;

    // Create FIFO if it doesn't exist
    if (mkfifo(FIFO_NAME, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo");
            exit(1);
        }
    }

    printf("waiting for writers...");
    fflush(stdout);

    // Open FIFO for reading
    fd = open(FIFO_NAME, O_RDONLY);
    if (fd == -1) {
        perror("open for reading failed");
        exit(1);
    }

    printf("got a writer!\n");

    do {
        if ((num = read(fd, str, MAX_LENGTH)) == -1)
            perror("read");
        else {
            str[num] = '\0';
            printf("consumer: read %d bytes\n", num);
            printf("%s", str);
        }
    } while (num > 0);

    close(fd);
}
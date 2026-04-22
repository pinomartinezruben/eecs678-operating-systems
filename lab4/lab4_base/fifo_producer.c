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
        if (errno != EEXIST) {  // ignore "already exists" error
            perror("mkfifo");
            exit(1);
        }
    }

    printf("waiting for readers...");
    fflush(stdout);

    // Open FIFO for writing
    fd = open(FIFO_NAME, O_WRONLY);
    if (fd == -1) {
        perror("open for writing failed");
        exit(1);
    }

    printf("got a reader!\n");
    printf("Enter text to write in the FIFO file: ");
    fgets(str, MAX_LENGTH, stdin);

    while (!feof(stdin)) {
        if ((num = write(fd, str, strlen(str))) == -1)
            perror("write");
        else
            printf("producer: wrote %d bytes\n", num);

        fgets(str, MAX_LENGTH, stdin);
    }

    close(fd);
}
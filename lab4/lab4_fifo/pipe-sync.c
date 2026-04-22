/*
  pipe-sync.c: Use Pipe for Process Synchronization
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

// Question: Update this program to synchronize execution of the parent and
// child processes using pipes, so that we are guaranteed the following
// sequence of print messages:
// Child line 1
// Parent line 1
// Child line 2
// Parent line 2


int main() {
  char *s, buf[1024];
  int ret, stat;
  s  = "Use Pipe for Process Synchronization\n";
  /* create pipe */
  int p1[2];
  int p2[2];
  pipe(p1);
  pipe(p2);
  ret = fork();
  if (ret == 0) {
    close(p1[0]);
    close(p2[1]);
    /* child process. */
    printf("Child line 1\n");
    write(p1[1],"go", 2);
    read(p2[0], buf, sizeof(buf));
    printf("Child line 2\n");
    close(p1[1]);
    close(p2[0]);
  } else {
    close(p1[1]);
    close(p2[0]);
    /* parent process */
    read(p1[0], buf, sizeof(buf));
    printf("Parent line 1\n");
    write(p2[1], "go", 2);
    wait(&stat);
    printf("Parent line 2\n");
    close(p1[0]);
    close(p2[1]);
  }
}

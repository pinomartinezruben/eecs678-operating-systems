/*
 * Example of using mmap. Taken from Advanced Programming in the Unix
 * Environment by Richard Stevens.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h> /* mmap() is defined in this header */
#include <fcntl.h>
#include <unistd.h>
#include <string.h>  /* memcpy */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void err_quit (const char * mesg)
{
  printf ("%s\n", mesg);
  exit(1);
}

void err_sys (const char * mesg)
{
  perror(mesg);
  exit(errno);
}

int main (int argc, char *argv[])
{
  int fdin, fdout, i; //fd in, fd out, and i are integegers
  char *src, *dst, buf[256]; //the memory address of src, dst is type char, so is buf but its aray of size 256
  struct stat statbuf; //stat is a type now?

  src = dst = NULL;

  if (argc != 3)
    err_quit ("usage: memmap <fromfile> <tofile>");

  /* 
   * open the input file 
   */
  if ((fdin = open (argv[1], O_RDONLY)) < 0) {
    sprintf(buf, "can't open %s for reading", argv[1]);
    perror(buf);
    exit(errno);
  }

  /* 
   * open/create the output file 
   */
  if ((fdout = open (argv[2], O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) {
    sprintf (buf, "can't create %s for writing", argv[2]);
    perror(buf);
    exit(errno);
  }

  /* 
   * 1. find size of input file 
   */
  // fstat function takes in the input file descripter and the memory adress of the statbuf 
  int did_fstat_succeed = fstat(fdin, &statbuf); // 0=sucess, -1 = fail
  if (did_fstat_succeed == -1) {
    sprintf(buf, "cant stat %s", argv[1]);
    perror(buf);
    exit(errno);
  }
  /* 
   * 2. go to the location corresponding to the last byte 
   */
  // lseek function takes in file descripter output to stretch "the file", we use the size found in step 1
  off_t did_lseek_succeed = lseek(fdout, statbuf.st_size - 1, SEEK_SET);
  if (did_lseek_succeed == -1) { //-1 = fail, 0 = sucess
    perror("lseek error");
    exit(errno);
  }

  /* 
   * 3. write a dummy byte at the last location 
   */
  // fdout is the file given the dummy byte, "" is the single byte that is dummy, and 1 is the size of the byte
  ssize_t did_write_succeed = write(fdout, "" , 1); //-1 = fail, >1 = did write, 0 is wrote nothing
  if (did_write_succeed == -1) {
    perror("write error");
    exit(errno);
  }

  /* 
   * 4. mmap the input file 
   */
  src = mmap(NULL, statbuf.st_size, PROT_READ, MAP_SHARED, fdin, 0);
  if (src == MAP_FAILED) {
    perror("mmap error for input");
    exit(errno);
  }
  /* 
   * 5. mmap the output file 
   */
  dst = mmap(NULL, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fdout, 0);
  if (dst == MAP_FAILED) {
    perror("mmap error for output")
    exit(errno);
  }
  /* 
   * 6. copy the input file to the output file 
   */
    /* Memory can be dereferenced using the * operator in C.  This line
     * stores what is in the memory location pointed to by src into
     * the memory location pointed to by dest.
     */
  memcpy(dst,src,statbuf.st_size);

  // cleanup for good practice =)
  munmap(src, statbuf.st_size);
  munmap(dst, statbuf.st_size);
  close(fdin);
  close(fdout);
  return 0;
} 



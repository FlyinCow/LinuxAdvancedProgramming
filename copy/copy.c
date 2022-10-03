#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFSIZE 1024

void onerror(const char *msg) {
  perror(msg);
  exit(errno);
}

int main(int argc, const char **argv) {
  if (argc < 3) {
    printf("Usage:\n");
    printf("copy <source> <dest>\n");
    printf("Copy file to <dest> form <source>.");
    exit(1);
  }

  if (0 != access(argv[1], R_OK)) {
    onerror("Can't access source file.");
  };
  if (0 == access(argv[2], F_OK)) {
    onerror("Target exists.");
  }

  int sourcefd = open(argv[1], O_RDONLY);
  if (sourcefd == -1) {
    onerror("Can't open source file.");
  }

  struct stat stat;
  if (0 != lstat(argv[1], &stat)) {
    onerror("Can't get origin file states.");
  }

  // dir copy
  if (S_ISDIR(stat.st_mode)) {
  }

  int targetfd =
      open(argv[2], O_WRONLY | O_CREAT,
           S_IRWXU | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH);
  if (-1 == targetfd) {
    close(sourcefd);
    onerror("Can't create target file.");
  }

  u_char buf[BUFFSIZE];
  ssize_t n_chars = 0;
  ssize_t n_write = 0;

  while ((n_chars = read(sourcefd, buf, BUFFSIZE)) > 0) {
    n_write = write(targetfd, buf, n_chars);
    if (n_write != n_chars) {
      close(sourcefd);
      close(targetfd);
      onerror("Error.");
    }
  }

  return 0;
}
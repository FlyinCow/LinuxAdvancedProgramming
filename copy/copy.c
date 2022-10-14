#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFSIZE 1024
u_char buf[BUFFSIZE];

void onerror(const char *msg) {
  perror(msg);
  exit(errno);
}

int main(int argc, const char **argv) {

  ssize_t n_chars = 0;
  ssize_t n_write = 0;

  if (argc < 3) {
    printf("Usage:\n");
    printf("\tcopy <source> <dest>\n");
    printf("Copy <source> file as <dest>.\n");
    exit(1);
  }

  // 源文件不可读
  if (0 != access(argv[1], R_OK)) {
    onerror("Can't access source file.\n");
  };
  // 目标位置存在文件
  if (0 == access(argv[2], F_OK)) {
    onerror("Target exists.\n");
  }

  int sourcefd = open(argv[1], O_RDONLY);
  if (sourcefd == -1) {
    onerror("Can't open source file.\n");
  }

  struct stat stat;
  if (0 != lstat(argv[1], &stat)) {
    onerror("Can't get origin file states.\n");
  }

  // 非普通文件类型，不考虑
  if (!S_ISREG(stat.st_mode)) {
    close(sourcefd);
    onerror("Source file is not a regular file.\n");
  }

  // 目标文件fd
  int targetfd =
      open(argv[2], O_WRONLY | O_CREAT,
           S_IRWXU | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH // 权限644
      );
  if (-1 == targetfd) {
    close(sourcefd);
    onerror("Can't create target file.");
  }

  // 循环读取源文件，并写入目标文件
  while ((n_chars = read(sourcefd, buf, BUFFSIZE)) > 0) {
    n_write = write(targetfd, buf, n_chars);
    if (n_write != n_chars) {
      close(sourcefd);
      close(targetfd);
      onerror("Error writting target: write count mismatch.\n");
    }
  }

  return 0;
}
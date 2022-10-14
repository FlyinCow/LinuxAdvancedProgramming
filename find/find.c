#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// 日志开关
#define DBG_PRINT
#undef DBG_PRINT

#define chunk_size 256 // 在文件中查找时每次的步长
char buf[chunk_size];  // 文件内容buffer

// 错误处理
void onerror(const char *msg) {
  perror(msg);
  exit(errno);
}

/**
 * @brief 在 `fd` 指向的文件中查找 `substr` 是否存在
 *
 * @param fd
 * @param substr
 * @return 找到返回1，未找到返回0，发生错误返回-1
 */
int find_from_file(int fd, const char *substr) {
  ssize_t read_size;
  unsigned long substr_len = strlen(substr);
  while (0 != (read_size = read(fd, buf, chunk_size))) {
    // read出错
    if (read_size == -1) {
      return -1;
    }
#ifdef DBG_PRINT
    printf("\t%s\n", buf);
#endif
    // 找到了，返回
    if (NULL != strstr(buf, substr))
      return 1;
    // 没找到，但已经是最后一次读取
    if (read_size < chunk_size)
      return 0;
    /** 防止被查找字符串被两次读取分割
     *  e.g. "aabbccddeeffgghh" 中 查找 "ddee"，查找步长为8
     *  第一轮: [aabbccdd]eeffgghh // "ddee"被分割
     *  第二轮: aabbc[cddeeffg]ghh // 命中
     */
    lseek(fd, -(substr_len - 1), SEEK_CUR);
    // 没找到，继续读取
  }
  // 最后一次读取正好是chunk_size大小，会跳出循环，且没有找到
  return 0;
}

/**
 * @brief 遍历 `root` 目录查找内容有 `substr` 的文件
 *
 * @param root
 * @param substr
 */
void find_from_dir(const char *root, const char *substr) {
  char current_path[1024] = {0}; // 记录当前查找的文件名
  struct stat s;                 // 用于判断文件类型
  struct dirent *ent;            // 用于遍历目录
  int fd,                        // 用于打开文件
      res;                       // 记录查询结果

  unsigned long root_len = strlen(root);

  DIR *dir = opendir(root);

  // 打开失败，根据errno判断error类型。描述来源: opendir(3)--Linux manual page
  if (NULL == dir) {
    switch (errno) {
    case EACCES:
      onerror("Permission denied.");
      break;
    case EMFILE:
      onerror("The per-process limit on the number of open file descriptors "
              "has been reached.");
      break;
    case ENFILE:
      onerror("The system-wide limit on the total number of open files has "
              "been reached.");
      break;
    case ENOENT:
      onerror("Directory does not exist");
      break;
    case ENOMEM:
      onerror("Insufficient memory to complete the operation.");
      break;
    case ENOTDIR:
      onerror("Target is not a directory.");
      break;
    default:
      onerror("Error.");
    }
  }
  strncpy(current_path, root, root_len);
  strncpy(current_path + root_len, "/", 1);

  while (NULL != (ent = readdir(dir))) {
    // 跳过 ".", ".." 和隐藏文件
    if (strncmp(ent->d_name, ".", 1) == 0)
      continue;
    // 更新当前被检查的文件名
    strncpy(current_path + root_len + 1, ent->d_name, strlen(ent->d_name));
    strncpy(current_path + root_len + 1 + strlen(ent->d_name), "\0", 1);
    stat(current_path, &s);
    if (S_ISREG(s.st_mode)) { // 是普通文件，打开并查找
      // 防止先读长文件后读短文件，长文件的内容污染短文件结果
      memset(buf, 0, chunk_size);
      fd = open(current_path, O_RDONLY);
      if (-1 == fd) {
        continue;
      }
      res = find_from_file(fd, substr);
#ifdef DBG_PRINT
      printf("res of %s for %s(fd:%d) is %d\n", substr, current_path, fd, res);
#endif
      if (res == -1 || res == 0) {
        // error或未查找到
        continue;
      } else {
        // 查找到
        printf("%s\n", current_path);
      }
      close(fd);
    } else if (S_ISDIR(s.st_mode)) { // 是目录，递归查找
      find_from_dir(current_path, substr);
    } else { // 其他情况不考虑
      continue;
    }
  }
}

int main(int argc, const char **argv) {
  if (argc < 3) {
    printf("usage:\n");
    printf("\tfind <dir> <str>\n");
    return 0;
  }
  memset(buf, 0, chunk_size);
  find_from_dir(argv[2], argv[1]);
  return 0;
}
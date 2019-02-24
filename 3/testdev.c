#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

/* Cmd for ioctl */
#define SCULL_IOCTL_MEM_CLEAR 1
#define SCULL_IOCTL_MEM_SHOW 10
#define SCULL_MEM_SIZE 0x4000


int main(int argc, char **argv)
{
  int fd, start, num;
  char text[] = "Hello world!\n";
  char buf[SCULL_MEM_SIZE + 1];
  fd = open("/dev/scull", O_RDWR);
  if (fd < 0)
  {
    printf("Open error!\n");
    return 0;
  }
  ioctl(fd, SCULL_IOCTL_MEM_CLEAR, NULL);
  ioctl(fd, SCULL_IOCTL_MEM_SHOW, NULL);
  //lseek(fd, 0, SEEK_CUR);
  write(fd, "abc", strlen("abc"));
  //lseek(fd, 0, SEEK_CUR);
  write(fd, "abc", strlen("abc"));
  lseek(fd, 0, SEEK_SET);
  read(fd, buf, 6);
  buf[6] = '\0';
  printf("%s\n", buf);
  ioctl(fd, SCULL_IOCTL_MEM_SHOW, NULL);
  // if (argc == 4 && strncmp(argv[1], "read", 4) == 0)
  // {
  //   start = atoi(argv[2]);
  //   num = atoi(argv[3]);
  //   lseek(fd, start, SEEK_SET);
  //   read(fd, buf, num);
  //   printf("Read: %s\n", buf);
  // }
  // else if (argc == 4 && strncmp(argv[1], "write", 5) == 0)
  // {
  //   start = atoi(argv[2]);
  //   lseek(fd, start, SEEK_CUR);
  //   write(fd, argv[3], strlen(argv[3]));
  //   printf("Write succeed!\n");
  // }
  // else if (argc == 3 && strncmp(argv[1], "ioctl", 5) == 0)
  // {
  //   if (strncmp(argv[2], "show", 4) == 0)
  //   {
  //     ioctl(fd, SCULL_IOCTL_MEM_SHOW, NULL);
  //     printf("Use the dmesg command to check the memory.\n");
  //   }
  //   else if (strncmp(argv[2], "clear", 5) == 0)
  //   {
  //     ioctl(fd, SCULL_IOCTL_MEM_CLEAR, NULL);
  //     printf("Clear successfully.\n");
  //   }
  //   else
  //     print_usage();
  // }
  // else
  //   print_usage();

  close(fd);
  return 0;
}

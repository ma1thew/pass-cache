#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "common.h"

void usage(char* argv[]) {
  printf("usage: %s ENTRY\n", argv[0]);
  exit(1);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    usage(argv);
  }
  struct sockaddr_un addr;

  int sfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sfd == -1) {
    die("socket() failed!");
  }

  char* spath = get_socket_path();
  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, spath, sizeof(addr.sun_path) - 1);

  if (connect(sfd, (struct sockaddr *) &addr,
              sizeof(struct sockaddr_un)) == -1) {
    die("connect() failed!");
  }
  
  if (write(sfd, argv[1], strlen(argv[1]) + 1) != strlen(argv[1]) + 1) {
    die("write() failed!");
  }

  size_t num_read = 0;
  char buf[BUF_SIZE];
  while ((num_read = read(sfd, buf, BUF_SIZE)) > 0) {
    if (write(STDOUT_FILENO, buf, num_read) != num_read) {
      die("write() failed!");
    }
  }

  free_socket_path(spath);
  exit(EXIT_SUCCESS);
}

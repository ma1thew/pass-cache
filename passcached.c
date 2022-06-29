#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

#include "common.h"

#define BACKLOG 5

struct CacheEntry {
  struct CacheEntry* next;
  char* entry_name;
  char* secret;
};

struct CacheEntry cache;

volatile sig_atomic_t done = 0;

void term(int signum) {
  done = 1;
}

char* pass_secret_for_actual(char* entry_name, int* exit_code) {
  int link[2];
  pid_t pid;
  if (pipe(link) == -1) {
    die("pipe() failed!");
  }
  if ((pid = fork()) == -1) {
    die("fork() failed!");
  }
  if (pid == 0) {
    dup2 (link[1], STDOUT_FILENO);
    close(link[0]);
    close(link[1]);
    execlp("pass", "pass", entry_name, NULL);
    die("execlp() failed!");
  } else {
    if (close(link[1]) == -1) {
      fprintf(stderr, "%s\n", "close() failed!");
    }
    char* final = malloc(BUF_SIZE);
    if (!final) {
      die("malloc() failed!");
    }
    char* buf = malloc(BUF_SIZE);
    int nbytes = 0;
    while(0 != (nbytes = read(link[0], buf, sizeof(buf)))) {
      buf[nbytes] = 0;
      if (strlen(final) + strlen(buf) < BUF_SIZE) {
        strcat(final, buf);
      }
    }
    if (waitpid(pid, exit_code, 0) == -1) {
      fprintf(stderr, "%s\n", "waitpid() failed!");
      return NULL;
    } else {
      return final;
    }
  }
  // TODO
  *exit_code = 0;
  return "test";
}

char* pass_secret_for(char* entry_name, int* exit_code) {
  struct CacheEntry* current = &cache;
  while (current->next) {
    if (!strcmp(entry_name, current->next->entry_name)) {
      // cache hit
      *exit_code = 0;
      return current->next->secret;
    }
    current = current->next;
  }
  // cache miss
  char* secret = pass_secret_for_actual(entry_name, exit_code);
  if (secret && WIFEXITED(*exit_code) && (WEXITSTATUS(*exit_code) == 0)) {
    struct CacheEntry* new_entry = malloc(sizeof(struct CacheEntry));
    if (!new_entry) {
      die("malloc() failed!");
    }
    new_entry->next = NULL;
    char* new_entry_name = malloc(strlen(entry_name) + 1);
    if (!new_entry_name) {
      die("malloc() failed!");
    }
    strcpy(new_entry_name, entry_name);
    new_entry->entry_name = new_entry_name;
    new_entry->secret = secret;
    current->next = new_entry;
    return new_entry->secret;
  } else {
    return secret;
  }
}

void destroy_cache(void) {
  struct CacheEntry* current = cache.next;
  while (current) {
    struct CacheEntry* next = current->next;
    free(current->entry_name);
    free(current->secret);
    free(current);
    current = next;
  }
}

int main(int argc, char *argv[]) {
  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = term;
  sigaction(SIGTERM, &action, NULL);
  struct sockaddr_un addr;

  int sfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sfd == -1) {
    die("socket() failed!");
  }

  char* spath = get_socket_path();
  if (strlen(spath) > sizeof(addr.sun_path) - 1) {
    die("Server socket path too long!");
  }

  if (remove(spath) == -1 && errno != ENOENT) {
    die("remove() failed!");
  }

  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, spath, sizeof(addr.sun_path) - 1);

  if (bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1) {
    die("bind() failed!");
  }

  if (listen(sfd, BACKLOG) == -1) {
    die("listen() failed!");
  }

  printf("Listening at: %s\n", spath);

  cache.next = NULL;
  cache.entry_name = NULL;
  cache.secret = NULL;
  char buf[BUF_SIZE];
  while (!done) {
    int cfd = accept(sfd, NULL, NULL);
    printf("Accepted socket fd = %d\n", cfd);
    if (read(cfd, buf, BUF_SIZE) > 0) {
      int exit_code = 0;
      char* secret = pass_secret_for(buf, &exit_code);
      if (!secret || !(WIFEXITED(exit_code) && WEXITSTATUS(exit_code) == 0)) {
        secret = "";
      }
      if (write(cfd, secret, strlen(secret) + 1) != strlen(secret) + 1) {
        fprintf(stderr, "%s\n", "write() failed!");
      }
    }

    if (close(cfd) == -1) {
      fprintf(stderr, "%s\n", "close() failed!");
    }
  }
  close(sfd);
  destroy_cache();
  free_socket_path(spath);
}

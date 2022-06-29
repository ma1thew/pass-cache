#ifndef _COMMON_H_
#define _COMMON_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define BUF_SIZE 512
#define DEFAULT_SOCK_FNAME "/pass-cache.sock"

bool socket_path_did_alloc = false;

void die(char* message) {
  fprintf(stderr, "%s\n", message);
  exit(EXIT_FAILURE);
}

char* get_socket_path(void) {
  char* explicit_path = getenv("PASS_CACHE_SOCK");
  if (explicit_path) {
    return explicit_path;
  }
  char* base_path = getenv("XDG_RUNTIME_DIR");
  if (!base_path) {
    base_path = "/tmp";
  }
  char* final_path = (char*)malloc(strlen(base_path) + strlen(DEFAULT_SOCK_FNAME));
  socket_path_did_alloc = true;
  strcpy(final_path, base_path);
  strcat(final_path, DEFAULT_SOCK_FNAME);
  return final_path;
}

void free_socket_path(char* path) {
  if (socket_path_did_alloc) {
    free(path);
  }
}

#endif //ifndef _COMMON_H_

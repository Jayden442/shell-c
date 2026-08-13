#include "include/shell.h"

#define MAX_ARGS 32

bool keep_looping = true;

int getUserInput() {
  size_t len = 0;
  char *line = NULL;
  if (getline(&line, &len, stdin) < 0) {
    free(line);
    return 0;
  }
  else {
    char **args = build_array(line);
    if (args) {
      execute_command(args);
      for (int i = 0; args[i] != NULL; i++) {
        free(args[i]);
      }
      free(args);
    }
    free(line);
  }
  return 1;
}

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);
  
  while (keep_looping) {
    printf("$ ");
    getUserInput();
  }

  return 0;
}

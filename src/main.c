#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static keep_looping = true;

int check_if_exit(char *line) {
  if (strncmp(line, "exit", 4) == 0) {
    keep_looping = false;
    return 1;
  }
  return 0;
}

int invalidInput(char *line) {
  line[strcspn(line, "\n")] = '\0';
  printf("%s: command not found\n", line);
  free(line);
  return 1;
}

int getUserInput() {
  size_t len = 0;
  char *line = NULL;
  if (getline(&line, &len, stdin) < 0) {
    invalidInput("");
  }
  else {
    if (check_if_exit(line)) {
      return 0;
    }
    invalidInput(line);
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

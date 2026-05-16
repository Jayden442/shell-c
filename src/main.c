#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    invalidInput(line);
  }
  return 1;
}
int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);
  while (1) {
    printf("$ ");
    getUserInput();
  }

  return 0;
}

#include <stdio.h>
#include <stdlib.h>

int invalidInput(char *line) {
  printf("%s: Command not found\n", line);
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

  // TODO: Uncomment the code below to pass the first stage
  printf("$ ");
  getUserInput();

  return 0;
}

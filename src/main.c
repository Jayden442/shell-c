#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_ARGS 32

static bool keep_looping = true;

int exit_cmd(char **args);
int echo_cmd(char **args);

typedef int (*command_func)(char **args);
typedef struct {
  const char *name;
  command_func func;
} command_entry;

command_entry commands[] = {
  {"echo", echo_cmd},
  {"exit", exit_cmd},
};

int exit_cmd(char **args) {
  keep_looping = false;
  return 1;
}

int echo_cmd(char **args) {
  int index = 1;
  while (args[index] != NULL) {
    printf("%s", args[index]);
    if (args[index+1] == NULL) {
      break;
    }
    printf(" ");
  }
  printf("\n");
  return 1;
}

int invalid_input(char *line) {
  line[strcspn(line, "\n")] = '\0';
  printf("%s: command not found\n", line);
  free(line);
  return 1;
}

char **build_array(char *line) {
  char **args = malloc(sizeof(char *)*MAX_ARGS);

  if (args == NULL) {
    return NULL;
  }

  char *token = strtok(line, " \t\n");
  int index = 0;
  while (token != NULL && index < MAX_ARGS - 1) {
    args[index++] = token;
    token = strtok(NULL, " \t\n");
  }

  args[index] = NULL;
  return args;
}

int execute_command(char **args) {
  if (args[0] == NULL) {
    return 0;
  }

  int num_commands = sizeof(commands) / sizeof(commands[0]);

  for (int i = 0; i < num_commands; i++) {
    if (strcmp(args[0], commands[i].name) == 0) {
      return commands[i].func(args);
    }
  }
  invalid_input(args[0]);
  return 0;
}

int getUserInput() {
  size_t len = 0;
  char *line = NULL;
  if (getline(&line, &len, stdin) < 0) {
    invalid_input("");
  }
  else {
    char **args = build_array(line);
    execute_command(args);
    free(args);
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

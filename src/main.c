#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_ARGS 32

static bool keep_looping = true;

int exit_cmd(char **args);
int echo_cmd(char **args);
int type_cmd(char **args);
int pwd_cmd(char **args);

typedef int (*command_func)(char **args);
typedef struct {
  const char *name;
  command_func func;
} command_entry;

command_entry commands[] = {
  {"echo", echo_cmd},
  {"exit", exit_cmd},
  {"type", type_cmd},
  {"pwd", pwd_cmd},
};

char **parse_path() {
  char *path_env = getenv("PATH");
  char *path_copy = strdup(path_env);
  if (!path_copy) {
    return NULL;
  }
  // find out how many ':' in path
  int count = 0;
  for (char *tmp = path_copy; *tmp; tmp++) {
    if (*tmp == ':') {
      count++;
    }
  }
  count += 2;
  char **dirs = malloc(sizeof(char *)*count);
  if (!dirs) {
    free(path_copy);
    return NULL;
  }

  char *token = strtok(path_copy, ":");
  int index = 0;
  while (token != NULL) {
    dirs[index++] = strdup(token);
    token = strtok(NULL, ":");
  }
  dirs[index] = NULL;
  free(path_copy);
  return dirs;
}

int check_executables(char *dirname, char *filename, char **fullpath) {
  // append file path
  int size = strlen(dirname) + strlen(filename) + 2;
  (*fullpath) = malloc(size);
  if (!(*fullpath)) {
    free(*fullpath);
    return 0;
  }
  snprintf(*fullpath, size, "%s/%s", dirname, filename);
  struct stat st;
  if (stat(*fullpath, &st) == 0) {
    if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
      return 1;
    }
  }
  return 0;
}

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
    index++;
  }
  printf("\n");
  return 1;
}

int type_cmd(char **args) {
  int index = 1;
  bool found_builtin;
  int num_commands = sizeof(commands) / sizeof(commands[0]);
  while (args[index] != NULL) {
    found_builtin = false;
    for (int i = 0; i < num_commands; i++) {
      if (strcmp(args[index], commands[i].name) == 0) {
        printf("%s is a shell builtin\n", args[index]);
        found_builtin = true;
        break;
      }
    }
    if (!found_builtin) {
      // printf("%s: not found\n", args[index]);
      // tokenize first
      bool found_exe = false;
      char **dirnames = parse_path();
      for (int i = 0; dirnames[i]; i++) {
        char *fullpath;
        if (check_executables(dirnames[i], args[index], &fullpath)) {
          found_exe = true;
          printf("%s is %s\n", args[index], fullpath);
          free(fullpath);
          break;
        }
      }
      if (!found_exe) {
        printf("%s: not found\n", args[index]);
      }

      for (int i = 0; dirnames[i]; i++) {
        free(dirnames[i]);
      }
      free(dirnames);
    }
    index++;
  }
}

int pwd_cmd(char **args) {
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("%s\n", cwd);
    return 1;
  }
  else {
    printf("Failed to get working directory\n");
  }
  
}



int invalid_input(char *line) {
  line[strcspn(line, "\n")] = '\0';
  printf("%s: command not found\n", line);
  free(line);
  return 1;
}

char **build_array(char *line) {
  char **args = malloc(sizeof(char *)*MAX_ARGS);

  if (!args) {
    free(args);
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

int execute_external(char **args) {
  pid_t pid = fork();
  if (pid == 0) {
    execvp(args[0], args);
    exit(1);
  }
  else {
    wait(NULL);
  }
  return 1;
}

int execute_command(char **args) {
  if (args[0] == NULL) {
    return 0;
  }

  int num_commands = sizeof(commands) / sizeof(commands[0]);
  bool found_builtin = false;
  for (int i = 0; i < num_commands; i++) {
    if (strcmp(args[0], commands[i].name) == 0) {
      found_builtin = true;
      return commands[i].func(args);
    }
  }
  if (!found_builtin) {
      // printf("%s: not found\n", args[index]);
      // tokenize first
      bool found_exe = false;
      char **dirnames = parse_path();
      for (int i = 0; dirnames[i]; i++) {
        char *fullpath;
        if (check_executables(dirnames[i], args[0], &fullpath)) {
          found_exe = true;
          for (int i = 0; dirnames[i]; i++) {
            free(dirnames[i]);
          }
          free(dirnames);
          // printf("%s is %s\n", args[index], fullpath);
          free(fullpath);
          execute_external(args);
          break;
        }
      }
      if (!found_exe) {
        invalid_input(args[0]);
      }
    }
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

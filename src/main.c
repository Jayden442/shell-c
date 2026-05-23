#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_ARGS 32

static bool keep_looping = true;

int get_num_args(char **args);

int exit_cmd(char **args);
int echo_cmd(char **args);
int type_cmd(char **args);
int pwd_cmd(char **args);
int cd_cmd(char **args);
void quote_pairs(int *low, int *high, int *start, char *text);

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
  {"cd", cd_cmd},
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

int cd_cmd(char **args) {
  int num_args = get_num_args(args);
  if (num_args > 2) {
    printf("cd: Too many arguments\n");
    return 0;
  }
  if (num_args == 1) {
    return 1;
  }
  int arg_len = strlen(args[1]);

  char *homedir = getenv("HOME");
  int home_len = strlen(homedir);
  char cwd[arg_len + home_len+1];
  // tilde expansion
  if (args[1][0] == '~') {
    strncpy(cwd, homedir, home_len);
    strncpy(cwd+home_len-1, args[1]+1, arg_len-1);
    cwd[arg_len+home_len] = '\0';
  }
  else {
    strncpy(cwd, args[1], arg_len);
    cwd[arg_len] = '\0';
  }
  
  if (chdir(cwd) != 0) {
    printf("cd: %s: No such file or directory\n", cwd);
    return 0;
  }
  return 1;
}

int get_num_args(char **args) {
  int index = 0;
  while (args[index] != NULL) {
    if (args[index+1] == NULL) {
      break;
    }
    index++;
  }
  return index+1;
}

int invalid_input(char *line) {
  line[strcspn(line, "\n")] = '\0';
  printf("%s: command not found\n", line);
  free(line);
  return 1;
}

char **build_array(char *line) {
    char **args = malloc(sizeof(char *) * MAX_ARGS);

    if (!args) {
        return NULL;
    }

    int arg_count = 0;

    int quote_start, quote_end, next_quote;
    quote_pairs(
        &quote_start,
        &quote_end,
        &next_quote,
        line
    );

    char *token_start = NULL;

    for (int i = 0;; i++) {
        int in_quotes =
            (quote_start != -1 &&
             i >= quote_start &&
             i <= quote_end);

        char c = line[i];

        /* Start of a token */
        if (token_start == NULL &&
            c != '\0' &&
            !(isspace(c) && !in_quotes)) {

            token_start = &line[i];
        }

        /* End of token:
           whitespace outside quotes or string end */
        if (token_start &&
            ((isspace(c) && !in_quotes) ||
             c == '\0')) {

            if (c != '\0')
                line[i] = '\0';

            args[arg_count++] = token_start;

            if (arg_count >= MAX_ARGS - 1)
                break;

            token_start = NULL;
        }

        /* Move to next quote pair if needed */
        if (i == quote_end && next_quote != -1) {
            quote_pairs(
                &quote_start,
                &quote_end,
                &next_quote,
                line
            );
        }

        if (c == '\0')
            break;
    }

    args[arg_count] = NULL;

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
      //return commands[i].func(args);
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

int parse_args(char **args) {
  int index = 0;
  while (args[index] != NULL) {
    char *newStr = malloc(2*strlen(args[index]) + 1); // gotta clean up

    int low = -1;
    int high = -1;
    int start = 0;
    int newStrExtra = 0;
    for (int i = 0; i < strlen(args[index]); i++) {
      if (i == high+1) {
        quote_pairs(&low, &high, &start, args[index]);
      }
      newStr[i+newStrExtra] = args[index][i];
      if (args[index][i] == 92 && i <= high && i >= low) {
        newStrExtra++;
        newStr[i+newStrExtra]; // ignore escape characters
      }
    }
    strncpy(args[index], newStr, strlen(newStr));
    args[index][strlen(newStr)] = '\0';
    index++;
  }
}

// returns the index which it ended
void quote_pairs(int *low, int *high, int *start, char *text) {
  *low = -2;
  *high = -2;
  for (int i = *start; i < strlen(text); i++) {
    if (text[i] == 39) {
      if (*low == -2) {
        *low = i;
      }
      else if (*high == -2) {
        *high = i;
      }
      else {
        *start = i;
        return;
      }
    }
  }
  *start = strlen(text);
}

int getUserInput() {
  size_t len = 0;
  char *line = NULL;
  if (getline(&line, &len, stdin) < 0) {
    invalid_input("");
  }
  else {
    char **args = build_array(line);
    parse_args(args);
    execute_command(args);
    int index = 0;
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

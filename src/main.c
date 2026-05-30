#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>

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
  printf("%s", cwd);
  
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
    char **args = malloc(MAX_ARGS * sizeof(char *));
    if (!args) {
        return NULL;
    }

    int arg_count = 0;
    int in_quotes = 0;
    int in_double_quotes = 0;
    int i = 0;

    while (line[i] != '\0') {

        /* Skip whitespace between arguments */
        while (isspace(line[i])) {
            i++;
        }

        if (line[i] == '\0') {
            break;
        }

        /* Worst-case token size = remaining string */
        char *token = malloc(strlen(&line[i]) + 1);

        if (!token) {
            for (int j = 0; j < arg_count; j++) {
                free(args[j]);
            }
            free(args);
            return NULL;
        }

        int j = 0;

        while (line[i] != '\0') {

            /* Toggle quote state */
            if (!in_quotes && line[i] == '\"') {
                in_double_quotes = !in_double_quotes;
                i++;
                continue; /* don't copy quote chars */
            }
            else if (!in_double_quotes && line[i] == '\'') {
                in_quotes = !in_quotes;
                i++;
                continue; /* don't copy quote chars */
            }

            // escaping outside quotes
            if (!in_quotes && !in_double_quotes) {
              if (line[i] == '\\') {
                if (line[i+1] == '\0') {
                  break;
                }
                token[j] = line[i+1];
                j++;
                i = i + 2;
                continue;
              }
            }

            // escape double quotes
            if (in_double_quotes && !in_quotes && line[i] == '\\') {
              if (line[i+1] == '\"' || line[i+1] == '\\') {
                if (line[i+1] == '\0') {
                  break;
                }
                token[j] = line[i+1];
                j++;
                i = i + 2;
                continue;
              }
            }


            /* End token only on whitespace outside quotes */
            if ((!in_quotes && !in_double_quotes) && isspace(line[i])) {
                break;
            }

            token[j++] = line[i++];
        }

        token[j] = '\0';

        args[arg_count++] = token;

        if (arg_count >= MAX_ARGS - 1) {
            break;
        }
    }

    args[arg_count] = NULL;

    return args;
}

int execute_external(char **args, char *outfile, int redirect_stdout) {
  pid_t pid = fork();
  if (pid == 0) {
    if (redirect_stdout) {
      int fd = open(
        outfile,
        O_WRONLY | O_CREAT | O_TRUNC,
        0644
      );
      if (fd < 0) {
        printf("Error open");
        exit(1);
      }
      
      dup2(fd, STDOUT_FILENO);
      close(fd);
    }
    execvp(args[0], args);
    exit(1);
  }
  waitpid(pid, NULL, 0);
  return 1;
}

int builtin_redirection(char **args, char *outfile) {
  int saved_stdout = -1;
  int fd = -1;

  if (outfile != NULL) {
    saved_stdout = dup(STDOUT_FILENO); // duplicate stdout
    if (saved_stdout == -1) {
      printf("Error dup");
      return -1;
    }
    fd = open( // open file
      outfile,
      O_WRONLY | O_CREAT | O_TRUNC,
      0644
    );

    if (fd == -1) {
      printf("Error open");
      close(saved_stdout);
      return -1;
    }

    if (dup2(fd, STDOUT_FILENO) == -1) { // stdout now goes to the file
      printf("Error dup2");
      close(fd);
      return -1;
    }
  }
  return saved_stdout;
}

int restore_fds(char *outfile, int saved_stdout) {
  if (outfile != NULL) {
    if (dup2(saved_stdout, STDOUT_FILENO) == -1) {
      printf("Error dup2");
    }
    close(saved_stdout);
  }
  return 1;
}

int execute_builtin(char **args, char *outfile, int redirect_stdout) {
  int num_commands = sizeof(commands) / sizeof(commands[0]);
  bool found_builtin = false;
  int saved_stdout = 0;
  for (int i = 0; i < num_commands; i++) {
    if (strcmp(args[0], commands[i].name) == 0) {
      if (redirect_stdout && outfile) {
        int saved_stdout = builtin_redirection(args, outfile);
      }
      commands[i].func(args);
      if (redirect_stdout && outfile) {
        restore_fds(outfile, saved_stdout);
      }
      return 1;
    }
  }
  return -1;
}

int execute_command(char **args) {
  if (args[0] == NULL) {
    return 0;
  }
  int index = 0;
  char *outfile = NULL;
  int redirect_stdout = -1;
  while (args[index]) {
    if (strcmp(">", args[index]) == 0 || strcmp("1>", args[index]) == 0) {
      outfile = args[index+1];
      redirect_stdout = 1;
      // printf("outfile: %s redirect: %d\n", outfile, redirect_stdout);
      break;
    }
    index++;
  }
  if (execute_builtin(args, outfile, redirect_stdout) == -1) {
    printf("not builtin");
      // printf("%s: not found\n", args[index]);
      // tokenize first
      bool found_exe = false;
      char **dirnames = parse_path();
      for (int i = 0; dirnames[i]; i++) {
        char *fullpath;
        if (check_executables(dirnames[i], args[0], &fullpath)) {
          found_exe = true;
          for (int j = 0; dirnames[j]; j++) {
            free(dirnames[j]);
          }
          free(dirnames);
          // printf("%s is %s\n", args[index], fullpath);
          free(fullpath);
          execute_external(args, outfile, redirect_stdout);
          break;
        }
      }
      if (!found_exe) {
        invalid_input(args[0]);
      }
    }
  return 0;
}

void printargs(char **args) {
  int index = 0;
  while (args[index]) {
    printf("index %d, %s ", index, args[index]);
    index++;
  }
  printf("\n");
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

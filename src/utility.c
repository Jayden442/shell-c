#include "include/shell.h"

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

int remove_arg(char **args, int index, int count) {
  for (int i = index; args[i] != NULL; i++) {
    args[i] = args[i + count];
  }
  return 1;
}

void printargs(char **args) {
  int index = 0;
  while (args[index]) {
    printf("index %d, %s ", index, args[index]);
    index++;
  }
  printf("\n");
}

int invalid_input(char *line) {
  char *copy = strdup(line);
  if (!copy) {
    printf("command not found\n");
    return 1;
  }
  copy[strcspn(copy, "\n")] = '\0';
  printf("%s: command not found\n", copy);
  free(copy);
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

int restore_fds(char *outfile, int saved_stdout, int redirect) {
  if (outfile != NULL) {
    if (dup2(saved_stdout, redirect) == -1) {
      printf("Error dup2");
    }
    close(saved_stdout);
  }
  return 1;
}

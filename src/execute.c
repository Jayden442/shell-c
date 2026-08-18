#include "include/shell.h"
#include "include/commands.h"

fileio streams[] = {
  {">", STDOUT_FILENO},
  {"1>", STDOUT_FILENO},
  {"2>", STDERR_FILENO},
  {">>", STDOUT_FILENO},
  {"1>>", STDOUT_FILENO},
  {"2>>", STDERR_FILENO},
};

extern const int num_commands;
extern builtin_redirection(char **args, char *outfile, int redirect);
extern builtin_append_redirection(char **args, char *outfile, int redirect);

int execute_builtin(char **args, char *outfile, int redirect, int append) {
  bool found_builtin = false;
  int saved_stdout = -1;
  for (int i = 0; i < num_commands; i++) {
    if (strcmp(args[0], commands[i].name) == 0) {
      if (redirect > -1 && outfile) {
        if (append > -1) {
          saved_stdout = builtin_append_redirection(args, outfile, redirect);
        }
        else {
          saved_stdout = builtin_redirection(args, outfile, redirect);
        }
      }
      commands[i].func(args);
      if (redirect > -1 && outfile) {
        restore_fds(outfile, saved_stdout, redirect);
      }
      return 1;
    }
  }
  return -1;
}

int execute_external(char **args, char *outfile, int redirect) {
  pid_t pid = fork();
  if (pid == 0) {
    if (redirect > -1) {
      int fd = open(
        outfile,
        O_WRONLY | O_CREAT | O_TRUNC,
        0644
      );
      if (fd < 0) {
        printf("Error open");
        exit(1);
      }
      
      dup2(fd, redirect);
      close(fd);
    }
    execvp(args[0], args);
    exit(1);
  }
  waitpid(pid, NULL, 0);
  return 1;
}

int execute_command(char **args) {
  if (args[0] == NULL) {
    return 0;
  }
  int index = 0;
  char *outfile = NULL;
  int redirect = -1;
  int append = -1;
  while (args[index]) {
    int num_file_streams = sizeof(streams) / sizeof(streams[0]);
    for (int i = 0; i < num_file_streams; i++) {
      if (strcmp(streams[i].string, args[index]) == 0) {
        redirect = streams[i].fileStream;
        if (i >= 3) { // append redirection
          append = 1;
        }
      }
    }
    if (redirect != -1) {
      outfile = args[index+1];
      remove_arg(args, index, 2); // remove '>' and the filename
      break;
    }
    index++;
  }
  if (execute_builtin(args, outfile, redirect, append) == -1) {
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
          execute_external(args, outfile, redirect);
          break;
        }
      }
      if (!found_exe) {
        invalid_input(args[0]);
      }
    }
  return 0;
}

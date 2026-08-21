#include "include/shell.h"
#include "include/commands.h"

command_entry commands[] = {
  {"echo", echo_cmd},
  {"exit", exit_cmd},
  {"type", type_cmd},
  {"pwd", pwd_cmd},
  {"cd", cd_cmd},
};

const int num_commands = sizeof(commands) / sizeof(commands[0]);

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
  return 1;
}

int pwd_cmd(char **args) {
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("%s\n", cwd);
    return 1;
  }
  else {
    printf("Failed to get working directory\n");
    return 0;
  }
  return 1;
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

int builtin_redirection(char **args, char *outfile, int redirect) {
  int saved_stdout = -1;
  int fd = -1;

  if (outfile != NULL) {
    saved_stdout = dup(redirect); // duplicate 
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

    if (dup2(fd, redirect) == -1) { // stdout now goes to the file
      printf("Error dup2");
      close(fd);
      return -1;
    }
    close(fd);
  }
  return saved_stdout;
}

int builtin_append_redirection(char **args, char *outfile, int redirect) {
  int saved_stdout = -1;
  int fd = -1;

  if (outfile != NULL) {
    saved_stdout = dup(redirect); // duplicate 
    if (saved_stdout == -1) {
      printf("Error dup");
      return -1;
    }
    fd = open( // open file
      outfile,
      O_WRONLY | O_CREAT | O_APPEND,
      0644
    );

    if (fd == -1) {
      printf("Error open");
      close(saved_stdout);
      return -1;
    }

    if (dup2(fd, redirect) == -1) { // stdout now goes to the file
      printf("Error dup2");
      close(fd);
      return -1;
    }
    close(fd);
  }
  return saved_stdout;
}

char *command_generator(const char *text, int state)
{
    static int index;
    static const char *commands[] = {
        "cd",
        "echo",
        "exit",
        "pwd",
        "type",
        NULL
    };

    if (!state) {
        index = 0;
    }

    while (commands[index] != NULL) {
        const char *command = commands[index++];

        if (strncmp(command, text, strlen(text)) == 0) {
            return strdup(command);
        }
    }

    return NULL;
}

char **completion_function(const char *text, int start, int end)
{
    if (start == 0) {
        char **matches = rl_completion_matches(text, command_generator);

        if (matches == NULL) {
            write(STDOUT_FILENO, "\a", 1);
        }
        return matches;
    }

    return NULL;
}

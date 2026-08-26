#ifndef SHELL_H
#define SHELL_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_ARGS 32

extern bool keep_looping;

typedef struct {
  const char *string;
  int fileStream;
} fileio;

typedef struct {
  int job_id;
  pid_t pid;
  char *command;
  bool running;
} job;

extern bool keep_looping;
extern job jobs_list[64];
extern int num_jobs;

char **parse_path(void);
int get_num_args(char **args);
char **build_array(char *line);
int invalid_input(char *line);

int exit_cmd(char **args);
int echo_cmd(char **args);
int type_cmd(char **args);
int pwd_cmd(char **args);
int cd_cmd(char **args);
int jobs_cmd(char **args);
int builtin_redirection(char **args, char *outfile, int redirect);
int builtin_append_redirection(char **args, char *outfile, int redirect);

int execute_command(char **args);
int execute_external(char **args, char *outfile, int redirect, int background);
int execute_builtin(char **args, char *outfile, int redirect, int append, int background);

char *command_generator(const char *text, int state);
char **completion_function(const char *text, int start, int end);

#endif

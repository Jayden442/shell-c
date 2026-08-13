typedef int (*command_func)(char **args);
typedef struct {
    const char *name;
    command_func func;
} command_entry;

extern command_entry commands[];
extern const int num_commands;
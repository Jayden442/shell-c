#include "include/shell.h"

#define MAX_ARGS 32

bool keep_looping = true;

int getUserInput() {
    char *line = readline("$ ");

    if (line == NULL) {
        return 0;
    }

    char **args = build_array(line);

    if (args) {
        execute_command(args);

        for (int i = 0; args[i] != NULL; i++) {
            free(args[i]);
        }

        free(args);
    }

    free(line);
    return 1;
}

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);
    rl_attempted_completion_function = completion_function;
    while (keep_looping) {
        reap_background_jobs();
        if (!getUserInput()) {
            break;
        }
    }
    return 0;
}
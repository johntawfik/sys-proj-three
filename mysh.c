#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 1024

void interaction(){
    printf("mysh> ");
    fflush(stdout);
}

void executeCommand(const char *cmd, int interactive) {
    char *tempCmd = strdup(cmd);
    tempCmd[strcspn(tempCmd, "\n")] = 0;
    if (strcmp(tempCmd, "exit") == 0) {
        free(tempCmd);
        printf("Exiting my shell.\n");
        exit(0);
    } else {
        printf("%s\n", tempCmd);
        system(tempCmd);
    }
    free(tempCmd);

    if(interactive) interaction();
}

void processInput(int fd, int interactive) {
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    while ((bytesRead = read(fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytesRead] = '\0';
        char *cmd = strtok(buffer, "\n");
        while (cmd != NULL) {
            executeCommand(cmd, interactive);
            cmd = strtok(NULL, "\n");
        }
    }
}

int main(int argc, char *argv[]) {
    int fd = 0; 
    int isInteractive = isatty(STDIN_FILENO);

    if (isInteractive) {
        printf("Welcome to my shell!\n");
    }

    if (argc > 1) {
        FILE *file = fopen(argv[1], "r");
        if (!file) {
            perror("Error opening file");
            return 1;
        }
        fd = fileno(file);
    }

    if (isInteractive) {
        printf("mysh> ");
        fflush(stdout);
    }

    processInput(fd, isInteractive);


    return 0;
}

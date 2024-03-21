#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <glob.h>

#define BUFFER_SIZE 1024
char *dirs[] = {"/usr/local/bin", "/usr/bin", "/bin"};

void interaction()
{
    printf("mysh> ");
    fflush(stdout);
}

int contains_more_than_one_space(const char *str)
{
    const char *firstSpace = strchr(str, ' ');
    if (firstSpace != NULL)
    {
        if (strchr(firstSpace + 1, ' ') != NULL)
        {
            return 1;
        }
    }
    return 0;
}

char **splitStringByPipe(const char *input)
{
    char **result = malloc(3 * sizeof(char *));
    if (result == NULL)
    {
        perror("Failed to allocate memory");
        return NULL;
    }

    const char *delimiterPosition = strchr(input, '|');

    size_t firstLength = delimiterPosition - input;
    result[0] = malloc(firstLength + 1); 
    if (result[0] == NULL)
    {
        perror("Failed to allocate memory");
        free(result);
        return NULL;
    }
    strncpy(result[0], input, firstLength);
    result[0][firstLength] = '\0'; 

    const char *secondStart = delimiterPosition + 1;
    result[1] = strdup(secondStart); 
    if (result[1] == NULL)
    {
        perror("Failed to allocate memory");
        free(result[0]);
        free(result);
        return NULL;
    }

    result[2] = NULL;

    return result;
}

char *get_first(const char *input)
{
    int length = 0;
    while (input[length] != '\0' && !isspace(input[length]))
    {
        length++;
    }

    char *firstWord = (char *)malloc(length + 1);
    if (firstWord == NULL)
    {
        perror("Failed to allocate memory");
        return NULL;
    }

    strncpy(firstWord, input, length);
    firstWord[length] = '\0';

    return firstWord;
}

char *emulate_which(char *file_name)
{
    if (contains_more_than_one_space(file_name))
        return NULL;
    char full_path[1024];

    for (int i = 0; i < 3; i++)
    {
        snprintf(full_path, sizeof(full_path), "%s/%s", dirs[i], file_name);
        char *path = dirs[i];
        if (access(path, X_OK) == 0)
        {
            return strdup(full_path);
        }
    }
    return NULL;
}

void executeCommand(const char *cmd, int interactive)
{
    char *tempCmd = strdup(cmd);
    tempCmd[strcspn(tempCmd, "\n")] = 0;
    char *first = get_first(tempCmd);

    if (strcmp(first, "exit") == 0)
    {
        const char *args = tempCmd + strlen(first);
        while (isspace(*args))
            args++;

        if (*args)
        {
            printf("Provided Arguments: %s\n", args);
        }
        free(tempCmd);
        printf("Exiting my shell.\n");
        exit(0);
    }
    else if (strcmp("cd", first) == 0)
    {
        const char *path = tempCmd + strlen(first);
        while (*path != '\0' && isspace(*path))
            path++;

        if (chdir(path) != 0)
            perror("chdir failed");
    }
    else if (strcmp("which", first) == 0)
    {
        char *path = tempCmd + strlen(first);
        while (*path != '\0' && isspace(*path))
            path++;
        char *res = emulate_which(path);
        if (res)
        {
            printf("%s", res);
        }
    }
    else
    {
        system(tempCmd);
    }
    free(tempCmd);
    free(first);
    if (interactive)
        interaction();
}

void process_wildcard(char *pattern, int interactive)
{
    char *initial_part = get_first(pattern);
    const char *second_part = pattern + strlen(initial_part);
    while (*second_part != '\0' && isspace(*second_part))
        second_part++;
    glob_t glob_result;
    memset(&glob_result, 0, sizeof(glob_result));

    int return_value = glob(second_part, GLOB_TILDE, NULL, &glob_result);
    if (return_value == 0)
    {
        for (size_t i = 0; i < glob_result.gl_pathc; ++i)
        {
            char *full_command = malloc(strlen(initial_part) + 1 + strlen(glob_result.gl_pathv[i]) + 1);
            if (full_command)
            {
                sprintf(full_command, "%s %s", initial_part, glob_result.gl_pathv[i]);
                executeCommand(full_command, interactive);
                free(full_command);
            }
        }
    }
    else if (return_value == GLOB_NOMATCH)
    {
        executeCommand(pattern, interactive);
    }

    globfree(&glob_result);
}

void process_pipe(char *cmd, int interactive)
{
    char **pipes = splitStringByPipe(cmd);
    int fd[2]; //file descriptors for pipes
    int i = 0;

    while (pipes[i] != 0)
        [

            i += 1;
        ]
}

void processInput(int fd, int interactive)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    while ((bytesRead = read(fd, buffer, BUFFER_SIZE - 1)) > 0)
    {
        buffer[bytesRead] = '\0';
        char *cmd = strtok(buffer, "\n");
        if (strchr(cmd, '*') != NULL)
            process_wildcard(cmd, interactive);
        else if (strchr(cmd, '|') != NULL)
            process_pipe(cmd, interactive);
        else
        {
            while (cmd != NULL)
            {
                executeCommand(cmd, interactive);
                cmd = strtok(NULL, "\n");
            }
        }
    }
}

int main(int argc, char *argv[])
{
    int fd = 0;
    int isInteractive = isatty(STDIN_FILENO);

    if (isInteractive)
    {
        printf("Welcome to my shell!\n");
    }

    if (argc > 1)
    {
        FILE *file = fopen(argv[1], "r");
        if (!file)
        {
            perror("Error opening file");
            return 1;
        }
        fd = fileno(file);
    }

    if (isInteractive)
    {
        printf("mysh> ");
        fflush(stdout);
    }

    processInput(fd, isInteractive);

    return 0;
}
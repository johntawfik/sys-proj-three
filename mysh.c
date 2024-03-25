#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <glob.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

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

char **splitStringByVal(char *input, char* delimiter)
{
    char **result = malloc(3 * sizeof(char *));
    if (result == NULL)
    {
        perror("Failed to allocate memory");
        return NULL;
    }

    char *delimiterPosition = strchr(input, *delimiter);

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

void removeAllWhitespaces(char* source) {
    char* i = source;
    char* j = source;
    while (*j != '\0') {
        *i = *j++;
        if (!isspace(*i)) {
            i++;
        }
    }
    *i = '\0';
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

char *executeCommand(const char *cmd, int interactive)
{
    printf("%s\n", cmd);
    char *tempCmd = strdup(cmd);
    tempCmd[strcspn(tempCmd, "\n")] = 0;
    char *first = get_first(tempCmd);
    char *result = NULL;

    if (strcmp(first, "exit") == 0)
    {
        const char *args = tempCmd + strlen(first);
        while (isspace(*args))
            args++;

        if (*args)
        {
            result = strdup(args);
        }
        else
        {
            result = strdup("Exiting my shell.\n");
        }
        printf("%s\n", result);
        free(tempCmd);
        exit(0);
    }
    else if (strcmp("cd", first) == 0)
    {
        const char *path = tempCmd + strlen(first);
        while (*path != '\0' && isspace(*path))
            path++;

        if (chdir(path) != 0)
        {
            // If chdir failed, return an error message
            result = strdup("chdir failed");
        }
    }
    else if (strcmp("which", first) == 0)
    {
        char *path = tempCmd + strlen(first);
        while (*path != '\0' && isspace(*path))
            path++;
        char *res = emulate_which(path);
        if (res)
        {
            result = strdup(res);
        }
    }
    else
    {
        system(tempCmd);
    }

    free(tempCmd);
    free(first);

    return result;
}


void process_wildcard(char *pattern, int interactive)
{
    char *initial_part = get_first(pattern);
    printf("%s\n", pattern);
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
                printf("%s\n command", full_command);
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
    // char **pipes = splitStringByVal(cmd, "|");
    // int fd[2]; 
    // int i = 0;

    // while (pipes[i] != 0)
    //     [

    //         i += 1;
    //     ]
    printf("handling pipes...");
}

void process_stdout(char* file_to_redirect, char* exec_cmd){
    int fd = open(file_to_redirect, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    pid_t pid = fork();

    if(pid < 0) {
        perror("dumbass");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        dup2(fd, STDOUT_FILENO);
        execlp(exec_cmd, exec_cmd, NULL);
    } else{
        close(fd);
        wait(NULL);
    }
}

void process_stdin(char* file_to_redirect, char* exec_cmd){
    int in = open(file_to_redirect, O_RDONLY);
    if(in < 0){
        perror("error opening file");
        return;  
    }

    pid_t pid = fork();
    if(pid == 0) {  
        dup2(in, STDIN_FILENO);
        execlp(exec_cmd, exec_cmd, NULL);
        perror("execlp");  
        exit(EXIT_FAILURE);
    } else if(pid > 0) {  
        wait(NULL);  
        close(in);
    } else {
        perror("fork");  
    }
}



/*
foo < bar baz redirect std input of foo baz to bar
quux *.txt > spam has globbing of *.txt and quux to spam
*/
void process_redirects(char* cmd, char* delim, int interactive){
    char** redirects = splitStringByVal(cmd, delim);
    removeAllWhitespaces(redirects[1]);
    char* file_to_redirect = get_first(redirects[1]);
    char* exec_cmd = get_first(cmd);
    if(strchr(delim, '>') != NULL) process_stdout(file_to_redirect, exec_cmd);
    else process_stdin(file_to_redirect, exec_cmd);
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
            // process_pipe(cmd, interactive);
            printf("piping...");
        else if(strchr(cmd, '<') != NULL || strchr(cmd, '>') != NULL){
            char* delim = (strchr(cmd, '<') != NULL) ? "<" : ">";
            process_redirects(cmd, delim, interactive);
        }
        else
        {
            while (cmd != NULL)
            {
                char* command_res = executeCommand(cmd, interactive);
                if(command_res) printf("%s\n", command_res);
                cmd = strtok(NULL, "\n");
            }
            
        }
        if (interactive)interaction();
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
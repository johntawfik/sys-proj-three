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
#define MAX_ARGS 128
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

char* construct_exec_path(char* exec){
    for(int i = 0; i < 3; i++){
        char* exec_path = malloc(strlen(dirs[i]) + strlen(exec) + 2);
        strcpy(exec_path, dirs[i]);
        strcat(exec_path, "/");
        strcat(exec_path, exec);
        printf("%s\n", exec_path);
        if(access(exec_path, X_OK) == 0) return exec_path;
    }
    return NULL; 
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

char* get_string_before_char(const char* input, char delimiter) {
    const char* delimiter_position = strchr(input, delimiter);
    size_t length;
    if (delimiter_position == NULL) {
        length = strlen(input);
    } else {
        length = delimiter_position - input;
    }
    
    char* result = malloc(length + 1);
    if (result) {
        strncpy(result, input, length);
        result[length] = '\0'; 
        while (length > 0 && isspace((unsigned char)result[length - 1])) {
            length--;
        }
        result[length] = '\0';
    }
    return result;
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
        printf("\n");
    }

    free(tempCmd);
    free(first);

    return result;
}


char* glob_pattern(const char *pattern) {
    glob_t glob_result;
    memset(&glob_result, 0, sizeof(glob_result));
    char *result = NULL;

    if (glob(pattern, GLOB_TILDE | GLOB_NOCHECK, NULL, &glob_result) == 0) {
        // Calculate the total length needed for the result string
        size_t total_length = 0;
        for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
            total_length += strlen(glob_result.gl_pathv[i]) + 1; // +1 for space or null terminator
        }

        result = malloc(total_length);
        if (result) {
            *result = '\0'; // Initialize the string with empty content
            for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
                strcat(result, glob_result.gl_pathv[i]);
                if (i < glob_result.gl_pathc - 1) strcat(result, " ");
            }
        }
    }
    globfree(&glob_result);

    return result;
}

char* process_wildcard(const char *command) {
    char *result = strdup(command); 
    char *wildcard_start;

    while ((wildcard_start = strpbrk(result, "*?[")) != NULL) {
        char *segment_end = strpbrk(wildcard_start, " \t\n");
        size_t segment_length = segment_end ? (size_t)(segment_end - wildcard_start) : strlen(wildcard_start);

        char *pattern = strndup(wildcard_start, segment_length);
        char *globbed = glob_pattern(pattern);
        free(pattern);

        if (globbed) {
            size_t prefix_length = wildcard_start - result;
            size_t new_length = prefix_length + strlen(globbed) + (segment_end ? strlen(segment_end) : 0) + 1;
            char *new_result = malloc(new_length);

            if (new_result) {
                strncpy(new_result, result, prefix_length);
                strcpy(new_result + prefix_length, globbed);
                if (segment_end) {
                    strcat(new_result, segment_end);
                }

                free(result);
                result = new_result;
            }
            free(globbed);
        }
    }

    return result; 
}

// void getCommandByPipe(char *cmd, char **cmd1, char **cmd2) {
//     char **pipes = splitStringByVal(cmd, "|");
//     if (pipes != NULL) {
//         cmd1 = cmd;
//         cmd2 = pipes + 1;
//     }
// }


void process_pipe(char *cmd, int interactive)
{

    char **pipes = splitStringByVal(cmd, "|");
    char **cmd1, *cmd2;
    int pipefd[2];

    

    
    pid_t pid1, pid2;

    getCommandByPipe(cmd, &cmd1, &cmd2);

    

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    char *args[MAX_ARGS];

    char* exec_path = construct_exec_path(args[0]);
    // if(exec_path == NULL){
    //     printf("Improper executable");
    //     close();
    //     exit(EXIT_FAILURE);
    // }
    // fork to execute first command

    
    if((pid1 = fork()) == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        // char *args[MAX_ARGS];
        executeCommand(cmd1, interactive);
        execv(exec_path, args);
        perror("execv cmd 1");
        exit(EXIT_FAILURE);
    }

    // fork to execute second command 
    if((pid1 = fork()) == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        // char *args[MAX_ARGS];
        executeCommand(cmd2, interactive);
        execv(exec_path, args);
        perror("execv cmd 1");
        exit(EXIT_FAILURE);
    }

    
    close(pipefd[0]); // Close both ends in the parent
    close(pipefd[1]);
    waitpid(pid1, NULL, 0); // Wait for both child processes
    waitpid(pid2, NULL, 0);
    
}

void process_stdout(char* file_to_redirect, char* exec_cmd){
    int fd = open(file_to_redirect, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char *args[MAX_ARGS];
    char *token;
    int arg_count = 0;

    char *cmd_copy = strdup(exec_cmd);
    if (!cmd_copy) {
        perror("Failed to duplicate exec_cmd");
        close(fd);
        exit(EXIT_FAILURE);
    }

    token = strtok(cmd_copy, " ");
    while (token != NULL && arg_count < MAX_ARGS - 1) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL; 

    char* exec_path = construct_exec_path(args[0]);
    if(exec_path == NULL){
        printf("Improper executable");
        close(fd);
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        free(cmd_copy);
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        dup2(fd, STDOUT_FILENO); 
        close(fd); 

        execv(exec_path, args); 

        perror("execvp failed");
        exit(EXIT_FAILURE);
    } else {
        close(fd); 
        wait(NULL); 

        free(cmd_copy); 
        free(exec_path);
    }
}


void process_stdin(char* file_to_redirect, char* exec_cmd) {
    int in = open(file_to_redirect, O_RDONLY);
    if (in < 0) {
        perror("error opening file");
        return;
    }

    char *args[MAX_ARGS];
    char *token;
    int arg_count = 0;

    char *cmd_copy = strdup(exec_cmd);
    if (!cmd_copy) {
        perror("Failed to duplicate exec_cmd");
        close(in);
        return;
    }

    token = strtok(cmd_copy, " ");
    while (token != NULL && arg_count < MAX_ARGS - 1) {
        args[arg_count++] = token;
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL; 

    char* exec_path = construct_exec_path(args[0]);
    if(exec_path == NULL){
        printf("Improper executable");
        close(in);
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == 0) {
        dup2(in, STDIN_FILENO); 
        close(in); 

        execvp(exec_path, args); 

        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        wait(NULL); 
        close(in); 
        free(cmd_copy); 
        free(exec_path);
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
    char* exec_cmd = get_string_before_char(cmd, *delim);
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
        // processSentence(cmd);
        if (strchr(cmd, '*') != NULL){
            cmd = process_wildcard(cmd);
            printf("%s\n", cmd);
        } 

        if (strchr(cmd, '|') != NULL)
            process_pipe(cmd, interactive);
            
        else if(strchr(cmd, '<') != NULL || strchr(cmd, '>') != NULL){
            char* delim = (strchr(cmd, '<') != NULL) ? "<" : ">";
            process_redirects(cmd, delim, interactive);
        } else
        {
            while (cmd != NULL)
            {
                char* command_res = executeCommand(cmd, interactive);
                if(command_res) printf("%s \n", command_res);
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
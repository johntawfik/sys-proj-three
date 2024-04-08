#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <glob.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "queue.h"

#define BUFFER_SIZE 1024
#define SUCCESS 1
#define FAIL 0
#define MAX_ARGS 128
char *dirs[] = {"/usr/local/bin", "/usr/bin", "/bin"};
int prev_status = 1; 
void interaction()
{
    printf("mysh> ");
    fflush(stdout);
}

int get_exit_status()
{
    return prev_status;
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

char *construct_exec_path(char *exec)
{
    for (int i = 0; i < 3; i++)
    {
        char *exec_path = malloc(strlen(dirs[i]) + strlen(exec) + 2);
        strcpy(exec_path, dirs[i]);
        strcat(exec_path, "/");
        strcat(exec_path, exec);
        if (access(exec_path, X_OK) == 0)
            return exec_path;
    }
    return NULL;
}

char **splitStringByVal(const char *input, const char *delimiter)
{
    int capacity = 10;
    char **result = malloc(capacity * sizeof(char *));
    if (result == NULL)
    {
        perror("Failed to allocate memory");
        prev_status = FAIL;
        return NULL;
    }

    char *copy = strdup(input); // Make a copy of the input string
    if (copy == NULL)
    {
        perror("Failed to allocate memory");
        free(result);
        prev_status = FAIL;
        return NULL;
    }

    char *token = strtok(copy, delimiter);
    int count = 0;

    // Tokenize the input string and store the tokens
    while (token != NULL)
    {
        if (count >= capacity)
        {
            // If the result array is full, resize it
            capacity *= 2;
            char **temp = realloc(result, capacity * sizeof(char *));
            if (temp == NULL)
            {
                perror("Failed to reallocate memory");
                for (int i = 0; i < count; i++)
                {
                    free(result[i]);
                }
                free(result);
                free(copy);
                return NULL;
            }
            result = temp;
        }

        result[count] = strdup(token);
        if (result[count] == NULL)
        {
            perror("Failed to allocate memory");
            for (int i = 0; i < count; i++)
            {
                free(result[i]);
            }
            free(result);
            free(copy);
            prev_status = FAIL;
            return NULL;
        }

        count++;
        token = strtok(NULL, delimiter);
    }

    free(copy);

    return result;
}

void removeAllWhitespaces(char *source)
{
    char *i = source;
    char *j = source;
    while (*j != '\0')
    {
        *i = *j++;
        if (!isspace(*i))
        {
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

char *executeCommand(const char *cmd)
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

void parse_arguments(const char* cmd, char** args) {
    int i = 0;
    char* token;
    char* cmd_copy = strdup(cmd); 

    
    token = strtok(cmd_copy, " ");
    while (token != NULL && i < MAX_ARGS - 1) { 
        args[i++] = strdup(token); 
        token = strtok(NULL, " "); 
    }
    args[i] = NULL; 

    free(cmd_copy); 
}

void process_pipe(char *cmd) {
    char *cmd1, *cmd2;
    int pipefd[2];
    pid_t pid1, pid2;

    
    char **split_cmds = splitStringByVal(cmd, "|");
    if (!split_cmds || !split_cmds[0] || !split_cmds[1]) {
        printf("Pipe command parsing error.\n");
        return;
    }
    cmd1 = split_cmds[0];
    cmd2 = split_cmds[1];

    // Create a pipe
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE); 
    }

    
    if ((pid1 = fork()) == 0) {
        
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]); 

        
        char* args[MAX_ARGS];
        parse_arguments(cmd1, args); 
        char* exec_path = construct_exec_path(args[0]);
        
        execv(exec_path, args);
        perror("execv cmd1 failed");
        exit(EXIT_FAILURE);
    }

    if ((pid2 = fork()) == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[1]); 

        
        char* args[MAX_ARGS];
        parse_arguments(cmd2, args); 
        char* exec_path = construct_exec_path(args[0]);
        
        execv(exec_path, args);
        perror("execv cmd2 failed");
        exit(EXIT_FAILURE);
    }

    
    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    
    free(split_cmds[0]);
    free(split_cmds[1]);
    free(split_cmds);
}

/*
foo < bar baz redirect std input of foo baz to bar
quux *.txt > spam has globbing of *.txt and quux to spam
*/
void process_redirects(char *cmd) {
    struct Queue *inputQueue = createQueue(10);
    struct Queue *outputQueue = createQueue(10);

    // Tokenize and process the command
    char *token = strtok(cmd, " ");
    while (token != NULL) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            enqueue(inputQueue, token);
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            enqueue(outputQueue, token);
        }
        token = strtok(NULL, " ");
    }

    int inFD = -1, outFD = -1; // File descriptors for input and output files
    char *lastInputFile = NULL;
    char *lastOutputFile = NULL;

    // Handle the last input redirection
    while (!isEmpty(inputQueue)) {
        if (lastInputFile) free(lastInputFile);
        lastInputFile = dequeue(inputQueue);
    }
    if (lastInputFile) {
        inFD = open(lastInputFile, O_RDONLY);
        if (inFD < 0) {
            perror("Failed to open input file");
            exit(EXIT_FAILURE);
        }
    }

    // Handle the last output redirection
    while (!isEmpty(outputQueue)) {
        if (lastOutputFile) free(lastOutputFile);
        lastOutputFile = dequeue(outputQueue);
    }
    if (lastOutputFile) {
        outFD = open(lastOutputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (outFD < 0) {
            perror("Failed to open output file");
            exit(EXIT_FAILURE);
        }
    }

    // Fork and execute the command with redirections
    pid_t pid = fork();
    if (pid == 0) { // Child process
        if (inFD >= 0) {
            dup2(inFD, STDIN_FILENO);
            close(inFD);
        }
        if (outFD >= 0) {
            dup2(outFD, STDOUT_FILENO);
            close(outFD);
        }

        // Prepare for execv
        char *args[MAX_ARGS]; // Adjust size as needed
        int arg_count = 0;
        args[arg_count++] = strtok(cmd, " "); // First token is the command
        while ((token = strtok(NULL, " ")) != NULL && arg_count < MAX_ARGS) {
            args[arg_count++] = token; // Populate args with command arguments
        }
        args[arg_count] = NULL; // Null-terminate the argument list

        char *exec_path = construct_exec_path(args[0]); // Find the full path to the executable
        if (!exec_path) {
            perror("Executable not found");
            exit(EXIT_FAILURE);
        }

        execv(exec_path, args); // Execute the command
        perror("execv failed"); // execv only returns on failure
        exit(EXIT_FAILURE);
    } else if (pid > 0) { // Parent process
        // Close file descriptors in the parent process, if they were opened
        if (inFD >= 0) close(inFD);
        if (outFD >= 0) close(outFD);

        wait(NULL); // Wait for child process to finish
    } else {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    // Cleanup
    if (lastInputFile) free(lastInputFile);
    if (lastOutputFile) free(lastOutputFile);
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
            process_pipe(cmd);
            
        else if(strchr(cmd, '<') != NULL || strchr(cmd, '>') != NULL){
            process_redirects(cmd);
        } else
        {
            while (cmd != NULL)
            {
                char* command_res = executeCommand(cmd);
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

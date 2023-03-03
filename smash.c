#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

// Function prototypes
int lexer(char *line, char ***args, int *num_args, char *delim);
void print_prompt(void);
void print_error(void);
int is_empty(const char *s);
int run_pwd(char **path, size_t *path_size);
int run_exec(char *line, int rd, int pipes);
int run_loop(char *args[], int num_args, int rd, int pipes, char *line);
void run_mult(char *line, char **args, int num_args);
int check_mult_syntax(char **mult_args, int num_mult_args, char **args, int num_args);

int main(int argc, char *argv[])
{

    if (argc > 1)
    {
        print_error();
    }

    // Delcare variables for user input
    char *line = NULL;
    char **args;
    int num_args;
    size_t size_line = 0;
    int rd = 0; // Used for redirection
    int pipes = 0;

    // Main loop for command line interpreter
    while (1)
    {
        // Reset rd and pipes
        rd = 0;
        pipes = 0;

        print_prompt();

        if (getline(&line, &size_line, stdin) == -1)
        {
            print_error();
            continue;
        }

        // Check for empty input
        if (is_empty(line) == 1)
        {
            continue;
        }

        // Check for syntax error on multiple commands, redirection and pipes
        if (strstr(line, ";;") || strstr(line, ">>") || strstr(line, "||"))
        {
            print_error();
            continue;
        }

        // Check for multiple command mode
        if (strstr(line, ";"))
        {
            run_mult(line, args, num_args);
            continue;
        }

        // Check for redirection
        if (strstr(line, ">"))
        {
            rd = 1;
        }

        // Check for pipes
        if (strstr(line, "|"))
        {
            pipes = 1;
        }

        char *line_copy = malloc(sizeof(char) * strlen(line) + sizeof(NULL));
        strcpy(line_copy, line);

        // Tokenize input
        if (lexer(line, &args, &num_args, " \t\n") == -1)
        {
            print_error();
            continue;
        }

        // exit command
        if (strcmp(args[0], "exit") == 0)
        {
            if (num_args > 1)
            {
                print_error();
                continue;
            }
            else
            {
                free(line);
                free(args);
                exit(0);
            }
        }

        // cd command
        if (strcmp(args[0], "cd") == 0)
        {
            if (num_args == 1 || num_args > 2)
            {
                print_error();
                continue;
            }
            if (chdir(args[1]) == -1)
            {
                print_error();
            }
            continue;
        }

        // pwd command
        if (strcmp(args[0], "pwd") == 0)
        {
            if (num_args > 1)
            {
                print_error();
                continue;
            }
            size_t path_size = 16;
            char *path = malloc(path_size);
            if (path == NULL)
            {
                print_error();
                continue;
            }
            if (run_pwd(&path, &path_size) == -1)
            {
                print_error();
                continue;
            }
            printf("%s\n", path);
            free(path);
            continue;
        }

        // Loop command
        if (strcmp(args[0], "loop") == 0)
        {
            if (run_loop(args, num_args, rd, pipes, line_copy) == -1)
            {
                print_error();
            }
            continue;
        }

        // execv() command
        if (run_exec(line_copy, rd, pipes) == -1)
        {
            print_error();
            continue;
        }
    }
}

/**
 * Function: lexer
 * ---------------
 * Takes a line and splits it into args similar to how argc and argv work in main
 *
 * line: The line being split up.  Will be mangled after completion of the function.
 *
 * args: A pointer to an array of strings that will be filled and allocated
 *       with the args from the line.
 *
 * num_args: A pointer to an integer for the number of arguments in args.
 *
 * delim: The delimiter for the string line
 *
 * return: 0 on success, -1 on failure.
 */
int lexer(char *line, char ***args, int *num_args, char *delim)
{
    *num_args = 0;
    // count number of args
    char *l = strdup(line);
    if (l == NULL)
    {
        return -1;
    }
    char *token = strtok(l, delim);
    while (token != NULL)
    {
        (*num_args)++;
        token = strtok(NULL, delim);
    }
    free(l);
    // split line into args
    *args = malloc(sizeof(char **) * *num_args);
    *num_args = 0;
    token = strtok(line, delim);
    while (token != NULL)
    {
        char *token_copy = strdup(token);
        if (token_copy == NULL)
        {
            return -1;
        }
        (*args)[(*num_args)++] = token_copy;
        token = strtok(NULL, delim);
    }
    return 0;
}

/**
 * Function: print_prompt
 * ----------------------
 * Prints the prompt for the shell to the console, and subsequently flushes stdout
 */
void print_prompt(void)
{
    printf("smash> ");
    fflush(stdout);
}

/**
 * Function: print_error
 * ---------------------
 * Prints the error message to the console
 */
void print_error(void)
{
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

/**
 * Function: is_empty
 * ------------------
 * Checks to see if a string is empty (all whitespace).
 * Taken from Stack Overflow at https://stackoverflow.com/a/3981593/6946494
 *
 * s: The string to be checked for emptiness
 *
 * return: 1 if the string is empty,
 *         0 if it has at least one non-whitespace character
 */
int is_empty(const char *s)
{
    while (*s != '\0')
    {
        if (!isspace((unsigned char)*s))
            return 0;
        s++;
    }
    return 1;
}

/**
 * Function: run_pwd
 * -----------------
 * Helper function for pwd built-in command, updating path and path_size as needed
 *
 * path: a pointer to a string where the path will be stored
 *
 * path_size: a pointer to a size_t variable holding the size in bytes of path
 *
 * return: -1 on failure, 0 on success
 */
int run_pwd(char **path, size_t *path_size)
{
    char *ret = getcwd(*path, *path_size);
    while (ret == NULL)
    {
        if (errno == ERANGE)
        {
            *path_size *= 2;
            *path = realloc(*path, *path_size);
            if (*path == NULL)
            {
                return -1;
            }
            ret = getcwd(*path, *path_size);
        }
        else
        {
            return -1;
        }
    }
    *path = ret;
    return 0;
}

/**
 * Function: run_exec
 * -----------------
 * Helper function for execv() system call.
 * Forks a process, letting the child process run the command with execv()
 * and then waits for it to finish before proceeding with the parent.
 *
 * line: the entire command to be executed, sometimes including redirection and pipes
 *
 * rd: 1 if the command uses redirection, 0 if not
 *
 * pipes: 1 if the command uses pipes, 0 if not
 *
 * return: -1 on failure, 0 on success.
 */
int run_exec(char *line, int rd, int pipes)
{
    int rc = fork();
    if (rc < 0)
    {
        return -1;
    }
    else if (rc == 0)
    {
        char **args;
        int num_args;

        if (pipes)
        {
            int pipefd[2];
            char **pipe_cmds;
            int num_pipe_cmds;

            // Split command into tokens containing commands & arguments for each pipe
            if (lexer(line, &pipe_cmds, &num_pipe_cmds, "|") == -1)
            {
                print_error();
                exit(1);
            }

            // Loop through until pipe commands are all handled
            for (int i = 0; i < num_pipe_cmds - 2; i++)
            {
                // Tokenize each end of the pipe
                char **pipe_args1;
                int num_pipe_args1;
                char **pipe_args2;
                int num_pipe_args2;
                if (lexer(pipe_cmds[i], &pipe_args1, &num_pipe_args1, " \t\n") == -1)
                {
                    print_error();
                    exit(1);
                }
                if (lexer(pipe_cmds[i + 1], &pipe_args2, &num_pipe_args2, " \t\n") == -1)
                {
                    print_error();
                    exit(1);
                }

                // Create each command array for each end of the pipe
                char **pipe_args1_tmp = malloc(sizeof(char **) * (num_pipe_args1 + 1));
                if (pipe_args1_tmp == NULL)
                {
                    return -1;
                }
                for (int i = 0; i < num_pipe_args1; i++)
                {
                    pipe_args1_tmp[i] = malloc(sizeof(char) * strlen(pipe_args1[i]) + sizeof(NULL));
                    strcpy(pipe_args1_tmp[i], pipe_args1[i]);
                }
                pipe_args1_tmp[num_pipe_args1] = NULL;

                char **pipe_args2_tmp = malloc(sizeof(char **) * (num_pipe_args2 + 1));
                if (pipe_args2_tmp == NULL)
                {
                    return -1;
                }
                for (int i = 0; i < num_pipe_args2; i++)
                {
                    pipe_args2_tmp[i] = malloc(sizeof(char) * strlen(pipe_args2[i]) + sizeof(NULL));
                    strcpy(pipe_args2_tmp[i], pipe_args2[i]);
                }
                pipe_args2_tmp[num_pipe_args2] = NULL;

                if (pipe(pipefd) == -1)
                {
                    return -1;
                }

                int pid = fork();

                // Fork on pipe
                if (pid < 0)
                {
                    return -1;
                }
                else if (pid == 0)
                {
                    dup2(pipefd[1], 1);
                    close(pipefd[0]);
                    execv(pipe_args1_tmp[0], pipe_args1_tmp);
                    print_error();
                    exit(1);
                }
                else
                {
                    dup2(pipefd[0], 0);
                    close(pipefd[1]);
                    if (waitpid(pid, NULL, 0) == -1)
                    {
                        return -1;
                    }
                    sleep(5); // REMOVE ME
                    execv(pipe_args2_tmp[0], pipe_args2_tmp);
                    print_error();
                    return -1;
                }
            }

            // Tokenize the last ends of the pipe for redirection
            char **last_pipe1_args;
            int num_last1_args;
            if (lexer(pipe_cmds[num_pipe_cmds - 2], &last_pipe1_args, &num_last1_args, " \t\n") == -1)
            {
                print_error();
                exit(1);
            }
            char **last_pipe1_args_tmp = malloc(sizeof(char **) * (num_last1_args + 1));
            if (last_pipe1_args_tmp == NULL)
            {
                return -1;
            }
            for (int i = 0; i < num_last1_args; i++)
            {
                last_pipe1_args_tmp[i] = malloc(sizeof(char) * strlen(last_pipe1_args[i]) + sizeof(NULL));
                strcpy(last_pipe1_args_tmp[i], last_pipe1_args[i]);
            }
            last_pipe1_args_tmp[num_last1_args] = NULL;

            char **last_pipe2_args;
            int num_last2_args;
            if (lexer(pipe_cmds[num_pipe_cmds - 1], &last_pipe2_args, &num_last2_args, " \t\n") == -1)
            {
                print_error();
                exit(1);
            }
            char **last_pipe2_args_tmp = malloc(sizeof(char **) * (num_last2_args + 1));
            if (last_pipe2_args_tmp == NULL)
            {
                return -1;
            }
            for (int i = 0; i < num_last2_args; i++)
            {
                last_pipe2_args_tmp[i] = malloc(sizeof(char) * strlen(last_pipe2_args[i]) + sizeof(NULL));
                strcpy(last_pipe2_args_tmp[i], last_pipe2_args[i]);
            }
            last_pipe2_args_tmp[num_last2_args] = NULL;

            // Handle redirection
            if (rd)
            {
                // Check for empty command
                if (strcmp(last_pipe2_args[0], ">") == 0)
                {
                    print_error();
                    exit(1);
                }

                int newfd; // New file descriptor

                // Check for more than one ">"
                int n = 0;
                for (int i = 0; i < num_last2_args; i++)
                {
                    if (strcmp(last_pipe2_args[i], ">") == 0)
                    {
                        n++;
                    }
                }
                if (n > 1)
                {
                    print_error();
                    exit(1);
                }

                // Check for ">" being second-to-last argument
                if (strcmp(last_pipe2_args[num_last2_args - 2], ">") != 0)
                {
                    print_error();
                    exit(1);
                }

                // Redirect output to given file
                if ((newfd = open(last_pipe2_args[num_last2_args - 1], O_CREAT | O_TRUNC | O_WRONLY, 0644)) < 0)
                {
                    print_error();
                    exit(1);
                }
                dup2(newfd, 1);
                close(newfd);

                // Change command array to eliminate redirection operator & file
                last_pipe2_args_tmp[num_last2_args - 2] = NULL;

                // Fork here ???
                if (pipe(pipefd) == -1)
                {
                    return -1;
                }

                int pid = fork();

                // Fork on pipe
                if (pid < 0)
                {
                    return -1;
                }
                else if (pid == 0)
                {
                    dup2(pipefd[1], 1);
                    close(pipefd[0]);
                    execv(last_pipe1_args_tmp[0], last_pipe1_args_tmp); // args 1
                    print_error();
                    exit(1);
                }
                else
                {
                    dup2(pipefd[0], 0);
                    close(pipefd[1]);
                    if (waitpid(pid, NULL, 0) == -1)
                    {
                        return -1;
                    }
                    execv(last_pipe2_args_tmp[0], last_pipe2_args_tmp); // args 2
                    print_error();
                    return -1;
                }


                // execv(last_pipe2_args[0], last_pipe2_args);
                // print_error();
                // exit(1);
            }
            else
            {
                if (pipe(pipefd) == -1)
                {
                    return -1;
                }

                int pid = fork();

                // Fork on pipe
                if (pid < 0)
                {
                    return -1;
                }
                else if (pid == 0)
                {
                    dup2(pipefd[1], 1);
                    close(pipefd[0]);
                    execv(last_pipe1_args_tmp[0], last_pipe1_args_tmp); // args 1
                    print_error();
                    exit(1);
                }
                else
                {
                    dup2(pipefd[0], 0);
                    close(pipefd[1]);
                    if (waitpid(pid, NULL, 0) == -1)
                    {
                        return -1;
                    }
                    execv(last_pipe2_args_tmp[0], last_pipe2_args_tmp); // args 2
                    print_error();
                    return -1;
                }
            }
        }
        else
        {
            if (lexer(line, &args, &num_args, " \t\n") == -1)
            {
                return -1;
            }
            char **cmd_args = malloc(sizeof(char **) * (num_args + 1));
            if (cmd_args == NULL)
            {
                return -1;
            }
            for (int i = 0; i < num_args; i++)
            {
                cmd_args[i] = malloc(sizeof(char) * strlen(args[i]) + sizeof(NULL));
                strcpy(cmd_args[i], args[i]);
            }
            cmd_args[num_args] = NULL;

            // Handle redirection
            if (rd)
            {
                // Check for empty command
                if (strcmp(cmd_args[0], ">") == 0)
                {
                    print_error();
                    exit(1);
                }

                int newfd; // New file descriptor

                // Check for more than one ">"
                int n = 0;
                for (int i = 0; i < num_args; i++)
                {
                    if (strcmp(cmd_args[i], ">") == 0)
                    {
                        n++;
                    }
                }
                if (n > 1)
                {
                    print_error();
                    exit(1);
                }

                // Check for ">" being second-to-last argument
                if (strcmp(cmd_args[num_args - 2], ">") != 0)
                {
                    print_error();
                    exit(1);
                }

                // Redirect output to given file
                if ((newfd = open(cmd_args[num_args - 1], O_CREAT | O_TRUNC | O_WRONLY, 0644)) < 0)
                {
                    print_error();
                    exit(1);
                }
                dup2(newfd, 1);
                close(newfd);

                // Change command array to eliminate redirection operator & file
                cmd_args[num_args - 2] = NULL;

                execv(cmd_args[0], cmd_args);
                print_error();
                exit(1);
            }
            else
            {
                execv(cmd_args[0], cmd_args);
                print_error();
                exit(1);
            }
        }
    }
    else
    {
        if (waitpid(rc, NULL, 0) == -1)
        {
            return -1;
        }
        return 0;
    }
}

/**
 * Function: run_loop
 * ------------------
 * Helper function that runs the loop command
 *
 * args: An array of strings containing the input for the command
 *
 * num_args: An integer for the number of arguments,
 *           including "loop" and its corresponding loop variable
 *
 * rd: 1 if the command uses redirection, 0 if not
 *
 * pipes: 1 if the command uses pipes, 0 if not
 *
 * line: the input for the entire command
 *
 * return: -1 on failure, 0 on success
 */
int run_loop(char *args[], int num_args, int rd, int pipes, char *line)
{
    if (num_args < 2)
    {
        return -1;
    }
    int loop_var = atoi(args[1]);
    if (loop_var < 1)
    {
        return -1;
    }
    for (int i = 0; i < loop_var; i++)
    {
        // exit command
        if (strcmp(args[2], "exit") == 0)
        {
            if (num_args > 3)
            {
                return -1;
            }
            else
            {
                exit(0);
            }
        }

        // cd command
        if (strcmp(args[2], "cd") == 0)
        {
            if (num_args == 3 || num_args > 4)
            {
                return -1;
            }
            if (chdir(args[3]) == -1)
            {
                return -1;
            }
            continue;
        }

        // pwd command
        if (strcmp(args[2], "pwd") == 0)
        {
            if (num_args > 1)
            {
                return -1;
            }
            size_t path_size = 16;
            char *path = malloc(path_size);
            if (path == NULL)
            {
                return -1;
            }
            if (run_pwd(&path, &path_size) == -1)
            {
                return -1;
            }
            printf("%s\n", path);
            free(path);
            continue;
        }

        // Loop command
        if (strcmp(args[2], "loop") == 0)
        {
            run_loop(args + 2, num_args - 2, rd, pipes, line);
        }

        // execv() command
        if (run_exec(line + 7, rd, pipes) == -1)
        {
            return -1;
        }
    }
    return 0;
}

/**
 * Function: run_mult
 * ------------------
 * Helper function to run in multiple-command mode
 *
 * line: The line containing user input
 *
 * args: An array of strings that will be
 *       filled in each iteration of the commands
 *
 * num_args: An integer of the number of arguments in args
 */
void run_mult(char *line, char **args, int num_args)
{
    char **mult_args;
    int num_mult_args;
    if (lexer(line, &mult_args, &num_mult_args, ";") == -1)
    {
        print_error();
        return;
    }

    if (check_mult_syntax(mult_args, num_mult_args, args, num_args) == -1)
    {
        print_error();
        return;
    }

    for (int i = 0; i < num_mult_args; i++)
    {
        int pipes = 0;
        int rd = 0; // Redirection for each command

        // Check for empty command
        if (is_empty(mult_args[i]) == 1)
        {
            continue;
        }

        // Check for redirection
        if (strstr(mult_args[i], ">"))
        {
            rd = 1;
        }

        // Check for pipes
        if (strstr(line, "|"))
        {
            pipes = 1;
        }

        char *mult_args_copy = malloc(sizeof(char) * strlen(line) + sizeof(NULL));
        strcpy(mult_args_copy, mult_args[i]);

        // Tokenize input
        if (lexer(mult_args[i], &args, &num_args, " \t\n") == -1)
        {
            print_error();
            continue;
        }

        // exit command
        if (strcmp(args[0], "exit") == 0)
        {
            if (num_args > 1)
            {
                print_error();
                continue;
            }
            else
            {
                free(line);
                free(args);
                exit(0);
            }
        }

        // cd command
        if (strcmp(args[0], "cd") == 0)
        {
            if (num_args == 1 || num_args > 2)
            {
                print_error();
                continue;
            }
            if (chdir(args[1]) == -1)
            {
                print_error();
            }
            continue;
        }

        // pwd command
        if (strcmp(args[0], "pwd") == 0)
        {
            if (num_args > 1)
            {
                print_error();
                continue;
            }
            size_t path_size = 16;
            char *path = malloc(path_size);
            if (path == NULL)
            {
                print_error();
                continue;
            }
            if (run_pwd(&path, &path_size) == -1)
            {
                print_error();
                continue;
            }
            printf("%s\n", path);
            free(path);
            continue;
        }

        // Loop command
        if (strcmp(args[0], "loop") == 0)
        {
            if (run_loop(args, num_args, rd, pipes, mult_args_copy) == -1)
            {
                print_error();
            }
            continue;
        }

        // execv() command
        if (run_exec(mult_args_copy, rd, pipes) == -1)
        {
            print_error();
            continue;
        }
    }
}

/**
 * Function: check_mult_syntax
 * ---------------------------
 * Helper function to determine if there are any syntax errors
 * on the input line that has multiple commands.
 *
 * mult_args: the array of strings where each element is an entire command w/ arguments
 *
 * num_mult_args: the number of elements in mult_args
 *
 * args: the array of strings containing the command-line input (for each command)
 *
 * num_args: the number of elements in args
 *
 * return: -1 if there are any syntax errors, 0 otherwise
 */
int check_mult_syntax(char **mult_args, int num_mult_args, char **args, int num_args)
{
    /**
     * To be honest I don't even know if I need this part,
     * but am doing it now to be safe anyway.  I don't remember
     * if dynamically-allocated arrays are pass-by-reference or not.
     */
    char **args_tmp = malloc(sizeof(char **) * num_args);
    for (int i = 0; i < num_args; i++)
    {
        args_tmp[i] = malloc(sizeof(char) * strlen(args[i]) + sizeof(NULL));
        strcpy(args_tmp[i], args[i]);
    }
    int num_args_tmp = num_args;

    char **mult_args_tmp = malloc(sizeof(char **) * num_mult_args);
    for (int i = 0; i < num_mult_args; i++)
    {
        mult_args_tmp[i] = malloc(sizeof(char) * strlen(mult_args[i]) + sizeof(NULL));
        strcpy(mult_args_tmp[i], mult_args[i]);
    }

    for (int i = 0; i < num_mult_args; i++)
    {
        int rd = 0; // Redirection for each command
        int pipes = 0;

        // Check for empty command
        if (is_empty(mult_args_tmp[i]) == 1)
        {
            continue;
        }

        // Check for redirection
        if (strstr(mult_args_tmp[i], ">"))
        {
            rd = 1;
        }

        // Check for pipes
        if (strstr(mult_args_tmp[i], "|"))
        {
            pipes = 1;
        }

        // Tokenize input
        if (lexer(mult_args_tmp[i], &args_tmp, &num_args_tmp, " \t\n") == -1)
        {
            return -1;
        }

        // exit command
        if (strcmp(args_tmp[0], "exit") == 0)
        {
            if (num_args_tmp > 1)
            {
                return -1;
            }
            continue;
        }

        // cd command
        if (strcmp(args_tmp[0], "cd") == 0)
        {
            if (num_args_tmp == 1 || num_args_tmp > 2)
            {
                return -1;
            }
            continue;
        }

        // pwd command
        if (strcmp(args_tmp[0], "pwd") == 0)
        {
            if (num_args_tmp > 1)
            {
                return -1;
            }
            continue;
        }

        // Loop command
        if (strcmp(args_tmp[0], "loop") == 0)
        {
            if (num_args_tmp < 2)
            {
                return -1;
            }
            int loop_var = atoi(args_tmp[1]);
            if (loop_var < 1)
            {
                return -1;
            }
            continue;
        }

        // execv() command
        if (rd)
        {
            // Check for empty command
            if (strcmp(args_tmp[0], ">") == 0)
            {
                return -1;
            }

            // Check for more than one ">"
            int n = 0;
            for (int i = 0; i < num_args_tmp; i++)
            {
                if (strcmp(args_tmp[i], ">") == 0)
                {
                    n++;
                }
            }
            if (n > 1)
            {
                return -1;
            }

            // Check for ">" being second-to-last argument
            if (strcmp(args_tmp[num_args_tmp - 2], ">") != 0)
            {
                return -1;
            }
        }
        if (pipes)
        {
            // Check for empty command
            if (strcmp(args_tmp[0], "|") == 0)
            {
                return -1;
            }

            // Check for receiving end of pipe missing
            if (strcmp(args_tmp[num_args - 1], "|") == 0)
            {
                return -1;
            }

            // Check for empty pipe
            for (int i = 1; i < num_args_tmp; i++)
            {
                if (strcmp(args_tmp[i - 1], "|") == 0 && strcmp(args_tmp[i], "|") == 0)
                {
                    return -1;
                }
            }
        }
    }
    return 0;
}
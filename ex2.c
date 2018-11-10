/*NAME: Ofir Cohen
* ID: 312255847
* DATE: 10/4/2018
* This software is a terminal (shell/bash) simulator with linux command
*/
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <pwd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_STRING_SIZE 512
#define END "done\n"
#define CHANGE_DIR "cd"
#define FILE_IN "<"
#define FILE_OUT ">"
#define FILE_OUT_APPEND ">>"
#define FILE_ERR "2>"
#define PIPE "|"

void print_prompt();
char** read_tokens(int, char[]);
int sum_words(char[]);
void child_process(char**, int);
void parent_process(char**);
void check_space(char[]);
void sig_handler(int);
int check_sign(char**, int, char*);
char** break_argv_left(char**, int, int);
char** break_argv_right(char**, int, int);
void pipe_main(char**, int, int);
void redirection(char**, int, int);
void print_done();
void free_array(char**, int);

static int flag_background = 0;
static int num_of_cmd = 0;
static int cmd_length = 0;

int main()
{
    char usrInput[MAX_STRING_SIZE];

    do{
        flag_background = 0;
        signal(SIGINT, sig_handler);
        signal(SIGCHLD, SIG_DFL);
        print_prompt();
        fgets(usrInput, MAX_STRING_SIZE - 1, stdin);

        if(strcmp(usrInput, END) != 0)
        {
            char copy_input[MAX_STRING_SIZE];
            strcpy(copy_input, usrInput);

            int count_words = sum_words(copy_input);
            if(count_words != 0)
            {
                char** argv = read_tokens(count_words, usrInput);

                if(strcmp(argv[0], CHANGE_DIR) == 0)
                {
                    int ret;
                    ret = chdir(argv[1]);
                    free_array(argv, count_words + 1);
                    continue;
                }

                pid_t child;
                child = fork();
                if(child < 0)                           //fork() failed
                {
                    printf("Error\n");
                    exit(1);
                }
                else if(child == 0)                     //child process
                {
                    child_process(argv, count_words);
                }
                else                                    //parent process
                {
                    parent_process(argv);
                    free_array(argv, count_words + 1);
                }
            }
            else
                check_space(usrInput);
        }
    }
    while(strcmp(usrInput, END) != 0);
    print_done();

    return 0;
}


void print_prompt()
{
    char cwd[MAX_STRING_SIZE];
    //to get the real id we need to write getpwuid(geteuid());
    printf("%s@%s>", getpwuid(0)->pw_name,getcwd(cwd, 512));
}


char** read_tokens(int count_words, char input[])
{
    char** argv = (char**)malloc((count_words + 1) * sizeof(char*));
    if(argv == NULL)
    {
        printf("Error: Could not allocate memory\n");
        exit(1);
    }

    char* ptr_input;
    ptr_input = strtok(input, " \n");
    int i = 0;
    while(i < count_words)
    {
        argv[i] = (char*)malloc((strlen(ptr_input) + 1) * sizeof(char));
        if(argv[i] == NULL)
        {
            printf("Error: Could not allocate memory\n");
        }
        strcpy(argv[i], ptr_input);
        i++;
        ptr_input = strtok(NULL, " \n");
    }
    argv[i] = NULL;
    return argv;
}


//counts how many words there are in input
int sum_words(char input[])
{
    int counter = 0;
    char *ptr_input;
    ptr_input = strtok(input, " \n");
    while(ptr_input != NULL)
    {
        if(strcmp(ptr_input, "&") == 0 && strtok(NULL, " \n") == NULL)
        {
            counter--;
            flag_background = 1;
        }
        counter++;
        ptr_input = strtok(NULL, " \n");
    }
    return counter;
}


//child process
void child_process(char** argv, int count_words)
{
    signal(SIGINT, sig_handler);

    int is_pipe = check_sign(argv, count_words, PIPE);
    int is_file_out = check_sign(argv, count_words, FILE_OUT);
    int is_file_out_append = check_sign(argv, count_words, FILE_OUT_APPEND);
    int is_file_err = check_sign(argv, count_words, FILE_ERR);
    int is_file_in = check_sign(argv, count_words, FILE_IN);
    int redir = 1;

    if(is_file_out == -1 && is_file_out_append == -1 && is_file_in == -1 && is_file_err == -1)
        redir = 0;

    if(is_file_out != -1)
        redirection(argv, count_words, is_file_out);
    else if(is_file_out_append != -1)
        redirection(argv, count_words, is_file_out_append);
    else if(is_file_err != -1)
        redirection(argv, count_words, is_file_err);
    else if(is_file_in != -1)
        redirection(argv, count_words, is_file_in);

    if(is_pipe != -1 && redir == 0)
        pipe_main(argv, count_words, is_pipe);

    if(redir == 0 && is_pipe == -1)
    {
        int check_exec;
        check_exec = execvp(argv[0], argv);			//execute the command
        if (check_exec == -1 &&)
            printf("%s: command not found\n", argv[0]);
    }
    exit(0);
}


//parent process
void parent_process(char** argv)
{
    if(flag_background == 0)
        wait(NULL);

    else
    {
        cmd_length += strlen(argv[0]);
        num_of_cmd++;
    }
}

void check_space(char input[])
{
    if(strcmp(input, "\n") != 0)
    {
        printf(": command not found\n");
        num_of_cmd++;
    }
}


void sig_handler(int signo)
{
    if(signo == SIGINT)
        signal(SIGCHLD, sig_handler);

    if(signo == SIGCHLD)
    {
        pid_t child;
        int status;
        child = wait(&status);
    }
}

int check_sign(char** argv, int count_words, char* sign)
{
    int i;
    for(i = 0; i < count_words - 1; i++)
    {
        if(strcmp(argv[i], sign) == 0 && i != 0)
            return i;
    }

    return -1;
}

char** break_argv_left(char** argv, int count_word, int pipe_index)
{
    char** left_argv = (char**)malloc((count_word + 1) * sizeof(char*));
    if(left_argv == NULL)
    {
        printf("Error: could not allocate memory\n");
        exit(1);
    }

    int i = 0;
    while(i < pipe_index)
    {
        left_argv[i] = (char*)malloc((strlen(argv[i]) + 1) * sizeof(char));
        if(left_argv[i] == NULL)
        {
            printf("Error: could not allocate memory\n");
            exit(1);
        }
        strcpy(left_argv[i], argv[i]);
        i++;
    }
    left_argv[i] = NULL;
    return left_argv;
}

char** break_argv_right(char** argv, int count_word, int pipe_index)
{
    char** right_argv = (char**)malloc((count_word + 1) * sizeof(char*));
    if(right_argv == NULL)
    {
        printf("Error: could not allocate memory\n");
        exit(1);
    }

    int i = 0;
    int j = pipe_index + 1;
    while(i < count_word - pipe_index - 1)
    {
        right_argv[i] = (char*)malloc((strlen(argv[j]) + 1) * sizeof(char));
        if(right_argv[i] == NULL)
        {
            printf("Error: could not allocate memory\n");
            exit(1);
        }
        strcpy(right_argv[i], argv[j]);
        i++;
        j++;
    }
    right_argv[i] = NULL;
    return right_argv;
}

void pipe_main(char** argv, int count_words, int pipe_index)
{
    char** left_argv = break_argv_left(argv, count_words, pipe_index);
    char** right_argv = break_argv_right(argv, count_words, pipe_index);

    int pipe_fd[2];
    if((pipe(pipe_fd)) == -1)
    {
        perror("Error: failed pipe");
        exit(1);
    }

    pid_t child;
    child = fork();
    if(child < 0)
    {
        printf("Error\n");
        exit(1);
    }
    else if(child == 0)
    {
        close(pipe_fd[0]);
        int val = dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);
        if(val == -1)
        {
            perror("Error: failed dup2");
            exit(1);
        }
        int check_exec;
        check_exec = execvp(left_argv[0], left_argv);			//execute the command
        if (check_exec == -1)
            printf("%s: command not found\n", argv[0]);

        exit(0);
    }
    else
    {
        wait(NULL);
        close(pipe_fd[1]);
        int val = dup2(pipe_fd[0], STDIN_FILENO);
        close(pipe_fd[0]);
        if(val == -1)
        {
            perror("Error: failed dup2");
            exit(1);
        }
        int check_exec;
        check_exec = execvp(right_argv[0], right_argv);			//execute the command
        if (check_exec == -1)
            printf("%s: command not found\n", argv[0]);
    }

    free_array(left_argv, pipe_index + 1);
    free_array(right_argv, count_words - pipe_index - 1);
}

void redirection(char** argv,int countWords,int file_redir_index)
{
    char ** right_argv = break_argv_right(argv, countWords, file_redir_index);
    char ** left_argv = break_argv_left(argv, countWords, file_redir_index);

    int fd = -1;
    if ( strcmp(argv[file_redir_index],FILE_OUT_APPEND) == 0 )
    {
        fd = open(right_argv[0], O_CREAT | O_APPEND | O_WRONLY, S_IRWXU | S_IRWXG |S_IRWXO);
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    else if ( strcmp(argv[file_redir_index],FILE_OUT) == 0 )
    {
        fd = open(right_argv[0], O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU | S_IRWXG |S_IRWXO);
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    else if ( strcmp(argv[file_redir_index],FILE_ERR) == 0 )
    {
        fd = open(right_argv[0], O_WRONLY|O_CREAT|O_APPEND, S_IRWXU);
        dup2(fd, STDERR_FILENO);
        close(fd);
    }

    else if(strcmp(argv[file_redir_index], FILE_IN) == 0)
    {
        fd = open(right_argv[0], O_RDONLY, 0);
        dup2(fd, STDIN_FILENO);
        close ( fd ) ;

    }

    int is_pipe = check_sign(left_argv,file_redir_index + 1,PIPE);
    if( is_pipe != -1)
        pipe_main(left_argv,file_redir_index ,is_pipe);

    else
    {
        int checkExec;
        checkExec = execvp(left_argv[0],left_argv);    // executing the command
        if (checkExec == -1)
            printf("%s: command not found\n", left_argv[0]);
    }

    free_array(left_argv, file_redir_index + 1);
    free_array(right_argv, countWords - file_redir_index - 1);
    wait(NULL);
}


void print_done()
{
    printf("Num of cmd: %d\nCmd length: %d\nBye !\n", num_of_cmd, cmd_length);
}


void free_array(char** arr, int size)
{
    int i;
    for(i = 0; i < size; i++)
        free(arr[i]);

    free(arr);
}

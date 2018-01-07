#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "sfish.h"
#include "debug.h"

void eval(char *input);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
pid_t Fork(void);
void sigchld_handler(int s);
void sigint_handler(int s);
int left_angle(char *input);
int right_angle(char *input);
volatile sig_atomic_t sig;


int main(int argc, char *argv[], char* envp[]) {
    char* input;
    char cwd[PATH_MAX];
    char pwd[PATH_MAX];
    char *homedir;
    bool exited = false;

    if(!isatty(STDIN_FILENO)) {
        // If your shell is reading from a piped file
        // Don't have readline write anything to that file.
        // Such as the prompt or "user input"
        if((rl_outstream = fopen("/dev/null", "w")) == NULL){
            perror("Failed trying to open DEVNULL");
            exit(EXIT_FAILURE);
        }
    }
    homedir = getenv("HOME");
    do {
        if(strncmp(getcwd(cwd, sizeof(cwd)), homedir, strlen(homedir)) == 0){
            pwd[0] = '~';
            input = readline(strcat(strcat(pwd, cwd+strlen(homedir)), " :: jihchen >>"));
            //clears pwd and cwd for next loop
            memset(&pwd[0], 0, sizeof(pwd));
            memset(&cwd[0], 0, sizeof(cwd));
        }
        else{
            input = readline(strcat(cwd, " :: jihchen >>"));
            //clears cwd for next loop
            memset(&cwd[0], 0, sizeof(cwd));
        }

        //extra credit hint
        /*write(1, "\e[s", strlen("\e[s"));
        write(1, "\e[20;10H", strlen("\e[20;10H"));
        write(1, "SomeText", strlen("SomeText"));
        write(1, "\e[u", strlen("\e[u"));*/

        // If EOF is read (aka ^D) readline returns NULL
        if(input == NULL) {
            continue;
        }


        // Currently nothing is implemented
        eval(input);
        //printf(EXEC_NOT_FOUND, input);

        // You should change exit to a "builtin" for your hw.
        //exited = strncmp(input, "exit", strlen("exit")) == 0;

        // Readline mallocs the space for input. You must free it.
        rl_free(input);

    } while(!exited);

    debug("%s", "user entered 'exit'");
    exit(0);
    return EXIT_SUCCESS;
}

void eval(char *input){
    char *argv[strlen(input)];
    int la,ra;
    //int bg;
    pid_t pid;

    la = left_angle(input);
    ra = right_angle(input);
    /*bg = */parseline(input, argv);
    if (argv[0] == NULL){
        return;
    }
    if (!builtin_command(argv)){
        sigset_t mask, prev;

        Signal(SIGCHLD, sigchld_handler);
        Sigemptyset(&mask);
        Sigaddset(&mask, SIGCHLD);
            //Block SIGCHLD
            Sigprocmask(SIG_BLOCK, &mask, &prev);
            //Child runs user-job
            if ((pid = Fork()) == 0){
            //execute process here
                if(la > 0){
                    int i = 0;
                    int f = 0;
                    char *path;
                    while(argv[i] != NULL){
                        if (strcmp(argv[i], "<") == 0 && f == 0){
                            argv[i] = NULL;
                            f = 1;
                            path = argv[i + 1];
                        }
                        else if(f == 1){
                            argv[i] = NULL;
                        }
                        i++;
                    }
                    int infile = open(path, O_RDONLY);
                    dup2(infile, STDIN_FILENO);
                    close(infile);
                    fflush(stdin);
                }
                if(ra > 0){
                    int i = 0;
                    int f = 0;
                    char *path;
                    while(argv[i] != NULL){
                        if (strcmp(argv[i], ">") == 0 && f == 0){
                            argv[i] = NULL;
                            f = 1;
                            path = argv[i + 1];
                        }
                        else if(f == 1){
                            argv[i] = NULL;
                        }
                        i++;
                    }
                    int outfile = open(path, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
                    dup2(outfile, STDOUT_FILENO);
                    close(outfile);
                    fflush(stdout);
                }
                if(execvp(argv[0],argv) < 0){
                    printf(EXEC_NOT_FOUND, argv[0]);
            }
            exit(0);
       // printf(EXEC_NOT_FOUND, input);
        }
        sig = 0;
        while (!sig){
            sigsuspend(&prev);
        }
        //Unblock SIGCHLD
        Sigprocmask(SIG_SETMASK, &prev, NULL);
    }
    return;

}
//checks if < operator in input, returns index of < if so
int left_angle(char * input){
    char *index = strchr(input, '<');
    if (index != NULL && index == strrchr(input, '<')){
        return (int) (index - input);
    }
    else if (index != NULL){
        printf(SYNTAX_ERROR, "Invalid redirection");
        return -1;
    }
    return 0;
}
//checks if > operator in input, returns index of > if so
int right_angle(char * input){
    char *index = strchr(input, '>');
    if (index != NULL && index == strrchr(input, '>')){
        return (int) (index - input);
    }
    else if (index != NULL){
        printf(SYNTAX_ERROR, "Invalid redirection");
        return -1;
    }
    return 0;
}

int builtin_command(char **argv){
    //exit built-in
    if(!strcmp(argv[0], "exit")){
        exit(0);
    }
    //help built-in
    if(!strcmp(argv[0], "help")){
        printf("help - prints a list of all builtins \n"
                "exit - exits the shell \n"
                "cd - changes the current working directory \n"
                "pwd - prints absolute path of the current working directory \n");
        return 1;
    }
    //cd built-in
    if(!strcmp(argv[0], "cd")){
        //no arguments, go to HOME directory
        if(argv[1] == NULL){
            chdir(getenv("HOME"));
        }
        else if(!strcmp(argv[1],"-")){
            chdir(getenv("OLDPWD"));
        }
        else if(!strcmp(argv[1],".")){
            chdir(".");
        }
        else if(!strcmp(argv[1],"..")){
            chdir("..");
        }
        else{
            chdir(argv[1]);
        }
        return 1;
    }
    //pwd built-in
    if(!strcmp(argv[0], "pwd")){
        printf("%s\n", getcwd(NULL,0) );
        return 1;
    }
    //return 0 if not a builtin
    return 0;
}

int parseline(char *buf, char **argv){
    int argc;
    char * tokens;

    argc = 0;
    tokens = strtok (buf, " ");
    while (tokens != NULL){
            argv[argc++] = tokens;
        tokens = strtok (NULL, " ");
    }
    argv[argc] = NULL;

    if (argc == 0){
        return 1;
    }
    return 0;
}

void sigchld_handler(int s){
   while ((sig = waitpid(-1, NULL, 0)) > 0) {}
}

void sigint_handler(int s){
}

//wrappers taken from textbook
pid_t Fork(void){
    pid_t pid;
    if((pid = fork()) < 0){
        printf(EXEC_ERROR, "Fork Error");
    }
    return pid;
}

handler_t *Signal(int signum, handler_t *handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* Block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* Restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
    printf(EXEC_ERROR,"Signal error");
    return (old_action.sa_handler);
}

void Sigemptyset(sigset_t *set)
{
    if (sigemptyset(set) < 0)
    printf(EXEC_ERROR,"Sigemptyset error");
    return;
}

void Sigaddset(sigset_t *set, int signum)
{
    if (sigaddset(set, signum) < 0)
    printf(EXEC_ERROR,"Sigaddset error");
    return;
}
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    if (sigprocmask(how, set, oldset) < 0)
    printf(EXEC_ERROR,"Sigprocmask error");
    return;
}
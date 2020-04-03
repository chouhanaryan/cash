#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "cash.h"


/* To change the color of shell prompt */
void green() {
    printf("\033[1;32m");
}
void blue() {
    printf("\033[1;34m");
}
void reset() {
    printf("\033[0m");
}

/* To print the prompt in the form "<user>@<host> <cwd> >" */
void prompt(void) {	
    char hostname[1204] = "";
	if (gethostname(hostname, sizeof(hostname)) == -1 ) {
        perror("hostname");
    } else {
	    // printf("%s@%s:%s> ", getenv("LOGNAME"), hostname, getcwd(currentDirectory, 1024));	    
        green();
        printf("%s@%s", getenv("LOGNAME"), hostname);
        printf(":");
        blue();
        printf("%s> ", getcwd(currentDirectory, 1024));
        reset();        
    }
}

/* To take user input */
char *input(void) {
    size_t size;
    char *input = NULL;
    int ret = getline(&input, &size, stdin);

    // Ctrl + D to handle EOF signal and exit shell
    if (ret == -1) {
        printf("\n");
        exit(EXIT_SUCCESS);
    }
    
    return input;
}

/* To parse user input into (command + args) */
char **parser(char *input) {
    char **command = malloc(BUFSIZ * sizeof(char *));
    char *separator = " \n";
    char *parsed;
    int bufsiz = BUFSIZ, index = 0;

    if (!command) {
        printf("Could not allocate memory\n");
        exit(EXIT_FAILURE);
    }

    parsed = strtok(input, separator);
    while (parsed != NULL) {
        command[index] = parsed;
        index++;
        if(index > bufsiz) {
            bufsiz += BUFSIZ;
            command = realloc(command, bufsiz * sizeof(char *));
            if (!command) {
                printf("Could not allocate memory\n");
                exit(EXIT_FAILURE);
            }
        }
        parsed = strtok(NULL, separator);
    }
    command[index] = NULL;
    return command;
}

/* To manage the shell's environment variables */
void environment(char **command, int option){
	char **var;
	switch (option) {
		// 'environ' command to print all environment variables along with their values
		case 0: { 
            for (var = environ; *var != 0; var ++) {
                printf("%s\n", *var);
            }
            break;
        }

        // 'getenv' command to print the value of an environment variable
        case 1: {
            char *env_var;
            if (command[1] == NULL) {
                printf("%s\n","Not enough input arguments");
            } else {
                if ((env_var = getenv(command[1])) != NULL) {
                    printf("%s\n", env_var);
                } else {
                    printf("%s", "The variable does not exist\n");
                }
            }
            break;
        }

		// 'setenv' command to set an environment variable to a value
		case 2: {
            if ((command[1] == NULL) && command[2] == NULL) {
                printf("%s\n","Not enough input arguments");
                break;
            }
            
            // Different outputs for new and overwritten variables
            if (getenv(command[1]) != NULL) {
                printf("%s\n", "The variable has been overwritten");
            } else {                
                printf("%s\n", "The variable has been created");
            }
            
            // If we specify no value for the variable, we set it to ""
            if (command[2] == NULL) {
                setenv(command[1], "", 1);
            // We set the variable to the given value
            } else {
                setenv(command[1], command[2], 1);
            }
            break;
        }

		// 'unsetenv' command to delete an environment variable
		case 3: {
            if (command[1] == NULL) {
                printf("%s\n","Not enough input arguments");
                break;
            }
            if (getenv(command[1]) != NULL) {
                unsetenv(command[1]);
                printf("%s", "The variable has been erased\n");
            } else {
                printf("%s", "The variable does not exist\n");
            }
            break;
        }
	}
}

/* To execute a command */
void execute(char **command) {
    pid_t child_pid;
    int stat_loc; // stores the status of waitpid()

    // blank input    
    if (command[0] == NULL) {
        return;

    // 'exit' command to exit shell        
    } else if (strcmp(command[0], "exit") == 0) {
            exit(EXIT_SUCCESS);    

    // 'cd' command to change pwd    
    } else if (strcmp(command[0], "cd") == 0) {
        if (command[1] == NULL) {
            char *home = getenv("HOME");
            if (chdir(home) != 0) {
                perror("home");
            }
        } else {
            if (chdir(command[1]) != 0) {
                perror(command[1]);
            }
        }
        return;

    // 'environ' command to list the environment variables
    } else if (strcmp(command[0], "environ") == 0) {
        environment(command, 0);

    // 'getenv' command to get the value of an environment variable 
    } else if (strcmp(command[0], "getenv") == 0) {
        environment(command, 1);

    // 'setenv' command to set environment variables
    } else if (strcmp(command[0], "setenv") == 0) {
        environment(command, 2);

    // 'unsetenv' command to undefine environment variables
    } else if (strcmp(command[0], "unsetenv") == 0)  {
        environment(command, 3);

    // external commands
    } else {
        child_pid = fork();
        if (child_pid == 0) {
            /* Never returns if the call is successful */
            execvp(command[0], command);
            printf("This won't be printed if execvp is successul\n");
        } else {
            waitpid(child_pid, &stat_loc, WUNTRACED);
        }
    }
}

int main(int argc, char *argv[], char **envp) {    
    environ = envp;
    setenv("SHELL", getcwd(currentDirectory, 1024), 1);

    while (TRUE) {
        char **command = NULL;
        prompt();
        command = parser(input());
        execute(command);
        free(command);
    }
}
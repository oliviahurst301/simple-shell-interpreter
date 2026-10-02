#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <readline/readline.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

/* ***GLOABL VARIABLES*** */

/* must be access by both sigint_handler and main */
volatile sig_atomic_t foreground_pid = 0;

/* keep track of whether interrupt happened */
volatile sig_atomic_t sigint_received = 0;

/* ********************** */

/* linked list struct to store info of background processes */
struct bg_process {
    pid_t bg_pid;
    char *execution_args;
    struct bg_process *next;
};

void sigint_handler(int sig) {
    
    /* if foreground_pid > 0, a child process is running */
    if (foreground_pid > 0) {
        
        /* send SIGINT to foreground child process */
        kill(foreground_pid, SIGINT);
    } 
    /* move to new line when no foreground child process is running */
    else {
        sigint_received = 1;
    }
}

int check_sigint(void) {
    if (sigint_received) {
        rl_done = 1;
    }
    return 0;
}

void check_bg_processes(struct bg_process **head) {
    
    int status;
    struct bg_process *current = *head;
    struct bg_process *previous = NULL;

    /* check status of background proccesses */
    while (current != NULL) {
        int waitpid_result = waitpid(current -> bg_pid, &status, WNOHANG);

        /* check for errors */
        if (waitpid_result == -1) {
            perror("error");
            previous = current;
            current = current -> next;
        }

        /* child proccess still going, move to next node */
        else if (waitpid_result == 0) {
            previous = current;
            current = current -> next;
        }
        
        /* if pid > 0 returned, child has finished */
        else if (waitpid_result > 0) {
            printf("%d: %s has terminated.\n", current -> bg_pid, current -> execution_args);

            /* save next node before freeing current node */
            struct bg_process *next = current -> next;
            
            /* remove current from list */
            if (previous == NULL) {
                *head = current -> next;
            } 
            else {
                previous -> next = current -> next;
            }

            free(current -> execution_args);
            free(current);

            current = next;
        }
    }
}

void print_bglist(struct bg_process *head) {
    int count = 0;
    struct bg_process *current = head;

    /* go through list of bg processes, display info */
    while (current != NULL) {
        printf("%d: %s\n", current -> bg_pid, current -> execution_args);
        count++;
        current = current -> next;
    }
    printf("Total Background jobs:  %d\n", count);
}

void create_prompt(char *prompt, size_t size) {
    
     /* HOST_NAME_MAX defined in <limits.h>,
    add 1 to account for null terminator */
    char hostname[HOST_NAME_MAX + 1];

    /* PATH_MAX defined in <limits.h> */
    char cwd[PATH_MAX];
    
    const char *username = getlogin();

    if (username == NULL) {
        perror("getlogin() failed");
        exit(1);
    }
        
    /* check if gethostname was successful */
    if (gethostname(hostname, sizeof(hostname)) == -1) {
        perror("error, no hostname");
        exit(1);
    } 
        
    /* check if getcwd was successful */
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("error, cannot find cwd");
        exit(1);
    }

    /* store formatted prompt in memory, return val stored in prompt_len */
    int prompt_len = snprintf(prompt, size, "%s@%s: %s > ", username, hostname, cwd);

    /* check for error */
    if (prompt_len < 0) {
        perror("error creating prompt\n");
        exit(1);
    }

    /* check if prompt was truncated */
    else if (prompt_len >= size) {
        perror("prompt was truncated\n");
        exit(1);
    } 
}

void change_directory(char **args) {

    int chdir_result;

    if (args[1] == NULL || strcmp(args[1], "~") == 0) {
        
        /* find users home directory */
        const char *home_dir = getenv("HOME");

        /* check if getenv() succeeded */
        if (home_dir == NULL) {
            perror("error finding home directory");
            return;
        } 
        else {
            chdir_result = chdir(home_dir);
        }
    }
    else {
        chdir_result = chdir(args[1]);
    }
    
    /* check if chdir succeeded */
    if (chdir_result == -1) {
        perror("change directory failed");
    }
}

pid_t start_bg_process (char **args) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return -1;
    }/* child process */
    else if (pid == 0) {

        /* bg process should ignore SIGINT */
        signal(SIGINT, SIG_IGN);
        
        /* receives command and args after bg */
        execvp(args[1], args + 1);

        /* only reached if execvp fails */
        perror("execvp failed");
        exit(1);
    } 
    /* parent process */
    else {
        return pid;
    }
}

void add_bg_process(struct bg_process **head, pid_t pid, char *command) {
    
    /* allocate space for another bg process node */
    struct bg_process *new_node = malloc(sizeof(struct bg_process));

    if (new_node == NULL) {
        perror("memory allocation failed");
        free(command);
        return;
    }

    new_node -> bg_pid = pid;
    new_node -> execution_args = command;
    new_node -> next = *head;
    *head = new_node;
}

void execute_foreground(char **args) {
    /* no background process, continue as normal */
    pid_t pid = fork();

    /* fork fail */
    if (pid < 0) {
        perror("fork failed");
        return;
    }
    
    /* in child process */
    else if (pid == 0) {
        execvp(args[0], args);
        
        /* this section only reached if execvp fails */
        fprintf(stderr, "%s\n", strerror(errno));
        exit(1);
    }

    /* in parent process */
    else {
        
        foreground_pid = pid;

        int wait_result;

        int status;

        do {
            wait_result = waitpid(pid, &status, 0);
        }
        while (wait_result == -1 && errno == EINTR);

        /* if wait fails, it returns -1 */
        if (wait_result == -1) {
            perror("waitpid error");
        } 

        /* child process is done */
        foreground_pid = 0;
    }
}

char **tokenize_args(char *input) {
    char **args = NULL;
    char *token;
    int num_args = 0;

    token = strtok(input, " ");

    /* continue parsing same string */
    while (token != NULL) {

        /* dynamically increase size of args_temp */
        char **args_temp = realloc(args, (num_args + 1) * sizeof(char *));

        /* make sure memory allocation was successful
        if unsuccessful, exit while loop */
        if (args_temp == NULL) {
            printf("realloc failed\n");
            free(args);
            return NULL;
        }
        args = args_temp;
        args[num_args] = token;
        num_args++;

        token = strtok(NULL, " ");
    }

    /* create room for & store last NULL element */
    char **args_temp = realloc(args, (num_args + 1) * sizeof(char *));

    if (args_temp == NULL) {
        printf("something went wrong, please enter a new command");
        free(input);
        free(args);
        return NULL;
    }
    args = args_temp;
    args[num_args] = NULL;

    return args;
}

int main() {

    /* set up SIGINT handling */
    struct sigaction sa;

    sa.sa_handler = sigint_handler;
    
    int sigemptyset_success = sigemptyset(&sa.sa_mask);

    /* terminate ssi if sigemptyset fails */
    if (sigemptyset_success == -1) {
        perror("sigemptyset failed");
        exit(1);
    }

    sa.sa_flags = 0;
    
    int sigaction_success = sigaction(SIGINT, &sa, NULL);

    /* terminate ssi if sigaction fails */
    if (sigaction_success == -1) {
        perror("sigaction failed");
        exit(1);
    }

    rl_event_hook = check_sigint;

    /* other variables for later use */

    int num_args = 0;

    struct bg_process *head = NULL;

    while (1) {

        bool realloc_failed = false;

        int ret_home_dir;

        char *new_dir_path;

        /* size of prompt must account for size of username, host, other characters & spaces */
        char prompt[HOST_NAME_MAX + PATH_MAX + 50];

        create_prompt(prompt, sizeof(prompt));
       
        /* display prompt, check for user input */
        char *input = readline(prompt);

        if (sigint_received) {
            sigint_received = 0;
            
            if (input != NULL) {
                free(input);
            } 
            continue;
        } 

        /* check if ctrl + D was pressed (EOF) */
        if (input == NULL) {
            break;
        }

        /* if user presses Enter, restart loop, display prompt again */
        else if (*input == '\0') {
            
            /* free allocated memory first */
            free(input);
            continue;
        }
        char **args = tokenize_args(input);

        if (args == NULL) {
            free(input);
            continue;
        }

        check_bg_processes(&head);

        if (strcmp(args[0], "cd") == 0) {
            change_directory(args);

            free(input);
            free(args);
            continue;
        }

        /* check if user entered bglist */
        else if (strcmp(args[0], "bglist") == 0) {
            print_bglist(head);

            free(input);
            free(args);
            continue;
        }        

        /* check if we want to execute background process */
        else if (strcmp(args[0], "bg") == 0) {

            if (args[1] == NULL) {
                fprintf(stderr, "improper bg command\n");
                free(input);
                free(args);
                continue;
            } 
            
            pid_t bg_pid = start_bg_process(args);
            
            if (bg_pid == -1) {
                free(input);
                free(args);
                continue;
            }
            
            /* determine how much memory is needed */
            int which_len = strlen("which ") + strlen(args[1]) + 1; /* +1 for null terminator */

            /* allocate amount of memory from above */
            char *which_command = malloc(which_len);

            /* ensure allocation success */
            if (which_command == NULL) {
                perror("allocation failed");
                free(input);
                free(args);
                continue;
            }

            /* store result into allocated memory */
            int which_result = snprintf(which_command, which_len, "which %s", args[1]);

            /* check if encountered an error or was truncated */
            if (which_result < 0 || which_result >= which_len) {
                fprintf(stderr, "failed to create which command\n");
                free(input);
                free(args);
                free(which_command);
                continue;
            }  

            /* check command to run & its output */
            FILE *which_pipe = popen(which_command, "r");

            if (which_pipe == NULL) {
                perror("popen failed");
                free(input);
                free(args);
                free(which_command);
                continue;
            } 

            char absolute_path[PATH_MAX];

            /* store line from which_pipe in char array, make sure it worked */
            if (fgets(absolute_path, sizeof(absolute_path), which_pipe) == NULL) {
                fprintf(stderr, "no output produced\n");
                pclose(which_pipe);
                free(input);
                free(args);
                free(which_command);
                continue;
            } 
                
            /* upon success, replace newline char which null terminator */
            else {
                absolute_path[strcspn(absolute_path, "\n")] = '\0';
            }

            int close_result = pclose(which_pipe);

            /* make sure file closed properly */
            if (close_result == -1) {
                perror("pclose failed");
                free(input);
                free(args);
                free(which_command);
                continue;
            } 
            else {
                free(which_command);
            }

            /* calculate space needed for command + arguments */
            int bg_args_len = strlen(absolute_path) + 1; /* to account for '\0' */

            /* start at 2 to account for args after command */
            for (int i = 2; i < num_args; i++) {
                bg_args_len += (strlen(args[i]) + 1);
            }

            /* allocate str */
            char *bg_args = malloc(bg_args_len);

            add_bg_process(&head, bg_pid, bg_args);

            if (bg_args == NULL) {
                perror("allocation failed");
                free(input);
                free(args);
                continue;
            } 
                    
            /* replace bg with null character */
            else {
                bg_args[0] = '\0';

                /* put absolute_path into bg_args */
                strcat(bg_args, absolute_path);

                /* add remaining args */
                for (int i = 2; i < num_args; i++) {
                    strcat(bg_args, " ");
                    strcat(bg_args, args[i]);
                }
            }
            free(input);
            free(args);
            continue;
        }
        else {
            execute_foreground(args);
            
            /* free allocated memory */
            free(input);
            free(args);
            continue;
        }
    return 0;
    }
}
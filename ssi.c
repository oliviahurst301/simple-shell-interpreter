#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <readline/readline.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {

    /* HOST_NAME_MAX defined in <limits.h>,
    add 1 to account for null terminator */
    char hostname[HOST_NAME_MAX + 1];

    /* PATH_MAX defined in <limits.h> */
    char cwd[PATH_MAX];

    /* separate arguments by spaces */
    const char *delimiter = " ";

    char *token;

    while (1) {

        const char *username = getlogin();

        if (username == NULL) {
            perror("getlogin() failed");
            exit(1);
        }

        char **args = NULL;

        char **args_temp = NULL;

        bool realloc_failed = false;

        pid_t pid;

        /* used to keep track of num elements in args[] */
        int num_args = 0;
        
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

        /* size of prompt must account for size of username, host, other characters & spaces */
        char prompt[strlen(username) + HOST_NAME_MAX + PATH_MAX + 7];
        
        /* store formatted prompt in array, return val stored in prompt_len */
        int prompt_len = snprintf(prompt, sizeof(prompt), "%s@%s: %s > ", username, hostname, cwd);
        if (prompt_len < 0) {
            perror("error creating prompt\n");
            exit(1);
        }
        else if (prompt_len >= sizeof(prompt)) {
            perror("prompt was truncated\n");
            exit(1);
        } 
       
        /* display prompt, check for user input */
        char *input = readline(prompt);

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
        
        /* get first argument token */
        token = strtok(input, delimiter);

        /* continue parsing same string */
        while (token != NULL) {

            /* dynamically increase size of args_temp */
            args_temp = realloc(args, (num_args + 1) * sizeof(char *));

            /* make sure memory allocation was successful
            if unsuccessful, exit while loop */
            if (args_temp == NULL) {
                printf("realloc failed");
                realloc_failed = true;
                break;
            }
            else {
                args = args_temp;
            }

            /* store tokens in args */
            args[num_args] = token;
            num_args++;
            token = strtok(NULL, delimiter);
        }

        /* if memory allocation failed, free memory
        and restart while loop */
        if (realloc_failed) {
            printf("something went wrong, please enter a new command");
            free(input);
            free(args);
            continue;
        }
        /* create room for & store last NULL element */
        args_temp = realloc(args, (num_args + 1) * sizeof(char *));
        
        if (args_temp == NULL) {
            printf("something went wrong, please enter a new command");
            free(input);
            free(args);
            continue;
        }
        else {
            args = args_temp;
        }
        args[num_args] = NULL;
        
        pid = fork();

        /* fork fail */
        if (pid < 0) {
            free(input);
            free(args);
            perror("fork failed");
            continue;
        }
        /* in child process */
        else if (pid == 0) {
            execvp(args[0], args);
            
            /* this section only reached if execvp fails */
            perror("execvp failed");
            free(input);
            free(args);
            exit(1);
        }

        /* in parent process */
        else {
            /* wait until child process is done */
            int wait_result = wait(NULL);

            /* if wait fails, it returns -1 */
            if (wait_result == -1) {
                perror("wait failed");
                free(input);
                free(args);
                exit(1);
            } 

            /* free allocated memory */
            free(input);
            free(args);
            continue;
        }
    } 
    return 0;
}
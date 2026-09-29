#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <readline/readline.h>
#include <stdbool.h>

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

        char **args = NULL;

        char **args_temp = NULL;

        bool realloc_failed = false;

        /* used to keep track of num elements in args[] */
        int num_args = 0;
        
        /* check if gethostname was successful */
        if (gethostname(hostname, sizeof(hostname)) == -1) {
            printf("error, no hostname");
        } 
        
        /* check if getcwd was successful */
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            printf("error, cannot find cwd");
        }

        /* size of prompt must account for size of username, host, other characters & spaces */
        char prompt[strlen(username) + HOST_NAME_MAX + PATH_MAX + 7];
        
        /* store formatted prompt in array, return val stored in prompt_len */
        int prompt_len = snprintf(prompt, sizeof(prompt), "%s@%s: %s > ", username, hostname, cwd);
        if (prompt_len < 0) {
            printf("error");
        }
        else if (prompt_len >= sizeof(prompt)) {
            printf("prompt was truncated");
        } 
       
        /* display prompt, check for user input */
        char *input = readline(prompt);

        /* check if ctrl + D was pressed (EOF) */
        if (input == NULL) {
            printf("Ending session, ");
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

            /* store pointers to tokens in args */
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
            printf("something went wrong, please enter a new command (2)");
            free(input);
            free(args);
            continue;
        }
        else {
            args = args_temp;
        }
        args[num_args] = NULL;
        
        /* temp test tokenization */
        for (int i = 0; i < num_args; i++) {
            printf("%s\n", args[i]);
        }
        
        free(input);
        free(args);
    } 
    
    printf("bye bye\n");
    return 0;

}
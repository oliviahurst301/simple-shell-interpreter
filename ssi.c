#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <readline/readline.h>
#include <readline/history.h>

int main() {

    const char *username = getlogin();
    
    /* HOST_NAME_MAX defined in <limits.h>,
    add 1 to account for null terminator */
    char hostname[HOST_NAME_MAX + 1];
    if (gethostname(hostname, sizeof(hostname)) == -1) {
        printf("error, no hostname");
    } 
    else {
        printf("%s\n", hostname);
    }
    // PATH_MAX defined in <limits.h>
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        printf("error, cannot find cwd");
    }
    else {
        printf("%s\n", cwd);
    }

    char prompt[strlen(username) + HOST_NAME_MAX + PATH_MAX + 6];

    int prompt_len = snprintf(prompt, sizeof(prompt), "%s@%s: %s >", username, hostname, cwd);
    if (prompt_len < 0) {
        printf("error");
    }
    else if (prompt_len >= sizeof(prompt)) {
        printf("prompt was truncated");
    } 
    else {
        printf("%s\n", prompt);
    }
    
    // const char* prompt = "username@hostname: /home/username >";
    // const char* prompt = ({"%s\n"}, username,"@",{"%s\n"}, hostname,":", pwd)

    int end_session = 0;

   /* while (!end_session) {
       // display prompt
       printf("SSI >");
       // read user input
       char *input = readline(prompt);

       // check if there is any input
       if (input == NULL) {
        return 0;
       }
       else if (*input != '\0') {
        // add to history
       }
       // check if end_session is still true

       // something ends session
       // end_session = 1;

       // free allocated memory
        free(input);
    } */
    
    printf("See ya");
    return 0;

}
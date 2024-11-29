//Brian Cabrera Diaz

/*

11/12 - No real goals for today, just get makefile done and copy over the example and work for a bit on it at the code party

*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>
#include <signal.h>

#include "cmd_parse.h"

// Chaney says this is a global used for good
/*
im assuming the reason this is a global is due to the fact that verbose is 
done everywhere in a program and having to manage that everywhere in the
program would be actual asinine 
*/
extern unsigned short is_verbose;

void handle_sigint(int sig);

// Signal handler for SIGINT
void handle_sigint(int sig) {
    
    char cwd_buf[1000] = {'\0'};
    char sys_name_buf[71] = {'\0'};

    (void)sig;
    // Print the witty message
    printf("\nIt takes more than that to kill me\n");

    // Re-display the prompt
    getcwd(cwd_buf, sizeof(cwd_buf));
    gethostname(sys_name_buf, sizeof(sys_name_buf));

    printf("%s %s\n %s %s # ",
           PROMPT_STR, cwd_buf,
           getenv("LOGNAME"), sys_name_buf);
    fflush(stdout); // Ensure the prompt is immediately displayed
}

int main(int argc, char *argv[]) {
    struct sigaction sa; 
    int ret;             

    // Set up the SIGINT handler
    memset(&sa, 0, sizeof(sa));   
    sa.sa_handler = handle_sigint; 
    sa.sa_flags = SA_RESTART;       
    sigemptyset(&sa.sa_mask);      

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction failed");
        exit(EXIT_FAILURE);
    }

    // Parse command-line arguments
    simple_argv(argc, argv);

    // Start processing user input (the main shell loop)
    ret = process_user_input_simple(); // Assigned value after declaration

    return ret;
}

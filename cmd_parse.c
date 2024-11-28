/// rchaney@pdx.edu

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#include "cmd_parse.h"

#define PROMPT_LEN 2000

// / new change im making my comments either blocks or have the " /"
// / my #defines
#define HIST 10


// I have this a global so that I don't have to pass it to every
// function where I might want to use it. Yes, I know global variables
// are frowned upon, but there are a couple useful uses for them.
// This is one.
unsigned short is_verbose = 0;

char *history_list[HIST] = {NULL};
int history_count = 0; // Number of valid commands added to history

// Signal handler for SIGINT
void handle_sigint(int sig) {
    printf("\nIt takes more than that to kill me\n");
    fflush(stdout); // Ensure the message is displayed immediately
}

int 
process_user_input_simple(void)
{
    char str[MAX_STR_LEN] = {'\0'};
    char *ret_val = NULL;
    char *raw_cmd = NULL;
    cmd_list_t *cmd_list = NULL;
    int cmd_count = 0;
    char prompt[PROMPT_LEN] = {'\0'};

    /*
    variables I have deemed neccessary
    */
    char cwd_buf[1000] = {'\0'};
    char sys_name_buf[71] = {'\0'};

    memset(history_list, 0, sizeof(history_list));

    for ( ; ; ) {
        // Set up a cool user prompt.
        // test to see of stdout is a terminal device (a tty)
        /*
        before this first print I need to find the current working directory and the system name.
        */
       getcwd(cwd_buf, 100);
       gethostname(sys_name_buf, 71);
       

        sprintf(prompt, " %s %s\n %s %s # "
                , PROMPT_STR
                , cwd_buf
                , getenv("LOGNAME")
                , sys_name_buf
            );
        fputs(prompt, stdout);
        memset(str, 0, MAX_STR_LEN);
        ret_val = fgets(str, MAX_STR_LEN, stdin);

        if (NULL == ret_val) {
            // end of input, a control-D was pressed.
            // Bust out of the input loop and go home.
            break;
        }

        // STOMP on the pesky trailing newline returned from fgets().
        if (str[strlen(str) - 1] == '\n') {
            // replace the newline with a NULL
            str[strlen(str) - 1] = '\0';
        }
        if (strlen(str) == 0) {
            // An empty command line.
            // Just jump back to the promt and fgets().
            // Don't start telling me I'm going to get cooties by
            // using continue.
            continue;
        }

        if (strcmp(str, BYE_CMD) == 0) {
            // Pickup your toys and go home. I just hope there are not
            // any memory leaks. ;-)

            //once we close the shell using the bye command we need to make sure any lists are deleted and properly killed in the back alley

            
        //this handles the history list clean up
        for (int i = 0; i < HIST; i++) {
            if (history_list[i] != NULL) {
                free(history_list[i]);
                history_list[i] = NULL;
            }
        }
            break;
        }

        // I put the update of the history of command in here.
        if(strcmp(str, HISTORY_CMD) != 0){
                free(history_list[HIST - 1]);

                memmove(&(history_list[1]), &(history_list[0]), (HIST - 1) * sizeof(char*));
                history_list[0] = strdup(str);
        }

        // Basic commands are pipe delimited.
        // This is really for Stage 2.
        raw_cmd = strtok(str, PIPE_DELIM);

        cmd_list = (cmd_list_t *) calloc(1, sizeof(cmd_list_t));

        // This block should probably be put into its own function.
        cmd_count = 0;
        while (raw_cmd != NULL ) {
            cmd_t *cmd = (cmd_t *) calloc(1, sizeof(cmd_t));

            cmd->raw_cmd = strdup(raw_cmd);
            cmd->list_location = cmd_count++;

            if (cmd_list->head == NULL) {
                // An empty list.
                cmd_list->tail = cmd_list->head = cmd;
            }
            else {
                // Make this the last in the list of cmds
                cmd_list->tail->next = cmd;
                cmd_list->tail = cmd;
            }
            cmd_list->count++;

            // Get the next raw command.
            raw_cmd = strtok(NULL, PIPE_DELIM);
        }
        // Now that I have a linked list of the pipe delimited commands,
        // go through each individual command.
        parse_commands(cmd_list);

        // This is a really good place to call a function to exec the
        // the commands just parsed from the user's command line.
        exec_commands(cmd_list);

        // We (that includes you) need to free up all the stuff we just
        // allocated from the heap. That linked list of linked lists looks
        // like it will be nasty to free up, but just follow the memory.
        free_list(cmd_list);
        cmd_list = NULL;
    }

    return(EXIT_SUCCESS);
}

void 
simple_argv(int argc, char *argv[] )
{
    int opt;

    while ((opt = getopt(argc, argv, "hv")) != -1) {
        switch (opt) {
        case 'h':
            // help
            // Show something helpful
            fprintf(stdout, "You must be out of your Vulcan mind if you think\n"
                    "I'm going to put helpful things in here.\n\n");
            exit(EXIT_SUCCESS);
            break;
        case 'v':
            // verbose option to anything
            // I have this such that I can have -v on the command line multiple
            // time to increase the verbosity of the output.
            is_verbose++;
            if (is_verbose) {
                fprintf(stderr, "verbose: verbose option selected: %d\n"
                        , is_verbose);
            }
            break;
        case '?':
            fprintf(stderr, "*** Unknown option used, ignoring. ***\n");
            break;
        default:
            fprintf(stderr, "*** Oops, something strange happened <%c> ... ignoring ...***\n", opt);
            break;
        }
    }
}

void exec_commands(cmd_list_t *cmds) {
    cmd_t *cmd = cmds->head;
    int p_trail = -1; // File descriptor for the previous command's pipe read end
    int pipe_fd[2];   // File descriptors for the current pipe
    pid_t pid;
    int is_last = 0;  // Flag for detecting the last command in the pipeline

    while (cmd != NULL) {
        // Check if this is the last command in the pipeline
        is_last = (cmd->next == NULL);

        // Handle built-in commands
        if (cmd->cmd && strcmp(cmd->cmd, CWD_CMD) == 0) {
            char cwd[MAXPATHLEN];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("cwd: %s\n", cwd);
            } else {
                perror("getcwd failed");
            }
            return; // Built-in commands don't need further processing
        } else if (cmd->cmd && strcmp(cmd->cmd, CD_CMD) == 0) {
            if (cmd->param_count == 0) {
                // Change to home directory
                if (chdir(getenv("HOME")) != 0) {
                    perror("cd failed");
                }
            } else {
                // Change to specified directory
                if (chdir(cmd->param_list->param) != 0) {
                    perror("cd failed");
                }
            }
            return;
        } else if (cmd->cmd && strcmp(cmd->cmd, BYE_CMD) == 0) {
            // Exit the shell
            exit(EXIT_SUCCESS);
        } else if (cmd->cmd && strcmp(cmd->cmd, ECHO_CMD) == 0) {
            // Echo command
            param_t *param = cmd->param_list;
            while (param) {
                printf("%s ", param->param);
                param = param->next;
            }
            printf("\n");
            return;
        } else if (cmd->cmd && strcmp(cmd->cmd, HISTORY_CMD) == 0) {
            for (int i = 0; i < HIST && history_list[i]; i++) {
                printf("%d: %s\n", i + 1, history_list[i]);
            }
            return;
        }

        // Create a pipe unless it's the last command
        if (!is_last && pipe(pipe_fd) < 0) {
            perror("pipe failed");
            exit(EXIT_FAILURE);
        }

        // Fork a child process
        pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) { // Child process
            // Handle input redirection
            if (cmd->input_src == REDIRECT_FILE && cmd->input_file_name != NULL) {
                int fd = open(cmd->input_file_name, O_RDONLY);
                if (fd < 0) {
                    perror("Input redirection failed");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            } else if (p_trail != -1) {
                // Redirect input to read from the previous command's pipe
                dup2(p_trail, STDIN_FILENO);
            }

            // Handle output redirection
            if (cmd->output_dest == REDIRECT_FILE && cmd->output_file_name != NULL) {
                int fd = open(cmd->output_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    perror("Output redirection failed");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            } else if (!is_last) {
                // Redirect output to write to the current pipe
                dup2(pipe_fd[1], STDOUT_FILENO);
            }

            // Close unnecessary file descriptors in the child
            if (p_trail != -1) {
                close(p_trail);
            }
            if (!is_last) {
                close(pipe_fd[0]);
                close(pipe_fd[1]);
            }

            // Prepare the argument list
            int argc = cmd->param_count + 2; // +1 for cmd->cmd, +1 for NULL
            char *argv[MAX_STR_LEN]; // Use a fixed size for argv
            argv[0] = cmd->cmd;
            param_t *param = cmd->param_list;
            int i;
            for (i = 1; i < argc - 1; i++) {
                argv[i] = param->param;
                param = param->next;
            }
            argv[i] = NULL; // Null-terminate the argument list

            // Execute the command
            execvp(cmd->cmd, argv);
            perror("execvp failed");
            exit(EXIT_FAILURE);
        } else { // Parent process
            // Close file descriptors no longer needed by the parent
            if (p_trail != -1) {
                close(p_trail);
            }
            if (!is_last) {
                close(pipe_fd[1]);
            }

            // Update p_trail to the read end of the current pipe
            p_trail = is_last ? -1 : pipe_fd[0];
        }

        // Move to the next command in the pipeline
        cmd = cmd->next;
    }

    // Wait for all child processes to finish
    while (wait(NULL) > 0);
}

void
free_list(cmd_list_t *cmd_list)
{
    // Proof left to the student.
    // You thought I was going to do this for you! HA! You get
    // the enjoyment of doing it for yourself.
    cmd_t *current = cmd_list->head;
    cmd_t *next = NULL;

    //this was not enjoyable
    if (cmd_list == NULL) {
        return;
    }

    while (current != NULL) {
        next = current->next;
        free_cmd(current);
        current = next;
    }

    // Free the cmd_list structure itself
    free(cmd_list);
}

void
print_list(cmd_list_t *cmd_list)
{
    cmd_t *cmd = cmd_list->head;

    while (NULL != cmd) {
        print_cmd(cmd);
        cmd = cmd->next;
    }
}

void
free_cmd (cmd_t *cmd)
{
    // Proof left to the student.
    // Yep, on yer own.
    // I beleive in you.
    param_t *param = NULL;

    if (cmd == NULL) {
        return;
    }

    // Free the raw_cmd string
    if (cmd->raw_cmd != NULL) {
        free(cmd->raw_cmd);
        cmd->raw_cmd = NULL;
    }

    // Free the cmd string
    if (cmd->cmd != NULL) {
        free(cmd->cmd);
        cmd->cmd = NULL;
    }

    // Free the input file name
    if (cmd->input_file_name != NULL) {
        free(cmd->input_file_name);
        cmd->input_file_name = NULL;
    }

    // Free the output file name
    if (cmd->output_file_name != NULL) {
        free(cmd->output_file_name);
        cmd->output_file_name = NULL;
    }

    // Free the parameter list
    param = cmd->param_list;
    while (param != NULL) {
        param_t *next_param = param->next;
        if (param->param != NULL) {
            free(param->param);
        }
        free(param);
        param = next_param;
    }

    // Free the cmd structure itself
    free(cmd);
}

// Oooooo, this is nice. Show the fully parsed command line in a nice
// easy to read and digest format.
void
print_cmd(cmd_t *cmd)
{
    param_t *param = NULL;
    int pcount = 1;

    fprintf(stderr,"raw text: +%s+\n", cmd->raw_cmd);
    fprintf(stderr,"\tbase command: +%s+\n", cmd->cmd);
    fprintf(stderr,"\tparam count: %d\n", cmd->param_count);
    param = cmd->param_list;

    while (NULL != param) {
        fprintf(stderr,"\t\tparam %d: %s\n", pcount, param->param);
        param = param->next;
        pcount++;
    }

    fprintf(stderr,"\tinput source: %s\n"
            , (cmd->input_src == REDIRECT_FILE ? "redirect file" :
               (cmd->input_src == REDIRECT_PIPE ? "redirect pipe" : "redirect none")));
    fprintf(stderr,"\toutput dest:  %s\n"
            , (cmd->output_dest == REDIRECT_FILE ? "redirect file" :
               (cmd->output_dest == REDIRECT_PIPE ? "redirect pipe" : "redirect none")));
    fprintf(stderr,"\tinput file name:  %s\n"
            , (NULL == cmd->input_file_name ? "<na>" : cmd->input_file_name));
    fprintf(stderr,"\toutput file name: %s\n"
            , (NULL == cmd->output_file_name ? "<na>" : cmd->output_file_name));
    fprintf(stderr,"\tlocation in list of commands: %d\n", cmd->list_location);
    fprintf(stderr,"\n");
}

// Remember how I told you that use of alloca() is
// dangerous? You can trust me. I'm a professional.
// And, if you mention this in class, I'll deny it
// ever happened. What happens in stralloca stays in
// stralloca.
#define stralloca(_R,_S) {(_R) = alloca(strlen(_S) + 1); strcpy(_R,_S);}

void
parse_commands(cmd_list_t *cmd_list)
{
    cmd_t *cmd = cmd_list->head;
    char *arg;
    char *raw;

    while (cmd) {
        // Because I'm going to be calling strtok() on the string, which does
        // alter the string, I want to make a copy of it. That's why I strdup()
        // it.
        // Given that command lines should not be tooooo long, this might
        // be a reasonable place to try out alloca(), to replace the strdup()
        // used below. It would reduce heap fragmentation.
        //raw = strdup(cmd->raw_cmd);

        // Following my comments and trying out alloca() in here. I feel the rush
        // of excitement from the pending doom of alloca(), from a macro even.
        // It's like double exciting.
        stralloca(raw, cmd->raw_cmd);

        arg = strtok(raw, SPACE_DELIM);
        if (NULL == arg) {
            // The way I've done this is like ya'know way UGLY.
            // Please, look away.
            // If the first command from the command line is empty,
            // ignore it and move to the next command.
            // No need free with alloca memory.
            //free(raw);
            cmd = cmd->next;
            // I guess I could put everything below in an else block.
            continue;
        }
        // I put something in here to strip out the single quotes if
        // they are the first/last characters in arg.
        if (arg[0] == '\'') {
            arg++;
        }
        if (arg[strlen(arg) - 1] == '\'') {
            arg[strlen(arg) - 1] = '\0';
        }
        cmd->cmd = strdup(arg);
        // Initialize these to the default values.
        cmd->input_src = REDIRECT_NONE;
        cmd->output_dest = REDIRECT_NONE;

        while ((arg = strtok(NULL, SPACE_DELIM)) != NULL) {
            if (strcmp(arg, REDIR_IN) == 0) {
                // redirect stdin

                //
                // If the input_src is something other than REDIRECT_NONE, then
                // this is an improper command.
                //

                // If this is anything other than the FIRST cmd in the list,
                // then this is an error.

                cmd->input_file_name = strdup(strtok(NULL, SPACE_DELIM));
                cmd->input_src = REDIRECT_FILE;
            }
            else if (strcmp(arg, REDIR_OUT) == 0) {
                // redirect stdout
                       
                //
                // If the output_dest is something other than REDIRECT_NONE, then
                // this is an improper command.
                //

                // If this is anything other than the LAST cmd in the list,
                // then this is an error.

                cmd->output_file_name = strdup(strtok(NULL, SPACE_DELIM));
                cmd->output_dest = REDIRECT_FILE;
            }
            else {
                // add next param
                param_t *param = (param_t *) calloc(1, sizeof(param_t));
                param_t *cparam = cmd->param_list;

                cmd->param_count++;
                // Put something in here to strip out the single quotes if
                // they are the first/last characters in arg.
                if (arg[0] == '\'') {
                    arg++;
                }
                if (arg[strlen(arg) - 1] == '\'') {
                    arg[strlen(arg) - 1] = '\0';
                }
                param->param = strdup(arg);
                if (NULL == cparam) {
                    cmd->param_list = param;
                }
                else {
                    // I should put a tail pointer on this.
                    while (cparam->next != NULL) {
                        cparam = cparam->next;
                    }
                    cparam->next = param;
                }
            }
        }
        // This could overwite some bogus file redirection.
        if (cmd->list_location > 0) {
            cmd->input_src = REDIRECT_PIPE;
        }
        if (cmd->list_location < (cmd_list->count - 1)) {
            cmd->output_dest = REDIRECT_PIPE;
        }

        // No need free with alloca memory.
        //free(raw);
        cmd = cmd->next;
    }

    if (is_verbose > 0) {
        print_list(cmd_list);
    }
}

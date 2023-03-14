
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <errno.h>

#include "cmd_parse.h"

#define HIST_CMD "history"
#define HIST 10

// I have this a global so that I don't have to pass it to every
// function where I might want to use it. Yes, I know global variables
// are frowned upon, but there are a couple useful uses for them.
// This is one.
unsigned short isVerbose = 0;
char *hist[HIST] = {0};

int 
process_user_input_simple(void)
{
    char str[MAX_STR_LEN];
    char *ret_val;
    char *raw_cmd;
    cmd_list_t *cmd_list = NULL;
    int cmd_count = 0;
    char prompt[30];

    memset(hist, 0, sizeof(hist));
    // Set up a cool user prompt.
    sprintf(prompt, PROMPT_STR " %s β: ", getenv("LOGNAME"));
    for ( ; ; ) {
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
            str[strlen(str) - 1] = '\0';
        }
        if (strlen(str) == 0) {
            // An empty command line.
            // Just jump back to the promt and fgets().
            // Don't start telling me I'm going to get cooties by
            // using continue.
            continue;
        }

        if (strcmp(str, QUIT_CMD) == 0) {
            // Pickup your toys and go home. I just hope there are not
            // any memory leaks. ;-)

            break;
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

    free(hist[HIST-1]);
    for (int i = HIST - 2 ; i >= 0; i--){
        hist[i+1] = hist[i];
    }
    hist[0] = strdup(str);

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
            isVerbose++;
            if (isVerbose) {
                fprintf(stderr, "verbose: verbose option selected: %d\n"
                        , isVerbose);
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

void 
exec_commands( cmd_list_t *cmds ) 
{
    cmd_t *cmd = cmds->head;
    param_t *curr = NULL;

    if (1 == cmds->count) {
        if (!cmd->cmd) {
            // if it is an empty command, bail.
            return;
        }
        free(hist[HIST-1]);
        memmove(&(hist[1]), &(hist[0]), (HIST -1) * sizeof(char*));
        hist[0] = cmd->cmd;
                
        if (0 == strcmp(cmd->cmd, CD_CMD)) {
            if (0 == cmd->param_count) {
                // Just a "cd" on the command line without a target directory
                // need to cd to the HOME directory.

                // Is there an environment variable, somewhere, that contains
                // the HOME directory that could be used as an argument to
                // the chdir() fucntion?
                chdir(getenv("HOME"));
            }
            else {
                // try and cd to the target directory. It would be good to check
                // for errors here.
                if (0 == chdir(cmd->param_list->param)) {
                    // a happy chdir!  ;-)
                }
                else {
                    // a sad chdir.  :-(
                }
            }
        }
        else if (0 == strcmp(cmd->cmd, CWD_CMD)) {
            char str[MAXPATHLEN];

            // Fetch the Current Working Wirectory (CWD).
            // aka - get country western dancing
            getcwd(str, MAXPATHLEN); 
            printf(" " CWD_CMD ": %s\n", str);
        }
        //the history command
        else if (0 == strcmp(cmd->cmd, HIST_CMD)) {
            int x = 0;
            for (int ind = HIST - 1; ind >= 0 ; ind--){
                if (hist[ind] != NULL){
                    printf("%d: %s", ++x, hist[ind]);
                    printf("\n");
                }
            }
            printf("\n");
        }
        else if (0 == strcmp(cmd->cmd, ECHO_CMD)) {
            // insert code here
            // insert code here
            // Is that an echo?
            
            param_t *temp = cmd->param_list;
            if (temp == NULL){
                return;
            }
            while (temp->next){
                printf("%s ", temp->param);
                temp = temp->next;
            }
                printf("%s\n", temp->param);

        }
        else {
            // A single command to create and exec
            // If you really do things correctly, you don't need a special call
            // for a single command, as distinguished from multiple commands.
            
            pid_t pid = fork();

            
            //child process
            if (pid == 0){
                //copy the params list
                //char **params = (char**)malloc(sizeof(char*) * (cmd->param_count + 1));
                char **params = (char**)calloc((cmd->param_count)+2, sizeof(char*));
                params[0] = cmd->cmd;
                curr= cmd->param_list;

                for (int i = 1 ; i <= cmd->param_count ; i++){
                    params[i] = cmd->param_list->param;
                    curr = curr->next;
                }

            
                if (REDIRECT_FILE == cmd->input_src) {
                    int fd;
                    fd = open(cmd->input_file_name, O_RDONLY);
                    if (fd < 0) {
                        fprintf(stderr, "******* redir in failed %d *******\n", errno);
                        exit(7);
                    }

                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }
                if (REDIRECT_FILE == cmd->output_dest) {
                    int fd;
                    fd = open(cmd->output_file_name, O_WRONLY | O_TRUNC | O_CREAT, 0666);
                    if (fd < 0) {
                        fprintf(stderr, "******* redir out in failed %d *******\n", errno);
                        exit(7);
                    }

                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            

                execvp(params[0], params);
                //shouldn't return
                perror("you had a little wuh oh accident in your exec_commands func in cmd_parse.h");
                exit(1);
            }

            //parent process
            else if (pid > 0){
                int status;
                waitpid(pid, &status, 0);

            }

            //something went wrong with fork()
            else {
                perror("failed to fork");
            }
        }
       
    }else {
        // Other things???
        // More than one command on the command line. Who'da thunk it!
        // This really falls into Stage 2.

        //this part will loop while we still have commands
        int ptrail = 0;
        int filedes[2];
        pid_t pid = -1;

        while(cmd){
            char **params = (char**)calloc((cmd->param_count)+2, sizeof(char*));
            //not last
            if (cmd != cmds->tail){
                //make our pipe
                pipe(filedes);
            }

            //fork stuff
            pid = fork();
            
            //child process
            if (pid == 0){
                //check if not first
                if (cmd != cmds->head){
                    dup2(ptrail, STDIN_FILENO);
                }
                //check if not last
                if (cmd != cmds->tail){
                    dup2(filedes[STDOUT_FILENO], STDOUT_FILENO);
                    close(filedes[STDIN_FILENO]);
                    close(filedes[STDOUT_FILENO]);

                }
                //exec with ragged aray
                params[0] = cmd->cmd;
                curr= cmd->param_list;

                for (int i = 1 ; i <= cmd->param_count ; i++){
                    params[i] = cmd->param_list->param;
                    curr = curr->next;
                }


                if (REDIRECT_FILE == cmd->input_src) {
                    int fd;
                    fd = open(cmd->input_file_name, O_RDONLY);
                    if (fd < 0) {
                        fprintf(stderr, "******* redir in failed %d *******\n", errno);
                        exit(7);
                    }

                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }
                if (REDIRECT_FILE == cmd->output_dest) {
                    int fd;
                    fd = open(cmd->output_file_name, O_WRONLY | O_TRUNC | O_CREAT, 0666);
                    if (fd < 0) {
                        fprintf(stderr, "******* redir out in failed %d *******\n", errno);
                        exit(7);
                }

                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

                execvp(params[0], params);
                perror("we did a lil wuh oh in our chained redirect");
                exit(1);
            }

            //parent
            if (pid > 0){
                //check if not first
                if (cmd != cmds->head){
                    close(ptrail);
                }
                //check if not last
                if (cmd != cmds->tail){
                    close(filedes[STDOUT_FILENO]);
                    ptrail = filedes[STDIN_FILENO];
                }

            }
            //something went wrong with fork
            else
                perror("failed to fork");
            cmd = cmd->next;    
        }
        while(wait(NULL) != -1);
    }
}

void
free_list(cmd_list_t *cmd_list)
{
    // Proof left to the student.
    // You thought I was going to do this for you! HA! You get
    // the enjoyment of doing it for yourself.
    cmd_t *cmd = cmd_list->head;
    cmd_t *temp = NULL;

    if (cmd_list == NULL)
        return;

    //traverse to tail
    while (cmd) {
        temp = cmd->next;
        free_cmd(cmd);
        free(cmd);
        cmd = temp;
    }
    //we assume we are in the tail now
    if (cmd_list)
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
    param_t *curr = cmd->param_list;
    param_t *temp = NULL; 
    // Proof left to the student.
    // Yep, on yer own.
    // I beleive in you.
    // free all the easy ones
    if (cmd->raw_cmd)
        free(cmd->raw_cmd);
    if (cmd->cmd)
        free(cmd->cmd);
    if (cmd->input_file_name)
        free(cmd->input_file_name);
    if (cmd->output_file_name)
        free(cmd->output_file_name);

    //iteratively free the param_list
    while (curr){
        temp = curr->next;
        free(curr->param);
        free(curr);
        curr = temp;
    }
        //free(curr);

    //NEED TO FINISH THIS PART
    
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

    if (isVerbose > 0) {
        print_list(cmd_list);
    }
}

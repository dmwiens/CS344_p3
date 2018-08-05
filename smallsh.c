/******************************************************************************
* Author:           David Wiens
* Date Created:     08/2/2018
*
* Desc: This program implements a small shell program for the Linux environment
*
******************************************************************************/


#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>


#define NUM_BG_PROC 100

// Declare background process record
struct BGProcRecord {
    int active;
    pid_t pid;
};

// Declare room information struct
struct Shell  {
    pid_t pid;
    char pidString[256];
    char entWords[512][256];    // 512 "words", each of max length 256 char
    int entWordsCnt;            // actual number of entered words
    //int modeForegroundOnly;
    char statusMessage[256];
    struct BGProcRecord bgproc[NUM_BG_PROC];   // array for holding background process information 
};



// Function Prototypes
void shellSetup(struct Shell*);
void shellLoop(struct Shell*);
void shellTeardown(struct Shell*);
void GetEntryWords(struct Shell* sh);
int WordIsBuiltinCommand(char* word);
void ChangeDirectory(struct Shell* sh);
void ExecWords(struct Shell* sh);
void ChildExecution(struct Shell* sh, int childExecInBackground);
void ParentExecution(struct Shell* sh, int childExecInBackground, pid_t childPid);
void SetStatusMessage(char* status, int exitStatus);
void CheckBGProc(struct Shell* sh);
void catchSIGCHLD(int signo);
void catchSIGTSTP(int signo);



// Global variables
int modeForegroundOnly = 0;     // added here to allow access in SIGTSTP handler
int getlineErrCnt = 0;

int main()
{

    // Define and initialize shell data
    struct Shell sh;
    sh.entWordsCnt = 0;
    //sh.modeForegroundOnly = 0;


    // Set up the shell
    shellSetup(&sh);

    // Loop for the Shell
    shellLoop(&sh);

    // Teardown the shell
    shellTeardown(&sh);

}



/******************************************************************************
Name: shellSetup
Desc: xxxdesc
******************************************************************************/
void shellSetup(struct Shell* sh)
{
    int i;

    // Get current process id
    sh->pid = getpid();
    sprintf(sh->pidString, "%d", sh->pid);


    // Initialize background process records
    for (i = 0; i < NUM_BG_PROC; i++) {
        sh->bgproc[i].active = 0;
        sh->bgproc[i].pid = 0;
    }


    // Set up Parent signal handlers
    // 1. Catch child terminations
    // 2. Protect Shell from terminating via SIGINT signal
    struct sigaction SIGCHLD_action = {0}, SIGTSTP_action = {0}, ignore_action = {0};

    // Set up child termination struct
    SIGCHLD_action.sa_handler = catchSIGCHLD;
    sigfillset(&SIGCHLD_action.sa_mask);
    SIGCHLD_action.sa_flags = SA_RESTART;

    // Set up mode-switch signal struct
    SIGTSTP_action.sa_handler = catchSIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;

    // Set up SIGINT ignore struct
    ignore_action.sa_handler = SIG_IGN;

    // Call sigaction's
    sigaction(SIGCHLD, &SIGCHLD_action, NULL);
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
    sigaction(SIGINT, &ignore_action, NULL);


    return;
}


/******************************************************************************
Name: shellLoop
Desc: This program continually prompts the user for input with a ": " and
        processes it (whether it is an ignorable line, a built-in command, or
        an executable command)
******************************************************************************/
void shellLoop(struct Shell* sh)
{
    int exitCommandIssued = 0;


    while (!exitCommandIssued)
    {
        // Check the Status of all background processes
        CheckBGProc(sh);


        // Test: Pause for 10 seconds
        //printf("Sleeping for 10 seconds\n");
        //sleep(10);


        // Get the users entry (and process into array of whitespace-delimited words)
        GetEntryWords(sh);


        // Process Input
        if ((sh->entWordsCnt > 0 && sh->entWords[0][0] == '#') || sh->entWordsCnt == 0) 
        {
            ; // comment or blank entered. Do nothing (proceed through and restart loop)

            //printf("Skip!\n");

        } else if (WordIsBuiltinCommand(sh->entWords[0]))
        {
            if (strcmp(sh->entWords[0], "exit") == 0)
            {
                //printf("exit entered\n");
                exitCommandIssued = 1;

            } else if (strcmp(sh->entWords[0], "cd") == 0)
            {
                //printf("cd entered\n");
                ChangeDirectory(sh);

            } else if (strcmp(sh->entWords[0], "status") == 0)
            {
                // Print status
                printf("%s\n", sh->statusMessage);
            } 

        } else {

            ExecWords(sh);


        } // end input processing
    }// endwhile

    return;
}



/******************************************************************************
Name: shellTeardown
Desc: This program deconstructs the shell (frees memory, terminates background
        processes, etc.) 
******************************************************************************/
void shellTeardown(struct Shell* sh)
{
    int i, childExitStatus;

    // kill off any active background processes
    for (i = 0; i < NUM_BG_PROC; i++) {
        if (sh->bgproc[i].active){
            //printf("killing bg process %d...\n", sh->bgproc[i].pid);

            // issue kill signals
            kill(sh->bgproc[i].pid, SIGKILL);

            // wait for background process to die
            pid_t actualPid = waitpid(sh->bgproc[i].pid, &childExitStatus, 0);
        }
    }

    return;
}


/******************************************************************************
Name: GetEntryWords
Desc: This program prompts the user for input, collects and parses the entered
        characters into words
******************************************************************************/
void GetEntryWords(struct Shell* sh)
{
    char* entryBuff = NULL;
    int entryBuffCharCnt = 0;
    size_t entryBuffSize = 0;
    int i;


    // Initalize number of words (to 0)
    sh->entWordsCnt = 0;

    // Print prompt (and flush output)
    printf("%s", ": ");
    fflush(stdout);

    // Get user input line
    entryBuffCharCnt = getline(&entryBuff, &entryBuffSize, stdin);
    //printf("Allocated %zu bytes for the %d chars you entered.\n", entryBuffSize, entryBuffCharCnt);
    //printf("Here is the raw entered line: \"%s\"\n", entryBuff);
    //printf("aftergetline: entryBuffCharCnt is %d, size is %d, errno: %d\n", entryBuffCharCnt, entryBuffSize, errno);

    // Process if getline successful
    if (entryBuffCharCnt != -1)
    {

        // Remove trailing newline
        entryBuff[entryBuffCharCnt-1] = '\0';
        entryBuffCharCnt--;

        //printf("Here is the cleaned entered line: \"%s\"\n", entryBuff);

        // Parse the entered characters into an array of words
        char* token = strtok(entryBuff, " \t");
        

        while (token != NULL)
        {
            strcpy(sh->entWords[sh->entWordsCnt], token);
            sh->entWordsCnt++;
            token = strtok(NULL, " \t");
        }


        // Convert any "$$" words to process ID
        for (i = 0; i < sh->entWordsCnt; i++)
        {
            // If any word ends with "$$", replace it with the (stringified) process ID
            char* dsdsLocation = strstr(sh->entWords[i], "$$");
            if (dsdsLocation != NULL)
            {
                sprintf(dsdsLocation, sh->pidString);
            }
        }

        // Test: print out entered words
        //printf("User entered %d words, as follows: \n", sh->entWordsCnt);
        //for (i = 0; i < sh->entWordsCnt; i++)
        //{
        //    printf("Word_%d: %s\n", i, sh->entWords[i]);
        //}

    

    // Free entry buffer
    //printf("beforefree\n");
    free(entryBuff);
    //printf("afterfree\n");
    
    } else {

        getlineErrCnt++;
        if (getlineErrCnt > 5) {exit(1);}
    }

    return;
}



/******************************************************************************
Name: WordIsBuiltinCommand
Desc: This program prompts receives a word and returns th
******************************************************************************/
int WordIsBuiltinCommand(char* word)
{
    return (strcmp(word, "exit") == 0) || (strcmp(word, "cd") == 0) || (strcmp(word, "status") == 0);
}



/******************************************************************************
Name: ChangeDirectory
Desc: This program changes the directory. word 0 is "cd" (already confirmed).
        word 1 (optional) is the file to move to.
******************************************************************************/
void ChangeDirectory(struct Shell* sh)
{

    if (sh->entWordsCnt == 1)   // only "cd" was entered
    {
        // Change cwd to HOME directory
        if (chdir(getenv("HOME")) != 0)
            perror("Failure to change working directory to HOME\n");

    } else // additional argument(s) were added 
    {
        // Change directory, passing in argument
        // NOTE: chdir handles interpretting the argument as relative or absolute
        if (chdir(sh->entWords[1]) != 0)
            perror("Failure to change working directory to entered directory.\n");
    }

}


/******************************************************************************
Name: ExecWords
Desc: This program executes the shells word arguments
******************************************************************************/
void ExecWords(struct Shell* sh)
{

    pid_t spawnPid = -5;
    int childExecInBackground = 0;


    // Look for Background flag ('&' at end of entered words)
    if (strcmp(sh->entWords[sh->entWordsCnt - 1], "&") == 0)
    {
        // Ampersand present
        // If mode allows, set flag to execute in background
        if (!modeForegroundOnly){
            //printf("Process with exec() in background \n");
            childExecInBackground = 1;
        } else {
            //printf("FG only mode. Process with exec() in foreground \n");
            childExecInBackground = 0;
        }

        // Decrement number of words to effectively "remove" the '&' from the end
        // This should occur regardless of mode.
        sh->entWordsCnt--;
    }
    else {
        //printf("Process with exec() in foreground \n");
        childExecInBackground = 0;
    }




    // Fork off child
    spawnPid = fork();

    switch (spawnPid) {
        case -1: { 
            perror("Fork Error!\n"); exit(1); break;
            }
        case 0: {
            
            ChildExecution(sh, childExecInBackground);
            break;
            }
        default: {

            ParentExecution(sh, childExecInBackground, spawnPid);
            break;
            }
    } // end switch

}



/******************************************************************************
Name: ChildExecution
Desc: This program is only called by the child after fork. The word array
at this point does not include a terminating ampersand.
******************************************************************************/
void ChildExecution(struct Shell* sh, int childExecInBackground)
{
    char* args[513];    // 512 + 1 (for NULL)
    int i;
    int inputSpecified = 0;
    int outputSpecified = 0;
    char inputArg[256];
    char outputArg[256];
    int inputFD, outputFD, devNullFD, result;


    // If foreground process, expose to SIGINT by setting default action
    // (If background, child process will ignore (via inheritance from parent) SIGINT)
    if (!childExecInBackground) {
        struct sigaction default_action = {0};
        default_action.sa_handler = SIG_DFL;
        sigaction(SIGINT, &default_action, NULL);
    }

    // Ignore SIGTSTP in child process
    struct sigaction ignore_action = {0};
    ignore_action.sa_handler = SIG_IGN;
    sigaction(SIGTSTP, &ignore_action, NULL);



    // Detect and set up indirection arguments

    // Indirection symbols will only appear in second to last and fourth to last words
    // Two times in a row, evaluate the second to last argument
    for (i = 0; i < 2; i++) {
        // Make sure there are enough words left for redirection to be possible
        if (sh->entWordsCnt >= 3)
        {
            if (strcmp(sh->entWords[sh->entWordsCnt-2], "<") == 0) {// input specified
                inputSpecified = 1;
                strcpy(inputArg, sh->entWords[sh->entWordsCnt-1]);

                // "Remove" the last two arguments
                sh->entWordsCnt = sh->entWordsCnt - 2;
            } else if (strcmp(sh->entWords[sh->entWordsCnt-2], ">") == 0) {// output specified
                outputSpecified = 1;
                strcpy(outputArg, sh->entWords[sh->entWordsCnt-1]);

                // "Remove" the last two arguments
                sh->entWordsCnt = sh->entWordsCnt - 2;           
            }
        }
    }

    // Test: redirection evaluation
    //printf("Input redirection: %d. The argument is \"%s\"\n", inputSpecified, inputArg);
    //printf("Output redirection: %d. The argument is \"%s\"\n", outputSpecified, outputArg);


    // Set up Input redirection
    if (inputSpecified) {
        inputFD = open(inputArg, O_RDONLY);

        if (inputFD != -1) {
            // redirect input
            result = dup2(inputFD, 0);
            if (result == -1) {perror("dup2 failed on input redirection.\n"); exit(1);}

        } else
        {
            perror("Input file open failed.\n");
            exit(1);
        }
    } else {
        // If child should execute in background and input is not specified, take from /dev/null
        if (childExecInBackground) {
            inputFD = open("/dev/null", O_RDONLY);
            dup2(inputFD, 0);
        }
    }

    // Set up Output redirection
    if (outputSpecified) {
        outputFD = open(outputArg, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (outputFD != -1) {
            // redirect input
            result = dup2(outputFD, 1);
            if (result == -1) {perror("dup2 failed on output redirection.\n"); exit(1);}

        } else
        {
            perror("Output file open failed.\n");
            exit(1);
        }
    } else {
        // If child should execute in background and output is not specified, send to /dev/null
        if (childExecInBackground) {
            outputFD = open("/dev/null", O_WRONLY);
            dup2(outputFD, 1);
        }
    }

    // Prepare arguments
    for (i = 0; i < sh->entWordsCnt; i++)
    {
        // Assign the address of the words to the arguments
        args[i] = &sh->entWords[i];
    }
    // Set char pointer after last word to NULL
    args[sh->entWordsCnt] = NULL;


    // Test: Print out arguments (only works if no output redirection)
    //for (i = 0; i <= sh->entWordsCnt; i++)
    //{
    //    printf("Argument %d is \"%s\"\n",i , args[i]);
    //}
    
    // Execute!
    execvp(args[0], args);

    // Should never get here, but just in case :)
    exit(2);
}


/******************************************************************************
Name: ParentExecution
Desc: This program is only called by the parent after fork. It either waits
or doesn't wait, depending on the mode.
******************************************************************************/
void ParentExecution(struct Shell* sh, int childExecInBackground, pid_t childPid)
{
    int childExitStatus = -5;
    int i;

    //printf("PARENT(%d): Sleeping for 1 second\n", getpid());
    //sleep(1);
    //printf("PARENT(%d): Wait()ing for child(%d) to terminate\n", getpid(), childPid);

    if (childExecInBackground) {

        // Print background process id;
        printf("background pid is %d\n", childPid);

        // Record background
        // Find next record with "inactive" status
        for (i = 0; i < NUM_BG_PROC; i++) {
            if (!sh->bgproc[i].active){
                // Set active and record info
                sh->bgproc[i].active = 1;
                sh->bgproc[i].pid = childPid;
                
                // Return once recorded
                return;
            }
        }

    } else {
        // Wait for termination
        pid_t actualPid = waitpid(childPid, &childExitStatus, 0);
        
        //printf("PARENT(%d): Child(%d) terminated.\n", getpid(), actualPid);

        // Set the status message
        SetStatusMessage(sh->statusMessage, childExitStatus);

        return;
    }

}


/******************************************************************************
Name: SetStatusMessage
Desc: This program sets the status message string based on the passed in exitStatus
integer
******************************************************************************/
void SetStatusMessage(char* status, int exitStatus)
{

    // Check whether process exited with return value
    if (WIFEXITED(exitStatus) != 0)
    {
        sprintf(status, "exit value %d", WEXITSTATUS(exitStatus));
    } 
    else if (WIFSIGNALED(exitStatus) != 0)
    {
        sprintf(status, "terminated by signal %d", WTERMSIG(exitStatus));
    }

}


/******************************************************************************
Name: CheckBGProc
Desc: This program checks the status of background programs and prints their
results if necessary.
******************************************************************************/
void CheckBGProc(struct Shell* sh)
{
    pid_t bgProcPid;
    int exitStatus;
    char bgStatusMessage[256];
    int i;

    // Find next record with "active" status
    for (i = 0; i < NUM_BG_PROC; i++) {
        if (sh->bgproc[i].active){
            // background process is active

            // Check on completion
            bgProcPid = waitpid(sh->bgproc[i].pid, &exitStatus, WNOHANG);
            if (bgProcPid != 0) 
            {
                // Process completed! Set Status message and print
                SetStatusMessage(bgStatusMessage, exitStatus);
                printf("background pid %d is done: %s\n", sh->bgproc[i].pid, bgStatusMessage);

                // Set record to inactive
                sh->bgproc[i].active = 0;
                sh->bgproc[i].pid = 0;
            }
        }
    }
}


/******************************************************************************
Name: catchSIGCHLD
Desc: If a child process is terminated (by SIGINT), the SIGCHLD signal triggers
        this to be called in the parent/shell process.
******************************************************************************/
void catchSIGCHLD(int signo)
{
    char* message = "terminated by signal 2\n";
    write(STDOUT_FILENO, message, 23);
}


/******************************************************************************
Name: catchSIGTSTP
Desc: If the shell is sent SIGTSTP, the execution mode inverts. This function
        catches that signal.
******************************************************************************/
void catchSIGTSTP(int signo)
{

    if (modeForegroundOnly) {
        modeForegroundOnly = 0;

        // Print message
        char* message = "Exiting foreground-only mode\n: ";
        write(STDOUT_FILENO, message, 32);
    } else {
        modeForegroundOnly = 1;
        
        // Print message
        char* message = "Entering foreground-only mode (& is now ignored)\n: ";
        write(STDOUT_FILENO, message, 51);
    }

}
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


// Declare room information struct
struct Shell  {
    pid_t pid;
    char pidString[256];
    char entWords[512][256];    // 512 "words", each of max length 256 char
    int entWordsCnt;            // actual number of entered words
    int modeForegroundOnly;
};



// Function Prototypes
void shellSetup(struct Shell*);
void shellLoop(struct Shell*);
void shellTeardown();
void GetEntryWords(struct Shell* sh);
int WordIsBuiltinCommand(char* word);
void ChangeDirectory(struct Shell* sh);
void ExecWords(struct Shell* sh);
void ChildExecution(struct Shell* sh, int childExecInBackground);
void ParentExecution(struct Shell* sh, int childExecInBackground, pid_t childPid);

int main()
{

    // Define and initialize shell data
    struct Shell sh;
    sh.entWordsCnt = 0;
    sh.modeForegroundOnly = 0;


    // Set up the shell
    shellSetup(&sh);

    // Loop for the Shell
    shellLoop(&sh);

    // Teardown the shell
    shellTeardown();

}



/******************************************************************************
Name: shellSetup
Desc: xxxdesc
******************************************************************************/
void shellSetup(struct Shell* sh)
{
    // Get current process id
    sh->pid = getpid();
    sprintf(sh->pidString, "%d", sh->pid);

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
        // Get the users entry (and process into array of whitespace-delimited words)
        GetEntryWords(sh);


        // Process Input
        if ((sh->entWordsCnt > 0 && sh->entWords[0][0] == '#') || sh->entWordsCnt == 0) 
        {
            printf("Skip!\n");

        } else if (WordIsBuiltinCommand(sh->entWords[0]))
        {
            if (strcmp(sh->entWords[0], "exit") == 0)
            {
                printf("exit entered\n");
                exitCommandIssued = 1;

            } else if (strcmp(sh->entWords[0], "cd") == 0)
            {
                printf("cd entered\n");
                ChangeDirectory(sh);

            } else if (strcmp(sh->entWords[0], "status") == 0)
            {
                printf("status entered\n");
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
void shellTeardown()
{

    return;
}


/******************************************************************************
Name: GetEntryWords
Desc: This program prompts the user for input, collects and parses the entered
        characters into words
******************************************************************************/
void GetEntryWords(struct Shell* sh)
{
    char* entryBuff;
    int entryBuffCharCnt = 0;
    size_t entryBuffSize = 0;
    int i;

    // Print prompt (and flush output)
    printf("%s", ": ");
    fflush(stdout);


    // Get user input line
    entryBuffCharCnt = getline(&entryBuff, &entryBuffSize, stdin);
    //printf("Allocated %zu bytes for the %d chars you entered.\n", entryBuffSize, entryBuffCharCnt);
    //printf("Here is the raw entered line: \"%s\"\n", entryBuff);
    
    // Remove trailing newline
    entryBuff[entryBuffCharCnt-1] = '\0';
    entryBuffCharCnt--;

    //printf("Here is the cleaned entered line: \"%s\"\n", entryBuff);

    // Parse the entered characters into an array of words
    char* token = strtok(entryBuff, " \t");
    sh->entWordsCnt = 0;

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
    // {
    //    printf("Word_%d: %s\n", i, sh->entWords[i]);
    // }

    // Free entry buffer
    free(entryBuff);

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
        if (!sh->modeForegroundOnly){
            printf("Process with exec() in background \n");
            childExecInBackground = 1;
        } else {
            printf("FG only mode. Process with exec() in foreground \n");
            childExecInBackground = 0;
        }

        // Decrement number of words to effectively "remove" the '&' from the end
        // This should occur regardless of mode.
        sh->entWordsCnt--;
    }
    else {
        printf("Process with exec() in foreground \n");
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




void ChildExecution(struct Shell* sh, int childExecInBackground)
{
    char* args[513];    // 512 + 1 (for NULL)
    int i;

    // Prepare arguments
    for (i = 0; i < sh->entWordsCnt; i++)
    {
        // Assign the address of the words to the arguments
        args[i] = &sh->entWords[i];
    }
    // Set char pointer after last word to NULL
    args[sh->entWordsCnt] = NULL;


    // Test: Print out arguments
    //for (i = 0; i <= sh->entWordsCnt; i++)
    //{
    //    printf("Argument %d is \"%s\"\n",i , args[i]);
    //}
    
    // Execute!
    execvp(args[0], args);

    // Should never get here, but just in case :)
    exit(2);

}



void ParentExecution(struct Shell* sh, int childExecInBackground, pid_t childPid)
{
    int childExitStatus = -5;

    printf("PARENT(%d): Sleeping for 2 seconds\n", getpid());
    sleep(2);
    printf("PARENT(%d): Wait()ing for child(%d) to terminate\n", getpid(), childPid);
    pid_t actualPid = waitpid(childPid, &childExitStatus, 0);
    printf("PARENT(%d): Child(%d) terminated.\n", getpid(), actualPid);
}
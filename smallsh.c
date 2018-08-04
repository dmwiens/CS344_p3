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
    printf("Process ID is: %s\n", sh->pidString);

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

            } else if (strcmp(sh->entWords[0], "status") == 0)
            {
                printf("status entered\n");
            } 

        } else {

            // look for background flag
            if (strcmp(sh->entWords[sh->entWordsCnt - 1], "&") == 0)
            {
                printf("Process with exec() in background \n");
            }
            else {
                printf("Process with exec() in foreground \n");
            }


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


    // get user input
    entryBuffCharCnt = getline(&entryBuff, &entryBuffSize, stdin);
    printf("Allocated %zu bytes for the %d chars you entered.\n", entryBuffSize, entryBuffCharCnt);
    printf("Here is the raw entered line: \"%s\"\n", entryBuff);
    
    // remove trailing newline
    entryBuff[entryBuffCharCnt-1] = '\0';
    entryBuffCharCnt--;

    printf("Here is the cleaned entered line: \"%s\"\n", entryBuff);

    // Parse the entered characters into an array of words
    char* token = strtok(entryBuff, " \t");
    sh->entWordsCnt = 0;

    while (token != NULL)
    {
        strcpy(sh->entWords[sh->entWordsCnt], token);
        sh->entWordsCnt++;
        token = strtok(NULL, " \t");
    }


    // Convert any $$ arguments to process ID
    for (i = 0; i < sh->entWordsCnt; i++)
    {
        if (strcmp(sh->entWords[i], "$$") == 0) {
            sprintf(sh->entWords[i], sh->pidString);
        }
    }

    // Test: print out entered words
    printf("User entered %d words, as follows: \n", sh->entWordsCnt);
    for (i = 0; i < sh->entWordsCnt; i++)
    {
        printf("Word_%d: %s\n", i, sh->entWords[i]);
    }

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
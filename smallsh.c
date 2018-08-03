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
    char* entryBuff;
    int entryBuffCharCnt;
    size_t entryBuffSize;
    char entWords[512][256];    // 512 "words", each of max length 256 char
    int entWordsCnt;            // actual number of entered words
    int modeForegroundOnly;
};



// Function Prototypes
void shellSetup();
void shellLoop(struct Shell*);
void shellTeardown();
void GetEntryWords(struct Shell* sh);


int main()
{

    // Define and initialize shell data
    struct Shell sh;
    sh.entryBuffCharCnt = 0;
    sh.entWordsCnt = 0;
    sh.modeForegroundOnly = 0;


    // Set up the shell
    shellSetup();

    // Loop for the Shell
    shellLoop(&sh);

    // Teardown the shell
    shellTeardown();

}



/******************************************************************************
Name: shellSetup
Desc: xxxdesc
******************************************************************************/
void shellSetup()
{

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

        // Print prompt (and flush output)
        printf("%s", ": ");
        fflush(stdout);

        // get user input
        sh->entryBuffCharCnt = getline(&sh->entryBuff, &sh->entryBuffSize, stdin);
        printf("Allocated %zu bytes for the %d chars you entered.\n", sh->entryBuffSize, sh->entryBuffCharCnt);
        printf("Here is the raw entered line: \"%s\"\n", sh->entryBuff);
        
        // remove trailing newline
        sh->entryBuff[sh->entryBuffCharCnt-1] = '\0';
        sh->entryBuffCharCnt--;

        printf("Here is the cleaned entered line: \"%s\"\n", sh->entryBuff);

        // Parse the entered characters into an array of words
        char* token = strtok(sh->entryBuff, " ");
        sh->entWordsCnt = 0;

        while (token != NULL)
        {
            strcpy(sh->entWords[sh->entWordsCnt], token);
            sh->entWordsCnt++;
            token = strtok(NULL, " ");
        }




        // Test: print out entered words
        printf("User entered %d words, as follows: \n", sh->entWordsCnt);
        int i;
        for (i = 0; i < sh->entWordsCnt; i++)
        {
            printf("Word_%d: %s\n", i, sh->entWords[i]);
        }




        free(sh->entryBuff);
    //while (!exitCommandIssued)
    //{}
    


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
    ;

    return;
}
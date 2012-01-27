/*
 CS575 Fall 2011 Project1 
 Ben Anderson
 9/23/2011
 BAsh: the (B)en (A)nderson (sh)ell
*/

//Header files needed 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

//Define constants
#define MAXLINE 1024
#define MAXARGS 20
#define PROMPT "myshell> "
#define HISTSIZE 10 //history size


// this will hold the last N commands for historical purposes
char history[HISTSIZE][MAXLINE];

// allow background jobs? no by default
int allow_bj = 0;
// this holds the output file name when redirection is used
char *output_file;
// to support multiple commands, keep track of the next one
char next_command[MAXLINE];
// this flag indicates if we're done with the program
int done = 0;

// helper functions used below
void addHistory(char *command);
void printHistory();
void clearCommand(char *cmdLine);
int getCommand(char* cmdLine);

int main(int argc, char *argv[])
{
  char cmd[MAXLINE] = "";
  char *args[MAXARGS];

  while(done == 0) {

    // check if there's a previously stored command to use (from using ; separator)
    if (next_command == NULL) {
      // no stored commands, start with prompt: "myshell >"
      printf(PROMPT);
      // accept the command and execute (implemented below)
      getCommand(cmd);
    } else {
      // use previous command stored in next_command
      strcpy(cmd,next_command);
    }
    
    // parse the command(s) into an array of arguments
    parsecmd(cmd, args);
    //printf("You ran: %s\n", cmd);

    // handle custom shell commands first;
    if (strncmp(cmd, "history", 7) == 0) {
      printHistory(); //implemented below
    } else if (strncmp(cmd, "exit", 4) == 0) {
      done = 1; //ready to terminate
    } else if (strncmp(cmd, "cd ", 3) == 0) {
      //implement CD system call
      if (chdir(args[1]) == 0) {
       // printf("You are now in %s\n", args[1]);
        printf("You are now in %s\n", getwd(cmd)); 
      } else {
        printf("Invalid directory, try again.\n");
      }
    } else {
      // if it's not a custom shell command, fork and process command as is

      // fork a child (example structure from wikipedia: http://en.wikipedia.org/wiki/Fork_(operating_system)
      pid_t pid;
      pid = fork();
      if (pid == -1) {
        // an error ocurred during fork...
        fprintf(stderr, "can't fork, error %d\n", errno);
        exit(EXIT_FAILURE);
      } else if (pid == 0) {
	// this is the child process

	// redirect output to a file
	if (output_file != NULL) {
	  int fd = open(output_file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
          dup2(fd, 1);   // make stdout go to file
          close(fd);
	} else {
	  // set to null so next cmd is back to stdout
	  output_file = NULL;
	}

        // execute the command in child process
        if (execvp(args[0], args) != 0) {
	  // if execvp returns non-zero, there was a problem
	  fprintf(stdout, "Command not found, try again.\n");
	}

        // couldn't get the output to stdout unless I use exit below...
	exit(0);

      } else {
        // main process (parent)
	
        //wait for cmd to finish before proceeding UNLESS...
        // ...the global flag was switch (by providing & option)
        if (allow_bj == 0) {
          wait(NULL);
        } else {
          //always set background job back to 0 for next command
          allow_bj = 0;
        }
      }
    }
  }
 
  return 0;
}

/* 
 * parsecmd - Parse the command line and build the argv array.
 * Return the number of arguments.
 *
 * This function only support single command. You need to modify it 
 * to support multiple commands, background job & symbol, and 
 * input/output redirect symbol <, > | 
 
 */
int parsecmd(const char *cmdline, char *arglist[]) 
{
  static char array[MAXLINE]; /* holds local copy of command line */
  char *buf = array;          /* ptr that traverses command line */
  char *delim;                /* points to first space delimiter */
  int argc;                   /* number of args */
  int bg;                     /* background job? */
  
  
  // check for file redirection and capture file name
  if (output_file = strchr(cmdline, '>')) {
    // set the '>' to '\0' so the executable command ends there
    *output_file = '\0';
    output_file++;

    // ignore spaces before file name
    while (*output_file && *output_file == ' ')
      ++output_file;

    // remove the trailing newline from the filename
    if (output_file[strlen(output_file)-1] == '\n')
      output_file[strlen(output_file)-1] = '\0';
  }

  // check for the background_job flag
  if ((delim = strchr(cmdline, '&'))) {
    *delim = '\0'; // don't include in actual command
    allow_bj = 1;  // set global flag to use background
  }

  // implement multi-command separator (;)
  if ((delim = strchr(cmdline, ';'))) {
    *delim = '\0'; 			// terminate prev command at ;
    strcpy(next_command, ++delim);	// copy remaining cmd to be executed next
  } else {
    next_command = NULL;		// no ;, then get next command from prompt
  }
  
  
  strcpy(buf, cmdline);
  // BA: change to check if last char is newline first, so it doesn't break ; parsing
  if (buf[strlen(buf)-1] == '\n')
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
  else
    strcat(buf, " ");

  while (*buf && (*buf == ' ')) /* ignore leading spaces */
    buf++;

  /* Build the arglist list */
  argc = 0;
  // based on delimiter " "(space), separate the commandline into arglist
  while ((delim = strchr(buf, ' ')) && (argc < MAXARGS - 1)) {
    arglist[argc++] = buf;
    *delim = '\0';
    buf = delim + 1;
    while (*buf && (*buf == ' ')) /* ignore spaces */
      buf++;
  }
  arglist[argc] = NULL;
  return argc;  
}


/* clearCommand: resets command string between calls
 * 
 * This was needed because strcopy was leaving the end
 * of the previous command if the next one was shorter
 */
void clearCommand(char *cmdLine) {
  int i = 0;
  while (cmdLine[i] != '\0') {
    cmdLine[i++] = '\0';
  }
}


/* getCommand: a simple function to take input
 * from the user and prep it for parsing
 */
int getCommand(char* cmdLine) {
  int c, n = 0;
  // if we don't clear the command, and the next one is shorter, they will be mixed
  clearCommand(cmdLine);
  while ((c = getchar()) != '\n') {
    cmdLine[n++] = c;
  }

  // parseCmd assumes this ends in a newline, so add tack it on
  cmdLine[n] = '\n';
  
  addHistory(cmdLine);

  return 0;
}


/* addHistory: this records each typed command and cycles out
 * the older ones as the list grows...
 */
void addHistory(char *command) {
  int i = 0;

  // circular array: start from the end and bump all prev entries
  //  down one... Example: 9=8, 8=7, 7=6...1=0 END
  for (i = HISTSIZE-1; i > 0; i--) {
    strcpy(history[i],history[i-1]);
  }
  
  // add the new command to the top of the history log (entry 0)
  strcpy(history[0], command);
}


/* printHistory: prints the list of N previous commands in an 
 * easy-to-read format 
 */
void printHistory() {
  printf("\nYour last %d commands:\n======================\n", HISTSIZE);

  int x;
  // run loop backwards to show history in chronological order
  for (x = HISTSIZE; x > 0; x--) {
    // don't show blank entries
    if (*history[x-1] == '\0') continue;
    // print command history items
    printf("%2d. %s", x, history[x-1]);
  }
}
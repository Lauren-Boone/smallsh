
/*****************************************************************
LAuren Boone
Program: smallsh
Description: This prgram immulates a shell similar to bash. It is
written in C. The shell runs command line instructions and returns
results ismilar to bash shells. 
*********************************************************************/

#include <dirent.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#define ON 1
#define OFF 0
#define MAXCHARS 2048

//globals used for managing bg processes
int backgroundFlag = 1;
int bg_processes[100];
int bg_num;

/**************************************************
Function: getInput()
REcieves:
Returns: None
DEscription: This function is used to get the user 
input, clear the buffers and sscanf to input. 
****************************************************/
int parseInput(char* input[], char *inFile, char* outFile, int *backGround, int *redir_in, int *redir_out) {
	
	int i;
	char *j;//used to remove end line character
	fflush(stdout);
	//fflush(stdin);
	printf(": ");
	int charsEntered = 0;
	char *getWords = NULL;
	size_t buffersize = MAXCHARS;
	charsEntered = getline(&getWords, &buffersize, stdin);
	if (charsEntered == -1) {//check if getLine encountered an error
		clearerr(stdin);//clearerror
	}
	int numArgs=0;//number of arguments returned (used in execvp)
	if (strcmp(getWords, "\n") == 0) {//if no input just return
		input[numArgs]="\n";//set value to \n 
		return 1;//(this is the number of arguments)
	}
	//if (strcmp(getWords[0], '#') == 0) {
		//input[numArgs] = "#";
	//	return;
	//}
	
	//Find the newline char and replace it with \0
	j = strchr(getWords, '\n');
	*j = '\0';
	
	
	//use space as a delimeter
	char space[] = " ";
														
	char* token = strtok(getWords, " ");//use strtok 
	char temp[400] = "";
	while(token!=NULL) {
		
		if (strcmp(token, "<")==0) {
			//this is an input file
			token = strtok(NULL, space);//get file name
			strcpy(inFile, token);//add name to inFile
			*redir_in = ON;//set to true there is an input file
		}
		else if (strcmp(token, ">")==0) {
			//this is an output file
			token = strtok(NULL, space);//get file name
			strcpy(outFile, token);
			*redir_out = ON;//set to true, there is an output file
		}
		//if the command is to be run in the background
		else if (strcmp(token, "&")==0) {
			*backGround = 1;
			//token = strtok(NULL, space);
			//strcpy(temp, token);
		}
		else {
			//strcpy(input[numArgs], token);
			input[numArgs] = strdup(token);
			//input[numArgs] = token;
			numArgs++;
			
		}

		//get next token
		token = strtok(NULL, space);
		//check that & was the last input otherwise it is an argument
		if (*backGround == 1 && token != NULL && backgroundFlag==OFF) {
			input[numArgs] = "&";//add to args if not end of input
			numArgs++;//increase number of args
			*backGround = 0;//reset flag
		}
		
	}
	free(getWords);
	getWords = NULL;
	return numArgs;
	
}
/***********************************
Function: killBG()
Description: THis is called when exit
is called. This kills all bg processes
before ending shell program
***************************************/
void killBG() {
	int i;
	//cycle through all bg processes and kill them (before exit)
	for ( i = 0; i < bg_num; ++i) {
		//printf("killing: %d\n", bg_processes[i]);
		kill(bg_processes[i], SIGTERM);//Terminate all backgrounds
	}
}

/*****************************************
Function: addBG
Description: This function adds a pid to 
the integer the end of the integer array
****************************************/
void addBG(int pid) {
	bg_processes[bg_num] = pid;
	bg_num++;
	//printf("added bg %d\n", bg_processes[(bg_num - 1)]);
	return;
}

/**********************************************
Function: removeBG
Description this function searches for the matching
pid and removes it. Then the following pid's are moved
up
******************************************************/
void removeBG(int pid) {
	int i, j;
	for ( i = 0; i < bg_num; ++i) {//cycle through till match
		if (bg_processes[i] == pid) {//if match move following pid's up
			for (j = i; j < bg_num; ++j) {
				//Set the folliwing bg pid to the previous spot
				bg_processes[j] = bg_processes[(j + 1)];
			}
			bg_num--;//one less bg process
			break;
		}
	}
}

/***************************************************************
Function: checkBackgroundProccess()
Recieves: none
Returns none
Description: Checks to see if any backgournd processes are still
running
*****************************************************************/
void checkBackgroundProccess(){
	pid_t checkPid = -1;
	int bg_exitStat;
	
	//check for all background proccesses this is similar to example in lecture
	while ((checkPid = waitpid(-1, &bg_exitStat, WNOHANG)) > 0) {
		if (WIFEXITED(bg_exitStat)) {//if exited normally
			WEXITSTATUS(bg_exitStat);
			printf("background pid %d is done: exit value %d\n", checkPid, bg_exitStat);
			fflush(stdout);
			}
		else if(WIFSIGNALED(bg_exitStat)){//if there was an issue
			WTERMSIG(bg_exitStat);
			printf("background pid %d is done: terminated by signal %d\n", checkPid, bg_exitStat);
			fflush(stdout);
		}
		
	}
	removeBG(checkPid);

}

/***********************************
function: catchSIGTSTP
Description: If caught switches flags
on and off. This code is
from the example in the signal lecture
*************************************/
void catchSIGTSTP(int sig) {
	//if flag is on then turn it off
	if (backgroundFlag == ON) {
		//message to display
		char *message= "\nEntering foreground-only mode (& is now ignored)\n: ";
		write(STDOUT_FILENO, message, 53);//cannot use printf
		backgroundFlag = OFF;//Turn off flagg
	}

	//If flag is off then turn if on
	else if (backgroundFlag == OFF) {
		char *message = "\nExiting foreground-only mode\n: ";
		write(STDOUT_FILENO, message, 33);//cannot use printf
		backgroundFlag = ON;//turn on flag
	}
	fflush(stdout);
	
	//clearerr(stdin);
	//fflush(stdin);
	return;
}


/**************************************************
Function: Main()
This function gets the input from the user and directs 
the input. The user can input CD, EXIT, STATUS, blanks
or comments.
*****************************************************/
int main() {

	//set up our variables
	char* input[512]; //we need an array of strings to hold all the possible arguments;
	int parentID = getpid();
	char inFile[300];//will hold input file names
	char outFile[300];//will hold output file names
	int numArgs = 0;//important for replace $$ with pid
	int bg_exitStat = 0;//holds bg exit status
	int exitStat = 0;//holds fg exit status
	int exitFlag = 1;//used to end looop when user types exit
	bg_num = 0;//global initialization
	int i, j;
	
	//Ignore ctrl C

	//This is set up similar to the exampel in lecture
	//Create three structs. One to ignore, one to tstp and sigint
	struct sigaction sa_sigint = { 0 }, sa_ignore_action = { 0 }, sa_sigtstp = { 0 };
	
	


	//Controlling SIGTSTP background process
	sa_sigtstp.sa_handler = catchSIGTSTP; //similar to example in lecutre
	sigfillset(&sa_sigtstp.sa_mask);
	sa_sigtstp.sa_flags = SA_RESTART; //allows program to continue

	//this now calls the catchSIGTSTP function from lecture
	sigaction(SIGTSTP, &sa_sigtstp, NULL);
	//signal(SIGTSTP, &catchSIGTSTP);

	


	while (exitFlag) {

		//flush
		fflush(stdout);
		fflush(stdin);

		//set sig_ign to ignore
		sa_sigint.sa_handler = SIG_IGN; //set ignore to sigign
		//sigfillset(&sa_sigint.sa_mask);
		sa_sigint.sa_flags = 0;
		//ctrl-C is now ignored for bg and allowed for fg
		sigaction(SIGINT, &sa_sigint, NULL);//ignore sigINT 




		//look up background proccess and check if done
		checkBackgroundProccess();


		
		
		
		//variables to determine if redirection
		int redir_in = 0, redir_out = 0;
		int background = 0;//if user wants to run in background

		//clear before getting new input
		memset(inFile, '\0', MAXCHARS);
		memset(outFile, '\0', MAXCHARS);
		memset(input, '\0', MAXCHARS);
			//get input 
			numArgs = parseInput(input, inFile, outFile, &background, &redir_in, &redir_out);
			if (numArgs == -1) {
				clearerr(stdin);
			}
			//This was set up to replace the $$ with the pid for the test script
			i = (numArgs - 1);
			//replace $$ with pid 
				for (j = 0; input[i][j]; ++j) {
					if ((input[i][j] == '$')) {
						if (input[i][(j + 1)] == '$') {//check that input[i] and j+1 are botth $$
							input[i][j] = '\0';//replace $$ with \0
							//add pid to end of string
								sprintf(input[i], "%s%d", input[i], parentID );
								
								break;
						}
					}
				}
				
			
			
			if (strcmp(input[0], "cd") == 0) {
				//go to a different directory
				if (numArgs == 2) { //if the user defines a directory
					chdir(input[1]);
				}
				else {//otherwise go home
					chdir(getenv("HOME"));
				
				}
				//printf("\n");
			}
			//exit the program
			else if (strcmp(input[0], "exit") == 0) {
				killBG();//function to kill all bg before exiting (no zombies)
				exitFlag = 0;
				exit(0);
				break;
			}
			//print the status
			else if (strcmp(input[0], "status") == 0) {
				printf("exit value %d\n", exitStat);
			}
			else if (input[0][0]== '#' || input[0]== "\n") {
				//DO NOTHING go to next line
			}
			else if(input !=NULL){
				
				//fflush(stdout);
			//	fflush(stdin);
				pid_t childPid = -4; //set to known variable 
				childPid = fork();
				//printf("Childpid: %d\n", childPid);
				int fd_in, fd_out, result;//used for opening and redirecting files

				//using switch statement to have the child do the work
				switch (childPid) {


				case -4:
					
					exit(1);
					break;



				case 0://forked sucessfully

					
					

					//Check if use wanted to be run in the background
					if (background == 1 ) { //if background is 1(run in background)
						if (!redir_in) { //if false(0)  input sent to dev null
							strcpy(inFile, "/dev/null");
						}
						if (!redir_out) {//if false(0) output sent to dev null
							strcpy(outFile, "/dev/null");
						}
					}

					//check if there is an input file
					if (redir_in) {
						//try to open the file and check that it opened correctly
						fd_in = open(inFile, O_RDONLY);
						if (fd_in == -1) {//print an error if file did not open properly
							perror("open()");
							exit(1);
						}
						//use dup2 to redirect input
						result = dup2(fd_in, 0);//assign it to stdin
						if (result == -1) {//check that the redirection was succesful
							perror("dup2()");
							exit(2);
						}
						
					}

					//check if there is an output file
					if (redir_out) {
						//try and open file 
						fd_out = open(outFile, O_WRONLY | O_TRUNC | O_CREAT, 0644);
						if (fd_out == -1) {//check if file was opened 
							perror("dup2");
							exit(1);
						}
						//if file was opened redirect output
						result = dup2(fd_out, 1);//assign it to stdout
						if (result == -1) {//check for errors
							printf("dup2", outFile);
							exit(2);
						}
					}
					


					//If sigInt is called while NOT in bg mode end the process
					if (background == OFF) {
						sa_sigint.sa_handler = SIG_DFL;
						sa_sigint.sa_flags = 0;
						sigaction(SIGINT, &sa_sigint, NULL);
						
					}

					//execute and assign exit status
					exitStat = execvp(input[0], input);
					
					//printf("\n");
					//if the exec did not exit properly print the error
					if (exitStat != 0) {
						perror(input[0]);
						exitStat = 1;
					}


					
					//otherwise exit 0
					exit(0);
					break;
				case -1://similar to lecture example
					perror("Did not fork\n");
					break;

				default:
					//check if user wanted a background and if background is allowed
					if (background == 1 && backgroundFlag == ON) {//if it's a background then do not wait(NOWHANG)
						pid_t flagReturn = waitpid(childPid, &bg_exitStat, WNOHANG);//DO NOT WAIT
						printf("background pid is %d\n", childPid);
						addBG(childPid);//add pid to array of pid's
						fflush(stdout);
					}
					//otherwise it's foreground
					else {//foreground processes wait until finished
						childPid = waitpid(childPid, &exitStat, 0);//wait
						//check the exit status this is very similar to example from lecture
						if (childPid != 0 && childPid != -1) {
							if (WIFSIGNALED(exitStat) != 0) {
								exitStat = WTERMSIG(exitStat);//set the exitStat (used in status)
								printf("Terminated by signal %d\n", exitStat);
							}
							else if (WIFEXITED(exitStat)) {
								exitStat = WEXITSTATUS(exitStat);
							}
						
							fflush(stdout);
						}
						//printf("pid: %d\n", exitStat); 
						
					}
				}
				
			}
			//check if any bg proccess are done
			checkBackgroundProccess();
			
		}
	
	return 0;
}
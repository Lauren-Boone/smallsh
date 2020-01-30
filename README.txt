
Program smallsh

Run the following commands in a bash terminal:

How to Compile:

	 make all
or
 
	 gcc -o smallsh -g smallsh.c

How to Run Program:
./smallsh
or
 p3testscript > mytestresults 2>&1 


To clean:
  make clean


This program immulates a shell. Commands include: chdir, ls, cd, pwd, and many exec commands. 
To run a process in the background use & after the command. 
SIGINT terminates any foreground processes and SIGSTSP disables all future background proceses until next SIGSTSP



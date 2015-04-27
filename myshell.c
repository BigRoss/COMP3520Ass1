/*Shell source code - Alexander Ling 430391570
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <sys/wait.h>   
#include <sys/types.h>
#include <dirent.h>

#define MAX_BUFFER 1024

extern char **environ;

void IO_command(char* args[], int argCounter, char* cwd);
int changeFileStream(char* args[], int argCounter, char* cwd);
void echo_command(char* args[], int argCounter, char* cwd, int IO);
void dir_command(char* args[], int argCounter, char* cwd, int IO);
void environ_command(char* args[], int argCounter, char* cwd, int IO);
void help_command(char* args[], int argCounter, char* cwd, int IO);
void cd_command(char* args[], char* cwd);
void otherProgram_command(char* args[]);
void pause_command();
void printError();
char* appendFileName(char* fileName, char* cwd);
char* mystrdup(char* str);

char originalCWD[MAX_BUFFER];
char originalCWDCrop[MAX_BUFFER];
int waitBool;

/* Note: These 2 functions and struct and everything in them are based on the program found at 
http://www.gnu.org/software/libc/manual/html_node/Noncanon-Example.html.
This program is licensed under the GNU Free Documentation License. */
void reset_input_mode(void);
void set_input_mode(void);
struct termios saved_attributes;

int main(int argc, char* argv[]){

	//Have a file pointer pointing to stdin by default, change this if we're reading from a batch file
	FILE *fp = stdin;
	if(argv[1] != NULL){
		//If there is a 2nd argument then use this argument as the batchfile, open this file and use it in the fgets later on
		fp = fopen(argv[1], "r");
	}

	char input[MAX_BUFFER];
	//Global strings used to hold the path of where the shell is executed from:
	//originalCWD includes appends the "/myshell", originalCWDCrop crops the "/myshell" off
	getcwd(originalCWD, MAX_BUFFER);
	getcwd(originalCWDCrop, MAX_BUFFER);
	strcat(originalCWD, "/myshell");
	setenv("SHELL", originalCWD, 1);

	while(1){
		char cwd[MAX_BUFFER];
		getcwd(cwd, MAX_BUFFER);

		//Only print the prompt if we're recieving input from stdin.
		if(fp == stdin){
			printf("%s$ ", cwd);
		}

		//Get input from the file, either batchfile or stdin
		if(fgets(input, MAX_BUFFER, fp) != NULL){
			char *line = mystrdup(input);
			char *token = strtok(line, " \t\n");
			if(token == NULL){
				continue;
			}
			//Create an array to hold all the arguments, also parse all tabs and new line characters to make other functions easier
			char* args[100];
			//Make the first argument the first token again since this will be useful for the exec functions
			args[0] = token;
			int argCounter = 1;
			char* arg = strtok(NULL, " \t\n");
			//Set the wait boolean to 0 if there is no '&' at the end of the arguments. If there is then change waitBool so that the functions know not to wait for the child
			waitBool = 0;
			while(arg != NULL){
				args[argCounter++] = arg;
				arg = strtok(NULL, " \t\n");	
			}

			//Check if we must run in the background by checking for the & symbol
			if(strcmp(args[argCounter - 1], "&") == 0){
				waitBool = 1;
				argCounter--;
			}
			//Ensure the last argument is NULL for using exec later on
			args[argCounter] = NULL;
			
			//Check if there is any IO redirection in the command. This is used for redirection for the internal echo, dir, help and environ commands
			int IOCheck = 0;
			for(int i = 0; i < argCounter; i++){
				if(strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0){
					IOCheck = 1;
				}
			}

			if(strcmp(token, "environ") == 0){
				environ_command(args, argCounter, cwd, IOCheck);
			}
			else if(strcmp(token, "echo") == 0){
				echo_command(args, argCounter, cwd, IOCheck);
			}
			else if(strcmp(token, "dir") == 0){
				dir_command(args, argCounter, cwd, IOCheck);
			}
			else if(strcmp(token, "help") == 0){
				help_command(args, argCounter, cwd, IOCheck);
			}
			else if(strcmp(token, "cd") == 0){
				cd_command(args, cwd);
			}
			else if(strcmp(token, "pause") == 0){
				pause_command();
			}
			else if(strcmp(token, "clr") == 0){
				system("clear");
			}
			else if(strcmp(token, "quit") == 0){
				//Ensure to free the line malloced with strdup and to close the file pointer
				free(line);
				fclose(fp);
				exit(0);
			}
			else{
				//If there is no input/output redirection then simply run the program using fork/exec
				if(IOCheck){
					IO_command(args, argCounter, cwd);
				}
				else{
					//Run the program using the arguments given
					otherProgram_command(args);
				}
			}
			free(line);
		}
		else{
			//For the batch file we check for the end of file, or errors while reading and also close the file pointer.
			if(feof(fp)){
				fclose(fp);
				return 0;
			}
			else if(ferror(fp)){
				fclose(fp);
				return -1;
			}
		}
	}
}


//This function is used when IO redirection is needed : Fork and use freopen for IO redirection, then exec to run the given program. 
void IO_command(char* args[], int argCounter, char* cwd){
	//First fork then do the input/output redirection
	int pid = fork();
	//Create a seperate secondary array to hold all the arguments of the program which come before the '<', '>' or '>>' symbols
	char* secArgs[argCounter];
	switch(pid){
		case -1:
			printf("ERROR: Fork Error.\n");
			return;
		case 0:
			//Child process, set the parent variable in the environment first
			setenv("PARENT", originalCWD, 1);

			//Initialize the secondary array to parse out the redirection symbols and filenames
			for(int i = 0; i < argCounter; i++){
				if(strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0){
					//Ensure to set the last array value to NULL after we're done searching the array
					secArgs[i] = NULL;
					break;
				}
				secArgs[i] = args[i];
				printf("secArgs[%d]: %s\n", i, secArgs[i]);
			}

			//Only exec if the stdin/stdout we're changed succesfully, same thing throughout the rest of the code
			if(changeFileStream(args, argCounter, cwd)){
				execvp(args[0], secArgs);
			}
			//Use exec with the secondary arguments as we don't want to include the filenames and redirection symbols as arguments.
			exit(-1);
		default:
			//Parent here, wait for the child process finishes before continuing to execute, if there was a '&' then don't wait for the child
			if(!waitBool){
				waitpid(pid, NULL, 0);
			}
	}
}

//This is used when we don't need to do any IO redirection. We are simply forking and execing to run the program inputted by the user.
void otherProgram_command(char* args[]){
	int pid = fork();
	switch(pid){
		case -1:
			printf("ERROR: Fork Error.\n");
			break;
		case 0:
			//Child process here, set the 'parent' environment variable and use exec with the given arguments
			setenv("PARENT", originalCWD, 1);
			execvp(args[0], args);
			exit(-1);
		default:
			if(!waitBool){
				waitpid(pid, NULL, 0);
			}
	}
	return;
}

void dir_command(char* args[], int argCounter, char* cwd, int IO){
	//Create a seperate secondary array to hold all the arguments of the program which come before the '<', '>' or '>>' symbols
	char* secArgs[argCounter];
	int pid = fork();
	switch(pid){
		case -1:
			printf("ERROR: Fork Error.\n");
			break;
		case 0:
			//Child process here, set the 'parent' environment variable and use exec with the given arguments
			setenv("PARENT", originalCWD, 1);

			//Initialize the secondary array to parse out the redirection symbols and filenames
			for(int i = 0; i < argCounter; i++){
				if(strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0){
					//Ensure to set the last array value to NULL after we're done searching the array
					secArgs[i] = NULL;
					break;
				}
				secArgs[i] = args[i];
			}

			//Check for IO redirection and change the stdin/stdout as needed
			if(IO){
				if(changeFileStream(args, argCounter, cwd) == 0){
					return;
				}
			}

			//Check if these directories exist/can be opened. If they can't print the appropriate error message
			for(int i = 1; i < 3; i++){
				if(secArgs[i] != NULL){
					if(opendir(secArgs[i]) == NULL){
						printError();
						exit(-1);
					}
				}
			}

			//Run 'ls -la <dir1> <dir2>' using exec
			execlp("ls", "ls", "-la", secArgs[1], secArgs[2], NULL);
			//If it gets to this point there has been an error, exit with error code -1, make sure to free before exit
			exit(-1);
		default:
			if(!waitBool){
				waitpid(pid, NULL, 0);
			}
	}
}

//Change the filestream using freopen by looping through the arguments given, also check if given file exists and is readable/writeable
//Return 1 if success, 0 if failed
int changeFileStream(char* args[], int argCounter, char* cwd){
	for(int i = 0; i < argCounter; i++){
		if(strcmp(args[i], "<") == 0){
			char* fileName = appendFileName(args[i + 1], cwd);
			//For reading a file we check for errors if the file doesn't exist AND if the file isn't readable.
			if(access(fileName, F_OK) == 0){
				if(access(fileName, R_OK) == 0){
					freopen(fileName, "r", stdin);
				}
				else{
					free(fileName);
					printError();
					return 0;
				}
			}else{
				free(fileName);
				printError();
				return 0;
			}
			free(fileName);
		}
		else if(strcmp(args[i], ">") == 0){
			char* fileName = appendFileName(args[i + 1], cwd);
			//For both appending and writing to a file we can still replace stdout even if the file doesn't exist as 
			//we will simply create a new file. Otherwise the original is overwritten or appended to.
			if(access(fileName, F_OK) == 0){
				if(access(fileName, W_OK) == 0){
					freopen(fileName, "w", stdout);
				}
				else{
					//Only check for errors if the given file doesn't have write permissions
					free(fileName);
					printError();
					return 0;
				}
			}
			else{
				freopen(fileName, "w", stdout);
			}
			free(fileName);
		}
		else if(strcmp(args[i], ">>") == 0){
			char* fileName = appendFileName(args[i + 1], cwd);
			if(access(fileName, F_OK) == 0){
				if(access(fileName, W_OK) == 0){
					freopen(fileName, "a", stdout);
				}
				else{
					free(fileName);
					printError();
					return 0;
				}
			}else{
				freopen(fileName, "a", stdout);
			}
			free(fileName);
		}
	}
	return 1;
}

//Print all the arguments seperated by a single space. The arguments have already been parsed of \t and \n so we don't have to worry about that
//Use IO as there may be IO involved and we need to fork so as to not modify the stdin/stdout of the original process
void echo_command(char* args[], int argCounter, char* cwd, int IO){
	if(IO == 0){
		for(int i = 1; i < argCounter; i++){
			printf("%s ", args[i]);
		}
		printf("\n");
		return;
	}

	int pid = fork();
	switch(pid){
		case -1:
			printf("Fork Error.\n");
			break;
		case 0:
			setenv("PARENT", originalCWD, 1);

			//If there is IO redirection then make sure to use changeFileStream to change stdin/stdout
			if(IO){
				if(changeFileStream(args, argCounter, cwd) == 0){
					return;
				}
			}

			//Print all the arguments starting from 1 as the 0th argument is the command
			int i = 1;
			while(args[i] != NULL){
				//Do not print anything after or including the IO symbols, so we break the loop there
				if(strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0){
					break;
				}
				printf("%s ", args[i]);
				i++;
			}
			printf("\n");
			//Exit the child process to go back to terminal
			exit(0);
		default:
			if(!waitBool){
				waitpid(pid, NULL, 0);
			}
	}
	return;
}

//Display the help.txt using the 'more' UNIX program using fork/exec
void help_command(char* args[], int argCounter, char* cwd, int IO){
	int pid = fork();
	switch(pid){
		case -1:
			printf("Fork Error.\n");
			break;
		case 0:
			//Child process here
			setenv("PARENT", originalCWD, 1);
			if(IO){
				if(changeFileStream(args, argCounter, cwd) == 0){
					return;
				}
			}
			/*Use exec here with the more program, -d is used as "[Press space to continue, 'q' to quit.]" and will display "[Press 'h' for instructions.]" 
			instead of ringing the bell when an illegal key is pressed. Make sure to find the full path for the readme file using originalCWDCrop*/
			char* readmeFile = strcat(originalCWDCrop, "/readme.txt");
			execlp("more", "more", "-d", readmeFile, NULL);
			exit(-1);
		default:
			if(!waitBool){
				waitpid(pid, NULL, 0);
			}
	}
}

//Loop through and print all the environment variables
void environ_command(char* args[], int argCounter, char* cwd, int IO){
	//If there is no redirection then simply print and exit without fork/exec
	if(IO == 0){
		int i = 0;
		while(environ[i] != NULL){
			printf("%s\n", environ[i]);
			i++;
		}
		return;
	}

	int pid = fork();
	switch(pid){
		case -1:
			printf("Fork Error.\n");
			break;
		case 0:
			//Child process here
			setenv("PARENT", originalCWD, 1);
			if(IO){
				if(changeFileStream(args, argCounter, cwd) == 0){
					return;
				}
			}
			int i = 0;
			while(environ[i] != NULL){
				printf("%s\n", environ[i]);
				i++;
			}
			exit(0);
		default:
			if(!waitBool){
				waitpid(pid, NULL, 0);
			}
	}
}

//Change the directory and make sure to change the PWD environment variable, if the directory is not given then simply print the 
//current working directory. If there is an error then print the error message.
void cd_command(char* args[], char* cwd){
	if(args[1] == NULL){
		printf("CWD: %s\n", cwd);
	}
	else{
		//Change directory
		if(chdir(args[1]) == 0){
			//Change the environment variable
			char newCWD[MAX_BUFFER];
			getcwd(newCWD, MAX_BUFFER);
			setenv("PWD", newCWD, 1);
			return;
		}
		else{
			//Error 
			printError(errno);
			return;
		}
	}
	return;
}

//This function appends the fileName to the current working directory, giving the full path of the file.
char* appendFileName(char* fileName, char* cwd){
	if(fileName == NULL){
		return NULL;
	}
	//Malloc 200 in case there is a long file path
	char* ret = (char*) malloc(300 * sizeof(char));
	// printf("Ret-1: %s\n", ret);
	char* newRet = (char*) malloc(300 * sizeof(char));
	// printf("Ret0: %s\n", ret);
	// printf("New Ret0: %s\n", newRet);
	strcat(ret, cwd);
	strcat(newRet, cwd);
	// printf("Ret1: %s\n", ret);
	// printf("New Ret0: %s\n", newRet);
	strcat(ret, "/");
	strcat(newRet, "/");
	// printf("Ret2: %s\n", ret);
	// printf("New Ret0: %s\n", newRet);
	strcat(ret, fileName);
	strcat(newRet, fileName);
	// printf("Ret3: %s\n", ret);
	// printf("New Ret0: %s\n", newRet);
	return newRet;
}

//Function used to print the various error messages given.
void printError(int errNo){
	switch(errno){
		case EACCES:
			printf("ERROR: Permission is denied for one of the components of path.\n");
			return;
		case EFAULT:
			printf("ERROR: Path points outside your accessible address space.\n");
			return;
		case ENAMETOOLONG:
			printf("ERROR: Name of path is too long.\n");
			return;
		case ENOENT:
			printf("ERROR: The file/directory does not exist.\n");
			return;
		case ENOTDIR:
			printf("ERROR: A component of path is not a directory.");
			return;
		case EROFS:
			printf("ERROR: Write permission was requested for a file on a read-only file system.\n");
			return;
		default:
			printf("ERROR\n");
	}
	return;
}

//Allocate memory for a string and return a copy of that string inside the allocated memory
char* mystrdup(char* str){
	if(str != NULL){
		return strcpy(malloc(strlen(str) + 1), str);
	}
	return NULL;
}

//Pause the terminal and don't print any user input until the 'Enter' key is pressed
void pause_command(){
	printf("Terminal paused: Press 'Enter' to unpause!\n");
	char c;
	FILE * null_file = NULL;

	set_input_mode();
	null_file = fopen("/dev/null", "w");

	while (1) {
		read(STDIN_FILENO, &c, 1);
		if (c == '\n') { /* Catch Enter */
			break;
		}
		else {
			fprintf (null_file, "%c", c); /* Discard other input */
			fflush (null_file);
		}
	}
	printf("\n"); /* Ensure that the prompt is on a new line */
	//Close the file and reset the input mode so text appears on the terminal again
	fclose(null_file);
	reset_input_mode();
}

void reset_input_mode (void) {
	tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
}

void set_input_mode (void) {
	struct termios tattr;

	/* Make sure stdin is a terminal. */
	if (!isatty (STDIN_FILENO)) {
		fprintf (stderr, "Not a terminal.\n");
		exit (EXIT_FAILURE);
	}

	/* Save the terminal attributes so we can restore them later. */
	tcgetattr (STDIN_FILENO, &saved_attributes);
	atexit (reset_input_mode);

	/* Set the funny terminal modes. */
	tcgetattr (STDIN_FILENO, &tattr);
	tattr.c_lflag &= ~(ICANON|ECHO); /* Clear ICANON and ECHO. */
	tattr.c_cc[VMIN] = 1;
	tattr.c_cc[VTIME] = 0;
	tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
}
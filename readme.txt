/*
* README.txt - Alexander Ling 430391570
*/

How to use this shell:

To use this shell simply input one of the commands below and provide the desired arguments. (Don't forget to press Enter!) The shell will print out a prompt which will display the current working directory in which the user may type in their desired commands. The shell may be exited via the 'quit' command or 'Ctrl + C' or just by closing the terminal.

The user may also run the shell using a batchfile input instead of typing commands into the prompt. This may be done by running the shell and then typing the path of the batchfile afterwards. Eg. "./myshell batchfile". This will cause the shell to run all of the commands in the batchfile until either an error or the end of file is found.




INTERNAL COMMANDS:

cd <directory> - Change the current working directory to the new directory if it exists, otherwise print the error. If no directory is inputted then print the current working directory. Everytime the directory is changed the PWD environment variable is also changed to the current directory.

clr - Clear the screen and move the prompt to the top of the screen.

dir <dir1> <dir2> - Prints out the contents of the inputted directories including files and folders. The absolute paths of the directories can be given, otherwise it searched for the directories on the current path. Using two directories is optional and this command will work with only one directory argument. If the given directories are invalid an error message will be printed.

environ - Print the environment variable including all the strings.

echo <comment> - Print the 'comment' onto the console followed by a new line. It also parses all tabs and extra spaces in the comment and prints all of the given arguments with a single whitespace in between. If no <comment> is provided then simply print a single new line.

help - Display this user manual.

pause - Pause the shell until the 'Enter' key is pressed. Nothing will be able to be outputted until the 'Enter' is pressed or the shell is exited.

quit - Quit the shell.




EXTERNAL COMMANDS:

If the user doesn't type an internal command then the shell will attempt to interpret the input and run it as an external command in the default shell. All arguments are also given to the shell. The external commands are run by creating a seperate process and running the external command inside the other process. After the command has finished we return to the execution of our shell.




ENVIRONMENTS CONCEPTS:

In this shell there is an envrionment variable which is a large group of strings. These strings contain information concerning the shells environment. By default there will be a variable called "SHELL" which will be set to the location of the myshell file. Moreover there is another environment variable "PWD" which contains the location of the current working directory. This variable is changed every time the user successfully changes directory using the 'cd' command. There are also a few other environment variables that are added such as the "PARENT" variable however I would have to go to too much depth (for a casual user) to explain the reasoning behind it.




I/O REDIRECTION:

If desired, users can take input from other files instead of typing in the command line. Moreover users can also output to files. This is done by using the '< file', '> file', and '>> file' symbols and providing a file after the symbol. The symbols mean 'take input from file', 'output to file', and 'append to file' respectively; where 'file' is the input/output file desired. If the file is not found or we do not have permission to read/write to the file then an appropriate message is printed.

The supported I/O redirection functions are 'dir', 'help', 'echo', and 'environ'. For example by using I/O redirection it is possible to run 'echo hello world > hello.txt'. This would result in overwriting the hello.txt file with "Hello world" if it exists. If the file doesn't exist, a new file called hello.txt is created and "Hello world" is outputted to the file.

If the user does not want to overwrite what is in a file and instead append to the file then they must use ">>" which is similar to the ">" symbol but instead appends to the file and does not overwrite any existing data in the file.

The "<" symbol means to take input from a file. For example we can sort input from "dictionary.txt" by using the command "sort < dictionary.txt". This would run the sort external program with the data from dictionary.txt




BACKGROUND EXECUTION:

A program is allowed to run in the background of the shell. This means  the shell AND the program run at the same time, without halting execution of the shell. As a result users may continue to run other commands while  programs are still running in the background.  

If a program is to run in the background, simply put a single '&' character seperated by whitespace at the end of the command. For example if a user could type "sleep 3 &" to run the sleep external command in the background. This would normally pause the shell for 3 seconds, however since it is in the background the shell continues execution.


End of readme.txt - Thanks for reading brah
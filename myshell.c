#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 64

// Setting up the shel;
// Turning user input into arguments 
void parse_input(char *input, char **args) {
    int i = 0;
    args[i] = strtok(input, " \t\n");
    while(args[i] != NULL && i < MAX_ARGS - 1){
	args[++i] = strtok(NULL, "\t\n");
    }
    args[i] = NULL;
}

// Clearing the screen
void clear_screen(){
	printf("\033[2J\033[H"); // Clear the screen and moving the top left
}

int main(){
	char input[MAX_INPUT_SIZE];
	char *args[MAX_ARGS];

	while(1){
		// Print current directory as a prompt
		char cwd[256];
		getcwd(cwd, sizeof(cwd));
		printf("%s $ ", cwd);
		fflush(stdout);

		// Read input
		if(fgets(input, sizeof(input), stdin) == NULL){
			printf("\n");
			break;
		}

		// Ignore empty input
		if(strcmp(input, "\n") == 0) continue;
		parse_input(input, args);
		if(args[0] == NULL) continue;

		// Built-in: quit 
		if(strcmp(args[0], "quit") == 0){
			break;
		}

		// Built-in: cd -> check directory
		if(strcmp(args[0], "cd") == 0){
			if(args[1] == NULL){
				fprintf(stderr, "cd: Missing Argument!\n");
			} else if(chdir(args[1]) != 0){
				perror("cd");
			}
			continue;
		}

		// Built-in: clr -> clear
		if(strcmp(args[0], "clear") == 0){
			clear_screen();
			continue;
		}

		// Built-in: dir <directory> -> list directory contents
		if(strcmp(args[0], "dir") == 0){
			char *dir = args[1] ? args[1] : ".";
			char command[MAX_INPUT_SIZE];
			snprintf(command, sizeof(command), "ls -l %s", dir);
			system(command);
			continue;
		}

		// Built-in: environ -> print environment variables
		if(strcmp(args[0], "environ") == 0){
			extern char **environ;
			for(char **env = environ; *env != NULL; env++){
				printf("%s\n", *env);
			}
		}

		// Built-in: echo <message> -> print message
		if(strcmp(args[0], "echo") == 0){
			for(int i = 1; args[i] != NULL; i++){
				printf("%s ", args[i]);
			}
		}

		// Built-in: pause -> pause operation of the shell until "Enter" is pressed 
		if(strcmp(args[0], "pause") == 0){
			printf("Press Enter to continue...");
			while(getchar() != '\n');
			continue;
		}

		// Built-in: help -> display help message
		if(strcmp(args[0], "help") == 0){
			printf("Built-in commands:\n");
			printf("  quit - Exit the shell\n");
			printf("  cd <directory> - Change the current directory\n");
			printf("  clear - Clear the screen\n");
			printf("  dir <directory> - List contents of a directory\n");
			printf("  environ - Print environment variables\n");
			printf("  echo <message> - Print a message\n");
			continue;
		}
	}
}



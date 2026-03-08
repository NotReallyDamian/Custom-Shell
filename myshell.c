#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 64

// Setting up the shell;
// Turning user input into arguments 
void parse_input(char *input, char **args, char **input_file, char **output_file, int *append, int *background) {
    *input_file = NULL;
    *output_file = NULL;
    *append = 0;
    *background = 0;
    int i = 0;
    char *token = strtok(input, " \t\n");
    while(token != NULL && i < MAX_ARGS - 1){
        if(strcmp(token, "<") == 0){
            token = strtok(NULL, " \t\n");
            if(token) *input_file = token;
        } else if(strcmp(token, ">>") == 0){
            token = strtok(NULL, " \t\n");
            if(token) *output_file = token;
            *append = 1;
        } else if(strcmp(token, ">") == 0){
            token = strtok(NULL, " \t\n");
            if(token) *output_file = token;
            *append = 0;
        } else if(strcmp(token, "&") == 0){
            *background = 1;
        } else {
            args[i++] = token;
        }
        token = strtok(NULL, " \t\n");
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
	char *input_file, *output_file;
	int append, background;

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
		parse_input(input, args, &input_file, &output_file, &append, &background);
		if(args[0] == NULL) continue;

		// Built-in: exit 
		if(strcmp(args[0], "exit") == 0){
			break;
		}

		// Built-in: cd -> check directory
		if(strcmp(args[0], "cd") == 0){
			if(args[1] == NULL){
				fprintf(stderr, "cd: Missing Argument!\n");
			} else {
				int saved_stdin = dup(0);
				int saved_stdout = dup(1);
				if(input_file){
					int fd = open(input_file, O_RDONLY);
					if(fd == -1){ perror("input file"); continue; }
					dup2(fd, 0);
					close(fd);
				}
				if(output_file){
					int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
					int fd = open(output_file, flags, 0644);
					if(fd == -1){ perror("output file"); continue; }
					dup2(fd, 1);
					close(fd);
				}
				if(chdir(args[1]) != 0){
					perror("cd");
				}
				dup2(saved_stdin, 0);
				dup2(saved_stdout, 1);
				close(saved_stdin);
				close(saved_stdout);
			}
			continue;
		}

		// Built-in: pwd -> print working directory
		if(strcmp(args[0], "pwd") == 0){
			int saved_stdin = dup(0);
			int saved_stdout = dup(1);
			if(input_file){
				int fd = open(input_file, O_RDONLY);
				if(fd == -1){ perror("input file"); continue; }
				dup2(fd, 0);
				close(fd);
			}
			if(output_file){
				int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
				int fd = open(output_file, flags, 0644);
				if(fd == -1){ perror("output file"); continue; }
				dup2(fd, 1);
				close(fd);
			}
			printf("%s\n", cwd);
			dup2(saved_stdin, 0);
			dup2(saved_stdout, 1);
			close(saved_stdin);
			close(saved_stdout);
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
			continue;
		}

		// Built-in: echo <message> -> print message
		if(strcmp(args[0], "echo") == 0){
			int saved_stdin = dup(0);
			int saved_stdout = dup(1);
			if(input_file){
				int fd = open(input_file, O_RDONLY);
				if(fd == -1){ perror("input file"); continue; }
				dup2(fd, 0);
				close(fd);
			}
			if(output_file){
				int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
				int fd = open(output_file, flags, 0644);
				if(fd == -1){ perror("output file"); continue; }
				dup2(fd, 1);
				close(fd);
			}
			for(int i = 1; args[i] != NULL; i++){
				printf("%s ", args[i]);
			}
			printf("\n");
			dup2(saved_stdin, 0);
			dup2(saved_stdout, 1);
			close(saved_stdin);
			close(saved_stdout);
			continue;
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
			printf("  exit - Exit the shell\n");
			printf("  cd <directory> - Change the current directory\n");
			printf("  pwd - Print working directory\n");
			printf("  clear - Clear the screen\n");
			printf("  dir <directory> - List contents of a directory\n");
			printf("  environ - Print environment variables\n");
			printf("  echo <message> - Print a message\n");
			continue;
		}

		// External command execution
		if(args[0] != NULL){
			pid_t pid = fork();
			if(pid == 0){
				if(input_file){
					int fd = open(input_file, O_RDONLY);
					if(fd == -1){ perror("input file"); exit(1); }
					dup2(fd, 0);
					close(fd);
				}
				if(output_file){
					int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
					int fd = open(output_file, flags, 0644);
					if(fd == -1){ perror("output file"); exit(1); }
					dup2(fd, 1);
					close(fd);
				}
				execvp(args[0], args);
				perror("exec");
				exit(1);
			} else if(pid > 0){
				if(!background){
					wait(NULL);
				}
			} else {
				perror("fork");
			}
		}
	}
}



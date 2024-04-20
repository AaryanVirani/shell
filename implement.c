#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "implement.h"

void changedirectories(char **args, int check){
	struct stat stats; //https://codeforwin.org/c-programming/c-program-check-file-or-directory-exists-not
	int result = stat(args[1], &stats);

	if (check == 1 || check > 2){
		fprintf(stderr, "Error: invalid command\n");
	}
	else if (S_ISDIR(stats.st_mode) && result == 0){ //Borrowed from: https://codeforwin.org/c-programming/c-program-check-file-or-directory-exists-not
		chdir(args[1]);
	}
	else{
		fprintf(stderr, "Error: invalid directory\n");
	}
}

void redirection(char **args, int index){
	char *out_file = NULL;
	char *in_file = NULL;
	int out_flag = 0;
	int append_flag = 0;
	int in_flag = 0;

	for (int i = 0; i < index; i++){
		if (strcmp(args[i], ">") == 0){
			if (out_flag){
				fprintf(stderr, "Too many redirections\n");
				exit(-1);
			}
			out_flag = 1;
			if (args[i+1] == NULL){
				fprintf(stderr, "No output file\n");
				exit(-1);
			}
			out_file = args[i+1];
			args[i] = NULL;
		}
		else if (strcmp(args[i], ">>") == 0){
			if (append_flag){
				fprintf(stderr, "Too many redirections\n");
				exit(-1);
			}
			append_flag = 1;
			if (args[i+1] == NULL){
				fprintf(stderr, "No output file\n");
				exit(-1);
			}
			out_file = args[i+1];
			args[i] = NULL;
		}
		else if (strcmp(args[i], "<") == 0){
			if (in_flag){
				fprintf(stderr, "Too many redirections\n");
				exit(-1);
			}
			in_flag = 1;
			if (args[i+1] == NULL){
				fprintf(stderr, "Error: invalid file\n");
				exit(-1);
			}
			in_file = args[i+1];
			args[i] = NULL;
		}
	}

	if (in_flag){
		int filedesc = open(in_file, O_RDONLY);
		if (filedesc == -1){
			fprintf(stderr, "Error: invalid file\n");
			exit(-1);
		}
		dup2(filedesc, STDIN_FILENO);
		close(filedesc);
	}

	if (out_flag){
		int filedesc = open(out_file, O_TRUNC | O_WRONLY| O_CREAT, 0644); //wasn't working without 0644...used this: https://www.multacom.com/faq/password_protection/file_permissions.htm 
		if (filedesc == -1){
			fprintf(stderr, "Error: cannot create file\n");
			exit(-1);
		}
		dup2(filedesc, STDOUT_FILENO);
		close(filedesc);
		
	}
	else if(append_flag){
		int filedesc = open(out_file, O_APPEND | O_WRONLY| O_CREAT, 0644);
		if (filedesc == -1){
			fprintf(stderr, "Error: cannot create/open file\n");
			exit(-1);
		}
		dup2(filedesc, STDOUT_FILENO);
		close(filedesc);
	}
}


void pipes(char **leftargs, char **rightargs, int leftindex, int rightindex){
	int original_input = dup(STDIN_FILENO);
	int original_output = dup(STDOUT_FILENO);
	int pipefd[2];
	pid_t cpid1, cpid2;
	
	if (pipe(pipefd) == -1){
		fprintf(stderr, "Error: pipe");
		exit(-1);
	}

	redirection(leftargs, leftindex);
	redirection(rightargs, rightindex);

	cpid1 = fork();
	if (cpid1 == -1){
		fprintf(stderr, "Error: fork");
		exit(-1);
	}

	else if (cpid1 == 0){
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);

		execvp(leftargs[0], leftargs);
		fprintf(stderr, "Error: execvp");
	}

	else{
		cpid2 = fork();

		if (cpid2 == -1){
			fprintf(stderr, "Error: fork child 2");
			exit(-1);
		}

		else if (cpid2 == 0){
			close(pipefd[1]);
			dup2(pipefd[0], STDIN_FILENO);
			close(pipefd[0]);

			execvp(rightargs[0], rightargs);
			fprintf(stderr, "Error: executing 2nd process");
			exit(-1);
		}
		else{
			close(pipefd[0]);
			close(pipefd[1]);
			int status1;
			int status2;
			waitpid(cpid1, &status1, 0);
			waitpid(cpid2, &status2, 0);

			dup2(original_input, STDIN_FILENO);
			dup2(original_output, STDOUT_FILENO);
			close(original_input);
			close(original_output);
		}
	}
}

void multipipes(char ***pipeargs, int *pipeargcount, int cmdcount){
	int original_input = dup(STDIN_FILENO);
	if (original_input == -1){
		fprintf(stderr, "Error: dup input\n");
		exit(-1);
	}

	int original_output = dup(STDOUT_FILENO);
	if (original_output == -1){
		fprintf(stderr, "Error: dup output\n");
		exit(-1);
	}

	int numpipes = cmdcount -1;
	int pipefds[numpipes][2];

	for (int i = 0; i < numpipes; i++){
		if (pipe(pipefds[i]) < 0){
			fprintf(stderr, "Error: pipe\n");
			exit(-1);
		}
	}

	for (int i = 0; i < cmdcount; i++){

		pid_t cpid = fork();

		if (cpid == -1){
			fprintf(stderr, "Error: fork\n");
			exit(-1);
		}

		else if(cpid == 0){
			printf("%d", pipeargcount[i]);
			if (i == 0){
				if (dup2(pipefds[0][1], STDOUT_FILENO) == -1){
					fprintf(stderr, "Error: dup2\n");
					exit(-1);
				}
			}
			else if (i == cmdcount -1){
				if (dup2(pipefds[numpipes-1][0], STDIN_FILENO) == -1){
					fprintf(stderr, "Error: 2nd dup2\n");
					exit(-1);
				}
			}
			else{
				if (dup2(pipefds[i-1][0], STDIN_FILENO) == -1){
					fprintf(stderr, "Error: 3rd dup2\n");
					exit(-1);
				}
				if (dup2(pipefds[i][1], STDOUT_FILENO) == -1){
					fprintf(stderr, "Error: 4th dup2\n");
					exit(-1);
				}
			}

			for (int j = 0; j < numpipes; j++){
				close(pipefds[j][0]);
				close(pipefds[j][1]);
			}
			execvp(pipeargs[i][0], pipeargs[i]);
			fprintf(stderr, "Error:execvp\n");
			exit(-1);
		}
	}

	for (int i = 0; i < numpipes; i++){
		close(pipefds[i][0]);
		close(pipefds[i][1]);
	}

	int status;
	for (int i = 0; i < cmdcount; i++){
		wait(&status);
	}

	if (dup2(original_input, STDIN_FILENO) == -1){
		fprintf(stderr, "Error: 6th dup2\n");
		exit(-1);
	}
	if (dup2(original_output, STDOUT_FILENO) == -1){
		fprintf(stderr, "Error: 5th dup\n");
		exit(-1);
	}
	close(original_input);
	close(original_output);
}

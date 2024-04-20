#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>

#include "implement.h"

typedef struct job{
	pid_t pid;
	char* cmd;
	int num;
	bool suspendedflag;
} job_t;

job_t joblist[100];
int numjobs = 0;

void add(pid_t pid, char* cmd, bool sus){
	if (numjobs == 100){
		fprintf(stderr, "Error: too many jobs\n");
		return;
	}
	job_t new = {pid, strdup(cmd), ++numjobs, sus};
	joblist[numjobs-1] = new;
}

void removejob(int jobindex){
	for (int i = 0; i < numjobs; i++){
		if (joblist[i].num == jobindex){
			free(joblist[i].cmd);

			for (int j = i; j < numjobs-1; j++){
				joblist[j] = joblist[j + 1];
			}

			numjobs--;
			break;
		}
	}
}

void print(){
	for(int i = 0; i < numjobs; i++){
		if (joblist[i].suspendedflag == true)
			printf("[%d] %s\n", joblist[i].num, joblist[i].cmd);
	}
}

void foreground(int jobind){
	pid_t pid = -1;

    for (int i = 0; i < numjobs; i++) {
        if (joblist[i].num == jobind) {
            pid = joblist[i].pid;
            joblist[i].cmd = false;
            removejob(jobind);
            break;
        }
    }

    if (pid == -1) {
        fprintf(stderr, "Error: invalid job\n");
        return;
    }

    kill(pid, SIGCONT);

    int status;
    waitpid(pid, &status, WUNTRACED);

    if (WIFSTOPPED(status)) {
        add(pid, joblist[numjobs-1].cmd, true);
    }
}

void handlesignal(int signum){
	if (signum == SIGINT || signum == SIGQUIT){
		printf("\n");
		return;
	}
}

void sigtstphandler(int sig){
	if (sig == 20){
		kill(getpid(), SIGTSTP);
		add(getpid(), joblist[numjobs-1].cmd, true);
	}
}

int main(){
	char path[PATH_MAX];
	char *buffer = NULL;
	size_t buffsize = 0;
	bool pipeflag = false;
	int pipecheck;
	int numpipes = 0;

	signal(SIGINT, handlesignal);
	signal(SIGQUIT, handlesignal);
	signal(SIGTSTP, handlesignal);

	char *args[10];

	while (true){

		if (getcwd(path, sizeof(path)) == NULL) {
            fprintf(stderr, "Error\n");
            return 1;
        }

		printf("[nyush %s]$ ", strrchr(path, '/') + 1);
		fflush(stdout);

		int check = getline(&buffer, &buffsize, stdin);
		if (check <= 1){
			//fprintf(stdout, "\n");
			exit(0);
		}

		if (buffer[check-1] == '\n'){
			buffer[check-1] = '\0';
			check--;
		}
	
		int i = 0;
		char *token = strtok(buffer, " "); //https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm 
		while (token != NULL){ //found out that I can divide the input using the following method
			if (strchr(token, '|')){
				pipeflag = true;
				pipecheck = i;
				numpipes = numpipes + 1;
			}
			args[i++] = token;
			token = strtok(NULL, " ");
		}
		args[i] = NULL;

		if (strcmp(args[0], ">") == 0 || strcmp(args[0], "<") == 0 || strcmp(args[0], ">>") == 0 || strcmp(args[0], "|") == 0){
			fprintf(stderr, "Error: invalid command\n");
			continue;
		}

		if((strcmp(args[i-1], ">") == 0 && args[i] == NULL)||(strcmp(args[i-1], "<") == 0 && args[i] == NULL)||(strcmp(args[i-1], ">>") == 0 && args[i] == NULL)||(strcmp(args[i-1], "|") == 0 && args[i] == NULL)){
			fprintf(stderr, "Error: invalid command\n");
			continue;
		}

		if (strcmp(buffer, "exit") == 0){
			if (i > 1){
				fprintf(stderr, "Error: invalid command\n");
				continue;
			}
			else{
				bool sus = false;
				for (int j = 0; j < numjobs; j++){
					if (kill(joblist[j].pid, 0) == 0){
						sus = true;
						break;
					}
				}
				if (sus){
					fprintf(stderr, "Error: there are suspended jobs\n");
					continue;
				}
				else{
					exit(0);
				}
			}
		}
		else if (strcmp(buffer, "cd") == 0){
			changedirectories(args, i);
		}
		else if (strcmp(buffer, "jobs") == 0){
			print();
		}
		else if (strcmp(buffer, "fg") == 0){
			if (i != 2){
				fprintf(stderr, "Error: invalid command\n");
				continue;
			}
			int jobindex = atoi(args[1]);
			if (jobindex <= 0 || jobindex > numjobs){
				fprintf(stderr, "Error: invalid job\n");
				continue;
			}
			foreground(jobindex);
		}
		else if (i > 2 && pipeflag){
			if (args[0] == NULL || args[i-1] == NULL){
				fprintf(stderr, "Error: invalid command\n");
				exit(-1);
			}
			else{
				if (numpipes == 1){
					char *leftargs[10];
					char *rightargs[10];
					int j = 0;

					for (j = 0; j < pipecheck; j++){
						leftargs[j] = args[j];
					}
					leftargs[j] = NULL;
					int leftindex = j;

					int k = 0;

					for (j = pipecheck + 1; args[j] != NULL; j++){
						rightargs[k++] = args[j];
					}
					rightargs[k] = NULL;
					int rightindex = k;

					pipes(leftargs, rightargs, leftindex, rightindex);
				}
				else{
					char **cmd[10];
					int cmdcount = 0;
					int argscount[10] = {0};

					cmd[0] = &args[0];
					argscount[0]++;

					for (int j = 0; args[j] != NULL; j++){
						if (strcmp(args[j], "|") == 0){
							args[j] = NULL;
							cmdcount++;
							cmd[cmdcount] = &args[j+1];
							continue;
						}
						argscount[cmdcount]++;
					}
					cmdcount++;

					multipipes(cmd, argscount, cmdcount);
				}
			}
		}
		else{
			pid_t pid = fork();
	        if (pid < 0) {
		    // fork failed (this shouldn't happen)
	        	fprintf(stderr, "Fork failed\n");
		    }
		    else if (pid == 0) {
		    // child (new process)
		    	redirection(args, i);
		    	execvp(args[0], args);  // or another variant
		    	fprintf(stderr, "Error: invalid program\n");
		    	exit(-1);
		    // exec failed
			} 
			else {
		    // parent
				int status;
				waitpid(-1, &status, 0);
			}
		}
		pipecheck = false;
		pipeflag = false;
	}
	return 0;
}
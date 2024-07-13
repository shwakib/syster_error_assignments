#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>

pid_t backgroundProcess=-1;

char* getCommand(char* command) {
    //this function will return the user command after the user hits enter by removing the \n & null character.
    int i = 0;
    while (command[i] != '\0') {
        if (command[i] == '\n') {
            command[i] = '\0';
            break;
        }
        i++;
    }

    return command;
}

void openingNewTerminal() {
    pid_t pid = fork(); //forks to open terminal from a child process.

    if (pid < 0) {

        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {

        execlp("xterm", "xterm", "-e", "./shell24", "shell24$newt", NULL); //using execlp to open a terminal
        
        perror("exec failed"); //error
        exit(EXIT_FAILURE);
    } else {

    }
}

void concatenateFiles(const char* files) {
    const char* delimiter = " # ";
    char* fileNames[5]; // Maximum 5 files
    int fileCount = 0;

    char buffer[1024]; // Buffer for reading file contents

    // Tokenize the input string to get file names
    char* token = strtok((char*)files, delimiter);
    while (token != NULL && fileCount < 5) {
        fileNames[fileCount++] = token;
        token = strtok(NULL, delimiter);
    }

    // Open and concatenate files
    for (int i = 0; i < fileCount; i++) {
        FILE* file = fopen(fileNames[i], "r");
        if (file == NULL) {
            fprintf(stderr, "Error: Unable to open file %s\n", fileNames[i]);
            continue; // Skip to the next file
        }

        // Read and print content of the file
        while (fgets(buffer, sizeof(buffer), file) != NULL) {
            printf("%s", buffer);
        }

        fclose(file); // Close the file
    }
}


void executePiping(char *command) {
    int num_commands = 0; //to track the number of commands in the pipeline.
    char **commands = malloc(sizeof(char *) * 30); //just allocating space for 30 commands. 

    char *token = strtok(command, "|"); // tokenix=zing commands by using |
    while (token != NULL) { 
        commands[num_commands] = strdup(token); //allocates memory for a copy of the token and stores it in the commands array 
        num_commands++; 

        if (num_commands % 30 == 0) { //check if the number of commands is multiple of 30 
            commands = realloc(commands, sizeof(char *) * (num_commands + 30)); // reallocating more memory if needed
        }

        token = strtok(NULL, "|"); //pointing to the next character
    }

    int pipes[num_commands - 1][2]; //2D array for storing pipe file descriptors

    for (int i = 0; i < num_commands; i++) { 
        //the number of pipes needed for the commands
        if (i < num_commands - 1) { //the required number of pipe will be always number of commands -1 
            if (pipe(pipes[i]) == -1) { 
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            if (i > 0) { //that means there is a output from the previous command which means that will be input for the current command.
                dup2(pipes[i-1][0], STDIN_FILENO);
                close(pipes[i-1][0]);
                close(pipes[i-1][1]);
            }
            if (i < num_commands - 1) { //that means there are some outputs whose input will be the output of the current command. 
                dup2(pipes[i][1], STDOUT_FILENO);
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            char *args[10]; // to store commands and its arguments
            char *token = strtok(commands[i], " "); //tokenizing by " "
            int j = 0; // to keep track in the args array
            
            while (token != NULL) { //storing the token extracted by strtok
                args[j++] = token;
                token = strtok(NULL, " ");
            }
            args[j] = NULL;
            
            execvp(args[0], args); //after storing all the commands execvp is called to replace the cuurent execution
            perror("execvp");
            exit(EXIT_FAILURE);
        } else {
            if (i > 0) { //to cleanup the pipe resources in the parent process
                close(pipes[i-1][0]);
                close(pipes[i-1][1]);
            }
        }
    }

    for (int i = 0; i < num_commands - 1; i++) { // iterates over each pipe and close the both ends.
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = 0; i < num_commands; i++) { // wait for each child to finish execution
        wait(NULL);
    }

    for (int i = 0; i < num_commands; i++) {
        free(commands[i]); //free arguments storage
    }
    free(commands); //free the storage array
}

void executeRedirection(char* command) {
    char* args[10]; //max number of commands
    int argCount = 0;

    pid_t pid=fork();

    if(pid<0){
        printf("forking error!");
    } else if(pid == 0){
        char* token = strtok(command, " "); //tokenizing the command based on " "
    while (token != NULL && argCount < 10) {
        args[argCount++] = token; // storing each command in the args array
        token = strtok(NULL, " "); //pointing to the the next character
    }
    args[argCount] = NULL;
    int fd;
    int i;
    for (i = 0; i < argCount; i++) {
        if (strcmp(args[i], ">") == 0) { //if the user input >
            fd = open(args[i + 1], O_WRONLY | O_TRUNC); // opening the file in write mode or TRUNC
            if (fd < 0) {
                perror("open failed");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO); //redirecting the output pointer
            close(fd);
            args[i] = NULL; //adding NULL to the end of the array
            break;
        } else if (strcmp(args[i], ">>") == 0) { //if the user input >>
            fd = open(args[i + 1], O_WRONLY | O_APPEND); //opening the file in writeonly or append mode
            if (fd < 0) {
                perror("open failed");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL; 
            break; 
        } else if (strcmp(args[i], "<") == 0) { //if the user gives <
            fd = open(args[i + 1], O_RDONLY); //opening the file in readonly mode
            if (fd < 0) {
                perror("open failed");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
            args[i] = NULL;
            break;
        }
    }
    execvp(args[0], args);
    perror("exec failed");
    exit(EXIT_FAILURE);
    } else{
        wait(NULL);
    }
}

void conditionalExecution(char* command, const char* delimiter) {
    char* token = strtok(command, delimiter); //tokenizing the commands using delimeter
    int condition = 1; //to check if subsequent commands should be executed based on the exit status of previous commands.

    while (token != NULL) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            char* args[6]; //upto 5 execution
            int argCount = 0;

            char* individualCommand = strtok(token, " "); //tokenizing based on " "
            while (individualCommand != NULL && argCount < 5) { //storing individual command in the args array
                args[argCount++] = individualCommand;
                individualCommand = strtok(NULL, " "); //pointing to the next character
            }
            args[argCount] = NULL;

            if (argCount >= 1 && argCount <= 5) {  //checking rule 2
                execvp(args[0], args);
                perror("exec failed");
                exit(EXIT_FAILURE);
            } else {
                fprintf(stderr, "Error: Number of arguments exceeds limit (1-5).\n");
                exit(EXIT_FAILURE);
            }
        } else {
            int status;
            waitpid(pid, &status, 0); //waiting for the child to finish execution

            if (WIFEXITED(status)) { // to check if the child process finished normally & extract the status code
                int exit_status = WEXITSTATUS(status);
                if (*delimiter == '&' && exit_status != 0) { //command failed
                    condition = 0; //changing the condition to let the program know that subsequent commands should not be executed
                } else if (*delimiter == '|' && exit_status == 0) {
                    condition = 0; 
                }
            }
        }

        if (!condition) { //to check if the condition is false or not 
            break;
        }

        token = strtok(NULL, delimiter); //retrieve the next token from the command string
    }
}

void pushProcessBackground(char* command){

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        char* args[4]; //initialing initial size for the args array
        int argCount = 0;

        char* individualCommand = strtok(command, " "); //tokenizing the command using " ". 
        while (individualCommand != NULL && argCount < 5) { //checking the number of arguments
            args[argCount++] = individualCommand; //storing each command in the args array
            individualCommand = strtok(NULL, " "); //then extracting each of the commands based on " "
        }
        args[argCount] = NULL; //adding NULL at the end of the array

        execvp(args[0], args); //executing
        perror("exec failed");
        exit(EXIT_FAILURE);
    } else {
        backgroundProcess = pid; //storing the PID for bringing the process foreground
    }
}

void bringProcessFront(){
    if (backgroundProcess != -1) {
        int status;
        waitpid(backgroundProcess, &status, WUNTRACED); //to wait for the specified process. in this case, the process we pushed background to change state
        backgroundProcess = -1; 
    } else {
        
    }
}

void executeCommandsSequentially(char* command) {
    const char* delimiter = ";"; //tokenizing based on ";"
    char* token = strtok(command, delimiter); 
    while (token != NULL) {      
        
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            char* args[6]; //max 5 commands
            int argCount = 0;

            char* individualCommand = strtok(token, " "); //tokenizing each command by " "
            while (individualCommand != NULL && argCount < 5) {
                args[argCount]=individualCommand; //storing each command individually in args
                argCount++;
                individualCommand = strtok(NULL, " "); //retrieving the next command
            }
            args[argCount] = NULL; //adding NULL

            if (argCount >= 1 && argCount <= 5) { //checking rule 2
                execvp(args[0], args);  //executing
                perror("exec failed");
                exit(EXIT_FAILURE);
            } else {
                fprintf(stderr, "Error: Number of arguments exceeds limit (1-5).\n");
                exit(EXIT_FAILURE);
            }
        } else {
            wait(NULL); //waiting for the child to finish execution
        }

        token = strtok(NULL, delimiter);
    }
}

int main() {
    
    while (1) {
        printf("shell24$ ");
        char command[256]; //to store user given command
        fgets(command, sizeof(command), stdin); //get user input from cmd

        char* userCommand=getCommand(command); //getting the user command after removing everything
        if (strcmp(userCommand,"newt")==0) {
            openingNewTerminal(); //opening new terminal if the user gave newt
        } else{
            char* occurrence=strpbrk(userCommand,"#|><&;"); //to check if there is any existance of any specified special character
            if (occurrence != NULL) {
                if (*occurrence == '#') {
                    concatenateFiles(userCommand);
                } else if(*occurrence == '|'){
                    char *firstInstance=strstr(userCommand, "||");
                    if(firstInstance!=NULL){
                        conditionalExecution(userCommand, "||");
                    } else{
                        executePiping(userCommand);
                    }
                } else if (*occurrence == '>' || *occurrence == '<' || strcmp(occurrence, ">>") == 0) {
                    executeRedirection(userCommand);
                }else if(*occurrence == '&'){
                    char *instance_of_and = strstr(userCommand, "&&");
                    char *instance_of_or = strstr(userCommand, "||");
                    if (instance_of_and != NULL && instance_of_or != NULL) {
                        conditionalExecution(userCommand, "&&||");
                    } else if (instance_of_and != NULL) {
                        conditionalExecution(userCommand, "&&");
                    }
                    else {
                        pushProcessBackground(userCommand);
                    }
                }
                else if(*occurrence == ';'){
                    executeCommandsSequentially(userCommand);
                }
            } else if(strcmp(userCommand, "fg") == 0){
                    bringProcessFront();
                } else {
                
            }
        }
    }
    
    return 0;
}
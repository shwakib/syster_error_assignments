#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

//getMaxPID function will get the total number of processes allowed in the system by the system administrator because the max number of allowed PID can be differnt  for each machine. 
int getMaxPID() {
    FILE *file;
    char maxID[256];
    #define cmdForMaxPID "/proc/sys/kernel/pid_max" //To get the maximum number of PID I am using proc/sys/kernel/pid_max. 

    // Opening the file to read the maximum PID.
    file = fopen(cmdForMaxPID, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Failed to open file %s\n", cmdForMaxPID);
        return -1;
    }

    // If there is nothing in the fp & returns NULL then it means there is nothing and it will -1 otherwise it will convert the output into integer value using atoi. 
    if (fgets(maxID, sizeof(maxID), file) == NULL) {
        fprintf(stderr, "Error: Failed to read from file %s\n", cmdForMaxPID);
        fclose(file);
        return -1;
    }

    // Close the file
    fclose(file);

    //returning the maxID by converting it into integer
    return atoi(maxID);
}

// In getParentProcessID, I am getting the parentProcessID for the given processID. 
int getParentProcessID(int processID) {
    char statusPath[256];
    FILE *file;
    int parentProcessID = -1;

    // To get the PPID, I am using proc/%d/status where all the information regarding a PID is present.
    sprintf(statusPath, "/proc/%d/status", processID);

    file = fopen(statusPath, "r");
    if (file == NULL) {
        // fprintf(stderr, "Error: Failed to open status file for process %d\n", processID);
        return -1;
    }

    char line[256]; //it is stored to each line of the file.
    while (fgets(line, sizeof(line), file) != NULL) { //as long the file won't reach its end, the loop will continue. Once, the while loop reaches the end of the file, it will return NULL>
        if (strncmp(line, "PPid:", 5) == 0) { //extracting out the portion after PPID from the /proc
            sscanf(line, "%*s %d", &parentProcessID); //getting the PPID number only without the string portion. ex: PPID: 123456
            break; //getting out of the loop once the parentID is found.
        }
    }

    fclose(file); //closing the file

    return parentProcessID; //returning the parentID
}

// Function to send a signal to a process
int sendSignalToProcess(int processID, int signal) {
    if (kill(processID, signal) == -1) { //upon failure, the KILL function will return -1
        perror("Error: Failed to send signal");
        return -1;
    }
    return 0; //it means the signal was sucessfully sent!
}

// Function to list all non-direct descendant PIDs of a process
void listNondirectDescendants(int processID, int rootID, int direct) {
    int maxPID = getMaxPID(); // gettign the maximum allocated PID for the system
    if (maxPID == -1) {
        printf("Error: Failed to get maximum PID value\n");
        return;
    }

    //checking every process of the system.
    for (int pid = 1; pid <= maxPID; pid++) {
        int parentPID = getParentProcessID(pid); //for every process from1 till max number process of the system, getting the parentID of the process.
        if (parentPID == processID) { // Check if the parentID of the current process is equal to the specified processID which indicates the immediate child of the processID.
            if (!direct) { //here check which descendants should be printed, direct or non-direct
                printf("%d\n", pid); // Print the PID if that process is a non-direct descendent
            }
            listNondirectDescendants(pid, rootID, 0); //then calling itself again to check if the process if direct or non-direct.
        }
    }
}

// Function to list all immediate descendants (children) of a process
void listImmediateDescendants(int processID) {
    char childrenPath[256];
    FILE *file;
    int childID;

    // Constructing the path for getting the CHILDID
    sprintf(childrenPath, "/proc/%d/task/%d/children", processID, processID);

    // Open the children file
    file = fopen(childrenPath, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Failed to open children file for process %d\n", processID);
        return;
    }

    // It will extract childID from the file and keep printing the child processID.
    printf("Immediate descendants of process %d:\n", processID);
    while (fscanf(file, "%d", &childID) == 1) {
        printf("%d\n", childID);
    }

    // Close the children file
    fclose(file);
}

// Function to list all sibling processes of a process
void listSiblingProcesses(int processID) {
    int parentID = getParentProcessID(processID); //getting the parentProcessID of the process

    if (parentID == -1) {
        // printf("Error: Failed to get parent process ID for process %d\n", processID);
        return;
    }

    int maxPID = getMaxPID(); // Getting the maximum number PID allocated for the system!
    if (maxPID == -1) {
        printf("Error: Failed to get maximum PID value\n");
        return;
    }

    //iterating through all the processes of the system to find the sibling of the process
    for (int pid = 1; pid <= maxPID; pid++) {
        int parentPID = getParentProcessID(pid); //getting the PPID for the specified processID.
        //Checking if the parentID of each processes from 1 till maxnumberofID is matched with the PPID of the specified process and the processID from the loop does not match the specified processID.
        if (parentPID == parentID && pid != processID) {
            printf("%d\n", pid); //Printing the Sibling PID of the specified PID.
        }
    }
}

//list PID of all the descendents which are defunct
void listDefunctDescendants(int processID) {
    char childrenPath[256];
    FILE *file;

    // Constructing the path for getting the CHILDID
    sprintf(childrenPath, "/proc/%d/task/%d/children", processID, processID);

    // Open the children file
    file = fopen(childrenPath, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Failed to open children file for process %d\n", processID);
        return;
    }

    // read childID from the file.
    int childID;
    while (fscanf(file, "%d", &childID) == 1) {
        // calling itself to check if the child is defunct or not
        listDefunctDescendants(childID);
        //checking if the current process is defunct or not by checking with the /proc/status
        char statusPath[256];
        sprintf(statusPath, "/proc/%d/status", childID);
        FILE *statusFile = fopen(statusPath, "r");
        if (statusFile == NULL) {
            fprintf(stderr, "Error: Failed to open status file for process %d\n", childID);
            continue;
        }
        char line[256];
        while (fgets(line, sizeof(line), statusFile) != NULL) { //it will get each line of the file until it reaches the eof. Upon reaching eof it will return NULL.
            if (strncmp(line, "State:", 6) == 0 && strstr(line, "Z")) { //extracting the portion after State and then checking the status of the process. If, defunct then it will print Z.
                printf("%d\n", childID); // Print the PID
                break; // Get out of the loop once the PID is found
            }
        }
        fclose(statusFile);
    }

    fclose(file);
}

// Function to list the grandchildren of a given process ID
void listGrandchildren(int processID) {
    char childrenPath[256];
    char grandChildrenPath[256];
    FILE *file,*file2;
    int childID,grandChildrenID;

    // // Constructing the path for getting the CHILDID
    sprintf(childrenPath, "/proc/%d/task/%d/children", processID, processID);

    // Open the children file
    file = fopen(childrenPath, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Failed to open children file for process %d\n", processID);
        return;
    }

    //read CHILD ID and then it gets the PID of each immediate child.
    printf("GrandChilren of process %d:\n", processID);
    while (fscanf(file, "%d", &childID) == 1) {
        sprintf(grandChildrenPath, "/proc/%d/task/%d/children", childID, childID); //to get the CHILDID of each process
        file2 = fopen(grandChildrenPath, "r"); //opening the grandChildren file.
        while (fscanf(file2, "%d", &grandChildrenID) == 1){ 
            printf("%d\n", grandChildrenID); //printing the GrandCHildren ID
        }
    }

    //closing the GrandChildren file
    fclose(file2);
    // Close the children file
    fclose(file);
}

// Function to check if a process is defunct
int isProcessDefunct(int processID) {
    char statusPath[256];
    FILE *file;
    char status[256];

    //constructing the path to check if the processID is defunct
    sprintf(statusPath, "/proc/%d/status", processID);

    file = fopen(statusPath, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Failed to open status file for process %d\n", processID);
        return -1;
    }

    //reading until the eof 
    while (fgets(status, sizeof(status), file) != NULL) {
        if (strncmp(status, "State:", 6) == 0) { //extracting the portion after STATE
            if (strstr(status, "Z")) { //checking if 'Z' is present in the STATE
                fclose(file); //closing the current file
                return 1; // Process is defunct
            }
            break; //getting out of loop.
        }
    }

    //closing the file
    fclose(file);
    return 0; // Process is not defunct
}

int main(int argc, char *argv[]) {
    //checking so that the user can't give more or less arguments
    if (argc < 3 || argc > 4) {
        printf("Invalid input! \nCorrect input format: %s [process_id] [root_process_id] \nCorrect input format: %s [process_id] [root_process_id] [option]\n", argv[0], argv[0]);
        return 0;
    }

    int processIdToSearch = atoi(argv[1]); //storing the processID from the command-line argument
    int rootProcessID = atoi(argv[2]); //storing the rootProcess ID 

    if (argc == 3) { //if the user only gives only 3 arguments that means the user want to check only if the given processID is under the speicified rootProcessID.
        int parentID = getParentProcessID(processIdToSearch); //getting the PPID of the process.
        if (parentID == rootProcessID) { //checking if the parentID matchen with the user given rootProcessID
            printf("%d %d\n", processIdToSearch,rootProcessID);
        } else {
            printf("Process %d does not belong to the process tree rooted at %d.\n", processIdToSearch, rootProcessID);
        }
    } else if (argc == 4) {

        //that means the user want to perform multiple actions based on options.
        char *option = argv[3];
        if (strcmp(option, "-rp") == 0) {
            // -rp: Kills the process with the specified process ID if it belongs to the process tree rooted at the root process ID.
            int parentID = getParentProcessID(processIdToSearch);
            if (parentID == rootProcessID) {
                if(kill(processIdToSearch, SIGKILL)==0){
                    printf("Process %d killed.\n", processIdToSearch);
                } else{
                    printf("Process %d does not belong to the process tree rooted at %d.\n", processIdToSearch, rootProcessID);
                }
            } else {
                printf("Process %d does not belong to the process tree rooted at %d.\n", processIdToSearch, rootProcessID);
            }
        } else if (strcmp(option, "-pr") == 0) {
            // -pr: Kills the root process (if it is valid).
            int parentID = getParentProcessID(processIdToSearch);
            if(parentID==rootProcessID){
                if(kill(rootProcessID, SIGKILL)==0){
                printf("Root process %d killed.\n", rootProcessID);
                } else{
                    printf("Process %d does not belong to the process tree rooted at %d.\n", processIdToSearch, rootProcessID);
                }
            } else{
                printf("Process %d does not belong to the process tree rooted at %d.\n", processIdToSearch, rootProcessID);
            }
        } else if (strcmp(option, "-xn") == 0) {
            // -xn: Lists the process IDs of all the non-direct descendants of the specified process ID.
            int parentID=getParentProcessID(processIdToSearch);
            if(parentID==rootProcessID){
                printf("The non-direct descendents of PID %d are:\n",processIdToSearch);
                listNondirectDescendants(processIdToSearch,processIdToSearch,1);
            } else{
                printf("No non-direct descendents!");
            }
        } else if (strcmp(option, "-xd") == 0) {
            // -xd: Lists the process IDs of all the immediate descendants (children) of the specified process ID.
            int parentID = getParentProcessID(processIdToSearch);
            if(parentID==rootProcessID){
                listImmediateDescendants(processIdToSearch);
            } else{
                printf("No direct descendants!");
            }
        } else if (strcmp(option, "-xs") == 0) {
            // -xs: Lists the process IDs of all the sibling processes of the specified process ID.
            int parentID = getParentProcessID(processIdToSearch);
            if(parentID==rootProcessID){
                printf("Sibling process of PID: %d are as follows!\n",processIdToSearch);
                listSiblingProcesses(processIdToSearch);
            } else{
                printf("Process %d does not belong to the process tree rooted at %d.\n", processIdToSearch, rootProcessID);
            }
        } else if (strcmp(option, "-xt") == 0) {
            // -xt: Pauses the specified process ID with the SIGSTOP signal.
            int parentID = getParentProcessID(processIdToSearch);
            if(parentID==rootProcessID){
                if(sendSignalToProcess(processIdToSearch, SIGSTOP)==0){
                    printf("Process %d paused.\n", processIdToSearch);
                } else{
                    printf("Process %d does not belong to the process tree rooted at %d.\n", processIdToSearch, rootProcessID);
                }
            } else{
                printf("Process %d does not belong to the process tree rooted at %d.\n", processIdToSearch, rootProcessID);
            }
        } else if (strcmp(option, "-xc") == 0) {
            // -xc: Sends the SIGCONT signal to all processes that have been paused earlier.
            int parentID = getParentProcessID(processIdToSearch);
            if(parentID==rootProcessID){
                if(sendSignalToProcess(processIdToSearch, SIGCONT)==0){
                    printf("SIGCONT signal sent to process %d.\n", processIdToSearch);
                } else{
                    printf("Process %d does not belong to the process tree rooted at %d.\n", processIdToSearch, rootProcessID);
                }
            } else{
                printf("Process %d does not belong to the process tree rooted at %d.\n", processIdToSearch, rootProcessID);
            }
        } else if (strcmp(option, "-xz") == 0) {
            // -xz: Lists the process IDs of all descendants of the specified process ID that are defunct.
            int parentID=getParentProcessID(processIdToSearch);
            if(parentID==rootProcessID){
                listDefunctDescendants(processIdToSearch);
            } else{
                printf("Process %d does not belong to the process tree rooted at %d.\n", processIdToSearch, rootProcessID);
            }
        } else if (strcmp(option, "-xg") == 0) {
            // -xg: Lists the process IDs of all the grandchildren of the specified process ID.
            int parentID=getParentProcessID(processIdToSearch);
            if(parentID==rootProcessID){
                listGrandchildren(processIdToSearch);
            } else{
                printf("No grandchildren!");
            }
        } else if (strcmp(option, "-zs") == 0) {
            // -zs: Prints the status of the specified process ID (Defunct/Not Defunct).
            if (isProcessDefunct(processIdToSearch)) {
                printf("Process %d is Defunct.\n", processIdToSearch);
            } else {
                printf("Process %d is Not Defunct.\n", processIdToSearch);
            }
        } else {
            printf("Invalid Input!\n");
        }
    }

    return 0;
}
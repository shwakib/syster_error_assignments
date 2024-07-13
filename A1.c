#define _XOPEN_SOURCE 500 // Required for nftw
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>

char* fileNameOrExtension; //to store the file name or extension to search
int taskStatus=0; //to check the status of the program
char* systemCommand; //to store the system command to run
char* storageDir; //to store the storage directory

//this function will check the option given by the user and return the desired option
char* checkOptions(char* options){
    if(strcmp(options,"-cp")==0){
        return "cp";
    }else if(strcmp(options,"-mv")==0){
        return "mv";
    }else {
        printf("Incorrect options.\n");
        exit(1);
    }
}

//this will search for the file with the same extension given by the user
int getFilesWithSameExt(const char *filepath, const char *ext) {
    size_t fpath_len = strlen(filepath); //getting the length of the filepath
    size_t ext_len = strlen(ext); //getting the length of the extension

    //if the length of the filepath is less than the extension then it will return 0 which means there is a mismatch.
    if (fpath_len < ext_len) {
        return 0;
    }
    return strcmp(filepath + fpath_len - ext_len, ext); //otherwise it will seperate the portion of the extension and then it will compare with the extension. if the extension matches then it will return a 0 otherwise, it will return 1.
}

//it will traverse all the directory in the specified directory by the user and show the expected file path
int displayInfo(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf){
    if(typeflag==FTW_F){ //checking the traversed directory is a regular file or not.
        if(strstr(fpath,fileNameOrExtension)){ //it will search for the filename given by the user in the filepath.
            printf("Found at: %s \n",fpath); //If the file is found in any filepath then it will return the filepath.
            taskStatus=1; //then it will set the taskStatus to 1 which will indicate the completion of the file search.
        }
    }
    return 0; //otherwise it will return zero and keep traversing
}

//it will search for the specific file given by the user and copy or move based on the options given by the user
int findAndCopyOrmoveFile(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf){
    if(typeflag==FTW_F){ //checking the traversed directory is a regular file or not.
        if(strstr(fpath,fileNameOrExtension)){ //it will search for the filename given by the user in the filepath.
            int newCommandSize=strlen(systemCommand)+strlen(fpath)+strlen(storageDir)+3; //identifying the memory allocation needed for the systemCommand with all the new strings. Here 3 has been added because there will be 2 spaces in between the fpath and storageDir and one space for the null terminator.
            char* newMemorySizeAllocation=realloc(systemCommand,newCommandSize*sizeof(char)); //Now reallocating the new identified space to the systemCommand.
            if(newMemorySizeAllocation==NULL){ //if the memory allocation can't be done and returns NULL then it will return an error message & exit.
                printf("New Memory Allocation Error!");
                exit(1);
            }

            systemCommand=newMemorySizeAllocation; //setting the pointer of systemCommand to poin at the newly allocated memory location.
            strcat(systemCommand," "); //adding a space ex: cp <space>
            strcat(systemCommand,fpath); //adding the filepath. ex: cp <space> /home/wakib/Desktop/Labs/Check.txt
            strcat(systemCommand," "); //adding a space.
            strcat(systemCommand,storageDir); //adding the adding the storageDirectory where the file will be copied. ex: cp <space> /home/wakib/Desktop/Labs/Check.txt /home/wakib/Desktop/Practices
            
            taskStatus=1; //setting the taskStatus to 1;
        }
    }
    return 0; //otherwise it will return zero and keep traversing
}

//it will search for the files with the specific extension given by the user and make a tar file with all those files in the specified location given by the user
int makeTarFile(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf){
    if(typeflag==FTW_F){ //checking the traversed directory is a regular file or not.
        if(getFilesWithSameExt(fpath, fileNameOrExtension) == 0){ //if the getFilesWithSameExt returns 0 then it will proceed with the same extension
            int newMemorySize=(strlen(systemCommand)+strlen(fpath)+2); //identifying the memory allocation needed for the systemCommand with all the new strings. Here 2 has been added because there will be 1 space in between the fpath and systemCommand and one space for the null terminator.
            char* newMemorySizeAllocation=realloc(systemCommand,newMemorySize*sizeof(char)); //Now reallocating the new identified space to the systemCommand.
            if(newMemorySizeAllocation==NULL){ //if the memory allocation can't be done and returns NULL then it will return an error message & exit.
                printf("New Memory Allocation Error!");
                exit(1);
            }

            systemCommand=newMemorySizeAllocation; //setting the pointer of systemCommand to poin at the newly allocated memory location.
            strcat(systemCommand," "); //adding a space.
            strcat(systemCommand,fpath); //adding the filepath to the existing systemCommand. ex: tar -cf <space> /home/wakib/Desktop/a1.tar /home/wakib/Desktop/Labs/check.txt /home/wakib/Desktop/Labs/check1.txt /home/wakib/Desktop/Practices/check.txt
        }
    }
    if(getFilesWithSameExt(systemCommand, fileNameOrExtension) == 0) { //then this function will all the file with the user specified extension has been picked or not in the systemCommand.
        taskStatus = 1; //if the function returns 0 then it indicates success.
    }
    return 0; //otherwise it will return zero and keep traversing
}

int main(int argc,char *argv[]){

//It will check the number of arguments the user is giving. If the number if arguments is less than 3 or more than  the program will show an error message.
    if(argc<3 || argc>5){
        printf("Wrong Input! \n");
        printf("1. fileutil [root_dir] filename to search file. \n");
        printf("2. fileutil [root_dir] [storage_dir] filename [options] to search & copy/move a file. \n");
        printf("3. fileutil [root_dir] [storage_dir] [extension]. \n");
        exit(1);
    }

//this is the first condition where the user will search for a file with the filename. Ex: check.txt will be searched in all the directories and all the occurance of check.txt will be printed
    if(argc==3){
        fileNameOrExtension=argv[2]; //storing the filename to be searched for.
        nftw(argv[1],displayInfo,30,FTW_PHYS | FTW_DEPTH); //nftw function where, the first argument is the directory where the files will be searched; displayInfo is the function name which be called everytime the nftw traverse a file;30 is the number of file descriptor and FTW_PHYS is for specifying so that the nftw does not follow any symbolic links.

        //IF THE taskStatus return 0 then it means the file could not be searched and then it will return an error message.
        if(taskStatus==0){
            printf("Search Unsuccessful!\n");
            exit(EXIT_FAILURE);
        }
    }

//If the user chooses to copy or move the given file then it will search for the file in the specified directory and copy/move based on the user given option to the user preferred directory
    if(argc==5){
        char* whichCommand=checkOptions(argv[3]); //it will check the options given by the user.
        fileNameOrExtension=argv[4]; //store the filename needs to be searched & copy/move.

        storageDir=argv[2]; //storing the storage directory.

        int commandSize=strlen(whichCommand); //identifying memory size for the systemCommand.
        systemCommand=malloc(commandSize); //assigning memory for the systemCommand.

        //if the memory allocation is not successful then it will show an error message.
        if(systemCommand==NULL){
            exit(1);
        }

        strcpy(systemCommand,whichCommand); //copying the user provided command to the systemCommand. ex: cp or mv

        nftw(argv[1],findAndCopyOrmoveFile,100,FTW_PHYS | FTW_DEPTH); //nftw function where, the first argument is the directory where the files will be searched; findAndCopyOrmoveFile is the function name which be called everytime the nftw traverse a file;100 is the number of file descriptor and FTW_PHYS is for specifying so that the nftw does not follow any symbolic links.

        //if the taskStatus returns 1 then it will run the systemCommand and show an message identifying the task has been completed and then free the allocated memory for the systemCommand.
        if(taskStatus==1){
            system(systemCommand);
            printf("Search Successful & file has been processed!\n");
            free(systemCommand);
            exit(0);
        }

        //If the program can't find any file then it will show an error message.
        printf("Search Unsuccessful! \n");
        exit(0);
    }

//Here, all the files with the user provided extension will be searched and make a tar file in the user defined location
    if(argc==4){
        char* commandForTar="tar -cf"; //system command to make tar file
        char* destFilePath="/a1.tar"; //the file name in the specified location


        int commandSize=strlen(commandForTar)+1+strlen(argv[2])+strlen(destFilePath)*sizeof(char); //to provide dynamic memory allocation to the system command so that it gets the appropriate storage everytime and does not utilize extra memory
        systemCommand=malloc(commandSize); //assigning the location to the systemCommand.

	//if the system can not allocate memory to the system command then it will show an error
        if(systemCommand==NULL){
            printf("Error allocating memory!");
            exit(1);
        }

        strcpy(systemCommand,commandForTar); //copying the  command for tar to the systemCommand
        strcat(systemCommand," "); //adding the space ex: tar -cf <space>
        strcat(systemCommand,argv[2]); //adding the storage dir ex: tar -cf <space> /home/wakib/Desktop
        strcat(systemCommand,destFilePath); //adding the tar file name with the storage dir ex: tar -cf <space> /home/wakib/Desktop/a1.tar

        fileNameOrExtension=argv[3]; //storing the extension given by the user.
        nftw(argv[1],makeTarFile,30,FTW_PHYS | FTW_DEPTH); //nftw function where, the first argument is the directory where the files will be searched; makeTarfile is the function name which be called everytime the nftw traverse a file;30 is the number of file descriptor and FTW_PHYS is for specifying so that the nftw does not follow any symbolic links.

	    //if the task is complete then the program will run the system command for making the tar file and exit
        if(taskStatus==1){
            struct stat fileInformation; //fileInformation is a stat type variable which holds all the information about the file.
            if (stat(argv[2], &fileInformation) == -1) { //checks if the directory given by the user exists or not! If not, then it will return -1
                if (mkdir(argv[2], 0777) == -1) { //if the directory is not present, then it will be created with 0777 permission, if can not then it will return "-1" and print the error message of mkdir.
                perror("mkdir");
                exit(EXIT_FAILURE);
                }
            printf("Directory created successfully.\n");
            } else {
            printf("Directory already exists.\n");
            }
            system(systemCommand);
	printf("Tar file has been created successfully! \n");
            free(systemCommand);
            exit(0);
        }

	//if the files could not be searched or taskStatus can't be 1 then it will return an error message.
        printf("Search Unsuccessful! \n");
        exit(0);
    }
}

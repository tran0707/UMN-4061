/* CSci4061 F2018 Assignment 1
* login: jungx457@umn.edu
* date: 10/05/18
* name: Chan Jung, Khoa Tran, Sara Nelson
* id: 5108030(jungx457),5411431(tran0707),5163126(nels8907) */

// This is the main file for the code
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "util.h"

/*-------------------------------------------------------HELPER FUNCTIONS PROTOTYPES---------------------------------*/
void show_error_message(char * ExecName);
//Write your functions prototypes here
void show_targets(target_t targets[], int nTargetCount);
/*-------------------------------------------------------END OF HELPER FUNCTIONS PROTOTYPES--------------------------*/


/*-------------------------------------------------------HELPER FUNCTIONS--------------------------------------------*/

//This is the function for writing an error to the stream
//It prints the same message in all the cases
void show_error_message(char * ExecName)
{
	fprintf(stderr, "Usage: %s [options] [target] : only single target is allowed.\n", ExecName);
	fprintf(stderr, "-f FILE\t\tRead FILE as a makefile.\n");
	fprintf(stderr, "-h\t\tPrint this message and exit.\n");
	exit(0);
}

//Write your functions here
//Status = 0 -> need tp build ||| Status = 1 -> don't build. 
void my_make(char *targetName, target_t targets[], int nTargetCount){
    int findTarget = find_target(targetName, targets , nTargetCount);
    int depCount = targets[findTarget].DependencyCount;
    for (int i = 0; i < depCount; i++){      
        char *depName = targets[findTarget].DependencyNames[i];
        //recursive call for my_make to change status of targets if needed and fork, exec on targets with status equal to 0.
        my_make(depName, targets, nTargetCount);
        //use compare_modification_time function to compare the time different between targets and its dependencies.
        int compareTime = compare_modification_time (targetName,depName);
        //modify status to 0 or 1 based on the return result of compare_modification_time function.
        if (compareTime == -1){             //target doesn't exist => Status = 0.
            targets[findTarget].Status = 0;
        }
        if (compareTime == 0){              //target and dependency have the same modification time => Status = 1.
            targets[findTarget].Status = 1;
        }
        if (compareTime == 1){              //target modified later than dependency => Status = 1.
            targets[findTarget].Status = 1;
        }
        if (compareTime == 2){              //dependency modified later than its target => Status = o.
            targets[findTarget].Status = 0;      
        }
    }
    // when a target has status equal to 0 => fork, exec, and wait.
    if (targets[findTarget].Status==0){
        int wait_child_status = 0;
	    int	pid = fork();
		if(pid < 0){                //fork error
			perror("Fork Error");
            exit(-1);
		}
        else if (pid == 0){         //child
	        printf("%s\n",targets[findTarget].Command);
            char target_command[256];
            strcpy(target_command,targets[findTarget].Command);
            char *args[150];
            parse_into_tokens(target_command,args," ");
			execvp (args[0],args);
            perror("Exec Failed");
            exit(-2);
	    }
        else{                       //parent wait for child to complete
            wait(&wait_child_status);
            if(WEXITSTATUS(wait_child_status) != 0){
                printf("Makefile: recipe for target '%s' failed \n make: *** [%s] Error  %d\n",targets[findTarget].TargetName,
                targets[findTarget].TargetName,WEXITSTATUS(wait_child_status));
                exit(-3);
            }
		}
    }
}
/*
//Phase1: Warmup phase for parsing the structure here. Do it as per the PDF (Writeup)
//Warmup checked off on 9/26/18
//void show_targets(target_t targets[], int nTargetCount)
{
	int i=0;
	//check for error
	if(nTargetCount<0){
		printf("error");
	}
	//For all targets, print out the target name, dependency count, dependency names, and command
	for(i=0, i<nTargetCount, i++)
	{
	printf("target name = %s, dependencycount= %d, dependencynames = %s,
    command = %s | ",targets[i].TargetName, targets[i].DependencyCount,
    targets[i].DependencyNames, targets[i].Command );
	}
}*/


/*-------------------------------------------------------END OF HELPER FUNCTIONS-------------------------------------*/


/*-------------------------------------------------------MAIN PROGRAM------------------------------------------------*/
//Main commencement
int main(int argc, char *argv[])
{
  target_t targets[MAX_NODES];
  int nTargetCount = 0;

  /* Variables you'll want to use */
  char Makefile[64] = "Makefile";
  char TargetName[64];

  /* Declarations for getopt. For better understanding of the function use the man command i.e. "man getopt" */
  extern int optind;   		// It is the index of the next element of the argv[] that is going to be processed
  extern char * optarg;		// It points to the option argument
  int ch;
  char *format = "f:h";
  char *temp;

  //Getopt function is used to access the command line arguments. However there can be arguments which may or may not need the parameters after the command
  //Example -f <filename> needs a finename, and therefore we need to give a colon after that sort of argument
  //Ex. f: for h there won't be any argument hence we are not going to do the same for h, hence "f:h"
  while((ch = getopt(argc, argv, format)) != -1)
  {
	  switch(ch)
	  {
	  	  case 'f':
	  		  temp = strdup(optarg);
	  		  strcpy(Makefile, temp);  // here the strdup returns a string and that is later copied using the strcpy
	  		  free(temp);	//need to manually free the pointer
	  		  break;

	  	  case 'h':
	  	  default:
	  		  show_error_message(argv[0]);
	  		  exit(1);
	  }

  }

  argc -= optind;
  if(argc > 1)   //Means that we are giving more than 1 target which is not accepted
  {
	  show_error_message(argv[0]);
	  return -1;   //This line is not needed
  }

  /* Init Targets */
  memset(targets, 0, sizeof(targets));   //initialize all the nodes first, just to avoid the valgrind checks

  /* Parse graph file or die, This is the main function to perform the toplogical sort and hence populate the structure */
  if((nTargetCount = parse(Makefile, targets)) == -1)  //here the parser returns the starting address of the array of the structure. Here we gave the makefile and then it just does the parsing of the makefile and then it has created array of the nodes
	  return -1;


  //Phase1: Warmup-----------------------------------------------------------------------------------------------------
  //Parse the structure elements and print them as mentioned in the Project Writeup
  /* Comment out the following line before Phase2 */
  //show_targets(targets, nTargetCount);  
  //End of Warmup------------------------------------------------------------------------------------------------------
   
  /*
   * Set Targetname
   * If target is not set, set it to default (first target from makefile)
   */
  if(argc == 1)
	strcpy(TargetName, argv[optind]);    // here we have the given target, this acts as a method to begin the building
  else
  	strcpy(TargetName, targets[0].TargetName);  // default part is the first target

  /*
   * Now, the file has been parsed and the targets have been named.
   * You'll now want to check all dependencies (whether they are
   * available targets or files) and then execute the target that
   * was specified on the command line, along with their dependencies,
   * etc. Else if no target is mentioned then build the first target
   * found in Makefile.
   */
	
  //Phase2: Begins ----------------------------------------------------------------------------------------------------
  /*Your code begins here*/
 
  if(does_file_exist(Makefile) == -1){      //check if Makefile exist. If not, print error messsage.
      printf("make4061: *** No targets specfied and no makefile found.  Stop.");
  }
  my_make(TargetName,targets,nTargetCount); //call my_make in main
  int target_index = find_target(TargetName,targets,nTargetCount);
  if(targets[0].Status == 1){               //the first target has Status 1 => already compiled => print message. 
      printf("make4061: '%s' is up to date\n", targets[0].TargetName);
  }
  else if (targets[target_index].Status == 1){ //./make4061 X => if X already compiled => print up to date message. 
      printf("make4061: '%s' is up to date.\n",targets[target_index].TargetName);
  }
  if(find_target(TargetName,targets,nTargetCount)== -1){ //it can't find the target, print error message.
      printf("make4061: *** No rule to make target '%s'. Stop.\n",TargetName);
  }
  
  
  /*End of your code*/
  //End of Phase2------------------------------------------------------------------------------------------------------

  return 0;
}
/*-------------------------------------------------------END OF MAIN PROGRAM------------------------------------------*/

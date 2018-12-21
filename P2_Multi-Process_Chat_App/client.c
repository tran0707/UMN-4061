#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include "comm.h"
#include "util.h"


/* -------------------------Main function for the client ----------------------*/
void main(int argc, char * argv[]) {

	int pipe_user_reading_from_server[2], pipe_user_writing_to_server[2];
	// You will need to get user name as a parameter, argv[1].

	if(connect_to_server("SERVER_ID_UMN", argv[1], pipe_user_reading_from_server, pipe_user_writing_to_server) == -1) {
		exit(-1);
	}
    print_prompt(argv[1]);
    // print pipes 
    printf("pipe to read %d\n", pipe_user_reading_from_server[0]);
    printf("pipe to write %d\n", pipe_user_writing_to_server[1]);
    //non_blocking pipes
    fcntl(pipe_user_reading_from_server[0], F_SETFL, fcntl(pipe_user_reading_from_server[0], F_GETFL)| O_NONBLOCK);
    fcntl(pipe_user_reading_from_server[1], F_SETFL, fcntl(pipe_user_reading_from_server[1], F_GETFL)| O_NONBLOCK);
	fcntl(pipe_user_writing_to_server[0], F_SETFL, fcntl(pipe_user_writing_to_server[0], F_GETFL)| O_NONBLOCK);
	fcntl(pipe_user_writing_to_server[1], F_SETFL, fcntl(pipe_user_writing_to_server[1], F_GETFL)| O_NONBLOCK);
	fcntl(0, F_SETFL, fcntl(0, F_GETFL)| O_NONBLOCK);
    //message declare
    char message[MAX_MSG];
    char message_from_server[MAX_MSG];

	/* -------------- YOUR CODE STARTS HERE -----------------------------------*/
    //closing non_use pipes
    close(pipe_user_writing_to_server[0]);
    close(pipe_user_reading_from_server[1]);

    while(1){
	 /********************** POLL STDIN ***********************/
        //reset message
        char buf_error [MAX_MSG];
        memset(buf_error,0,MAX_MSG);
        memset(message,0,MAX_MSG);
        int input = read(0,message,MAX_MSG);
        strcpy(buf_error, message);
        if (input < 0 &&  errno == EAGAIN){
            //keep polling
        } 
        else if (input != 0){
            if(strcmp(strtok(buf_error, " "), "\n") == 0){
                //if user input is " \n" => skip it.
            }
            else if(write(pipe_user_writing_to_server[1], message, strlen(message)+1) == -1){
                fprintf(stderr, "Failed to write to Server\n");
                return;
            }
            print_prompt(argv[1]);
        }
        else{
            printf("Failed to read from stdin\n");
            exit(-1);
        }

	/************************* POLL ON CHILD *******************/
        //reset message_from_server
        memset(message_from_server,0,MAX_MSG);
        int nread = read(pipe_user_reading_from_server[0],message_from_server,MAX_MSG);
        if(nread < 0 && errno == EAGAIN){
            //keep polling
        }
        else if (nread != 0){
            printf("%s\n",message_from_server);
            print_prompt(argv[1]);
        }
        else if(nread == 0){
            printf("The user %s is killed.\n", argv[1]);
            exit(0);
        }
    }
    usleep(1000);
	/* -------------- YOUR CODE ENDS HERE -----------------------------------*/
}

/*--------------------------End of main for the client --------------------------*/



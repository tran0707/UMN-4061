#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include "comm.h"
#include "util.h"

/* -----------Functions that implement server functionality -------------------------*/

/*
 * Returns the empty slot on success, or -1 on failure
 */
int find_empty_slot(USER * user_list) {
	// iterate through the user_list and check m_status to see if any slot is EMPTY
	// return the index of the empty slot
    int i = 0;
	for(i=0;i<MAX_USER;i++) {
    	if(user_list[i].m_status == SLOT_EMPTY) {
			return i;
		}
	}
	return -1;
}

/*
 * list the existing users on the server shell
 */
int list_users(int idx, USER * user_list)
{
	// iterate through the user list
	// if you find any slot which is not empty, print that m_user_id
	// if every slot is empty, print "<no users>""
	// If the function is called by the server (that is, idx is -1), then printf the list
	// If the function is called by the user, then send the list to the user using ()
    // and passing m_fd_to_user
	// return 0 on success
	int i, flag = 0;
	char buf[MAX_MSG] = {}, *s = NULL;

	/* construct a list of user names */
	s = buf;
	strncpy(s, "---connecetd user list---\n", strlen("---connecetd user list---\n"));
	s += strlen("---connecetd user list---\n");
	for (i = 0; i < MAX_USER; i++) {
		if (user_list[i].m_status == SLOT_EMPTY)
			continue;
		flag = 1;
		strncpy(s, user_list[i].m_user_id, strlen(user_list[i].m_user_id));
		s = s + strlen(user_list[i].m_user_id);
		strncpy(s, "\n", 1);
		s++;
	}
	if (flag == 0) {
		strcpy(buf, "<no users>\n");
	} else {
		s--;
		strncpy(s, "\0", 1);
	}

	if(idx < 0) {
		printf("%s",buf);
		printf("\n");
	} else {
		/*  to the given pipe fd */
		if (write(user_list[idx].m_fd_to_user, buf, strlen(buf) + 1) < 0)
			perror("writing to server shell");
	}

	return 0;
}

/*
 * add a new user
 */
int add_user(int idx, USER * user_list, int pid, char * user_id, int fd_pipe_to_child, int fd_pipe_to_parent)
{
	// populate the user_list structure with the arguments passed to this function
	// return the index of user added
    USER addUser = {};
    addUser.m_pid = pid;
    strcpy(addUser.m_user_id, user_id);
    addUser.m_fd_to_user = fd_pipe_to_child;
    addUser.m_fd_to_server = fd_pipe_to_parent;
    addUser.m_status = SLOT_FULL;
    user_list[idx] = addUser;
    return idx;
}

/*
 * Kill a user
 */
void kill_user(int idx, USER * user_list) {
	// kill a user (specified by idx) by using the systemcall kill()
	// then call waitpid on the use
    int wstatus;
    int child_pid = user_list[idx].m_pid;
    int kill_status = kill(child_pid, SIGKILL); 
    if(kill_status == -1){
        printf("failed to kill user.\n");
    }
    waitpid(child_pid,&wstatus,WNOHANG);
}
/*
 * Perform cleanup actions after the used has been killed
 */
void cleanup_user(int idx, USER * user_list)
{
	// m_pid should be set back to -1
	// m_user_id should be set to zero, using memset()
	// close all the fd
	// set the value of all fd back to -1
	// set the status back to empty
    user_list[idx].m_pid = -1;
    memset(user_list[idx].m_user_id, '\0', MAX_USER_ID);
    close(user_list[idx].m_fd_to_server);
    close(user_list[idx].m_fd_to_user);
    user_list[idx].m_fd_to_user = -1;
    user_list[idx].m_fd_to_server = -1;
    user_list[idx].m_status = SLOT_EMPTY;
}

/*
 * Kills the user and performs cleanup
 */
void kick_user(int idx, USER * user_list) {
	// should kill_user()
	// then perform cleanup_user()
    kill_user(idx,user_list);
    cleanup_user(idx,user_list);
}

/*
 * broadcast message to all users
 */
int broadcast_msg(USER * user_list, char *buf, char *sender)
{
	//iterate over the user_list and if a slot is full, and the user is not the sender itself,
	//then send the message to that user
	//return zero on success
    int i;
    for (i = 0; i < MAX_USER ; i++){
        if(user_list[i].m_status == SLOT_FULL && (strcmp(user_list[i].m_user_id, sender) != 0)){
            char user_id_msg [MAX_MSG];
            sprintf(user_id_msg, "%s: %s ", sender, buf);
            if(write(user_list[i].m_fd_to_user, user_id_msg, strlen(user_id_msg)+1) < 0){
                printf("failed to send message.\n");
                return -1;
            }
        }
    }
	return 0;
}

/*
 * Cleanup user chat boxes
 */
void cleanup_users(USER * user_list)
{
	// go over the user list and check for any empty slots
	// call cleanup user for each of those users.
    int i;
    for(i = 0; i < MAX_USER; i ++){
        if(find_empty_slot(user_list) != -1){
            cleanup_user(i,user_list);
        }
    }
}

/*
 * find user index for given user name
 */
int find_user_index(USER * user_list, char * user_id)
{
	// go over the  user list to return the index of the user which matches the argument user_id
	// return -1 if not found
	int i, user_idx = -1;

	if (user_id == NULL) {
		fprintf(stderr, "NULL name passed.\n");
		return user_idx;
	}
	for (i=0;i<MAX_USER;i++) {
		if (user_list[i].m_status == SLOT_EMPTY)
			continue;
		if (strcmp(user_list[i].m_user_id, user_id) == 0) {
			return i;
		}
	}

	return -1;
}

/*
 * given a command's input buffer, extract name
 */
int extract_name(char * buf, char * user_name)
{
	char inbuf[MAX_MSG];
    char * tokens[16];
    strcpy(inbuf, buf);

    int token_cnt = parse_line(inbuf, tokens, " ");

    if(token_cnt >= 2) {
        strcpy(user_name, tokens[1]);
        return 0;
    }

    return -1;
}

int extract_text(char *buf, char * text)
{
    char inbuf[MAX_MSG];
    char * tokens[16];
    strcpy(inbuf, buf);

    int token_cnt = parse_line(buf, tokens, " ");

    if(token_cnt >= 3) {
        memset(text,0,token_cnt);
        int i;
        for(i = 2; i < token_cnt; i++){
            strcat(text, tokens[i]);
            strcat(text, " ");
        }
        return 0;
    }

    return -1;
}


/*
 * send personal message
 */
void send_p2p_msg(int idx, USER * user_list, char *buf)
{

    // get the target user by name using extract_name() function
	// find the user id using find_user_index()
	// if user not found, write back to the original user "User not found", using the write()function on pipes. 
	// if the user is found then write the message that the user wants to send to that user.
    
    char  user_name[MAX_USER_ID];
    char text[MAX_MSG];
    if(extract_name(buf,user_name) == -1) { 
        char *mess = "Need The Second and Third Input For User Name and Text Message.";
        if(write(user_list[idx].m_fd_to_user, mess,strlen(mess)+1) <0){
            printf("failed to write to %s.\n", user_name);
            return;
        }
    }
    extract_text(buf,text);
    int index_user;
    index_user = find_user_index(user_list, user_name); 
    if(index_user == -1){
        char *mess = "User not found.";
        if(write(user_list[idx].m_fd_to_user, mess,strlen(mess)+1) <0){
            printf("failed to write to %s.\n", user_name);
            return;
        }
    }
    else{
        if(idx == index_user){
            char *same_user = "Can't Send Messsage to Yourself.";
            if(write(user_list[idx].m_fd_to_user, same_user,strlen(same_user)+1) <0){
                printf("failed to write to %s \n", user_name);
                return;
            }
        }
        else{
            char send_mess [MAX_MSG];
            strcpy(send_mess,user_list[idx].m_user_id);
            strcat(send_mess, ": ");
            strcat(send_mess, text);
            if(write(user_list[index_user].m_fd_to_user, send_mess, strlen(send_mess)+1) <0){
                    printf("failed to write to %s\n", user_list[index_user].m_user_id);
                    return;
            }
            memset(text, 0, strlen(text));
            memset(send_mess,0,strlen(send_mess));
        }
   }
}


void init_user_list(USER * user_list) {

	//iterate over the MAX_USER
	//memset() all m_user_id to zero
	//set all fd to -1
	//set the status to be EMPTY
	int i=0;
	for(i=0;i<MAX_USER;i++) {
		user_list[i].m_pid = -1;
		memset(user_list[i].m_user_id, '\0', MAX_USER_ID);
		user_list[i].m_fd_to_user = -1;
		user_list[i].m_fd_to_server = -1;
		user_list[i].m_status = SLOT_EMPTY;
	}
}

/* ---------------------End of the functions that implementServer functionality -----------------*/

/* ---------------------Start of the Main function ----------------------------------------------*/
int main(int argc, char * argv[]){
    char * YOUR_UNIQUE_ID = "SERVER_ID_UMN";
	setup_connection(YOUR_UNIQUE_ID); // Specifies the connection point as argument.
	USER user_list[MAX_USER];
	init_user_list(user_list);   // Initialize user list

	fcntl(0, F_SETFL, fcntl(0, F_GETFL)| O_NONBLOCK);
	print_prompt("admin");
	int pid;
	while(1) {
		/* ------------------------YOUR CODE FOR MAIN--------------------------------*/
		// Handling a new connection using get_connection
		int pipe_SERVER_reading_from_child[2];
		int pipe_SERVER_writing_to_child[2];

		char user_id[MAX_USER_ID];

    // -------------------------- POLL FOR NEW USER CONNECTIONS. -------------------------- //
        int pipe_child_reading_from_user[2];
        int pipe_child_writing_to_user[2];
        if (get_connection(user_id, pipe_child_writing_to_user, pipe_child_reading_from_user) != -1){
        // Check max user and same user id
        int empty_slot = find_empty_slot(user_list);
        if(empty_slot!= -1){
            if ((pipe(pipe_SERVER_reading_from_child) < 0) || (pipe(pipe_SERVER_writing_to_child) < 0) ) {
                printf("Failed to create pipes for USER: %s.\n", user_id);
				return(-1);
            }

            // Set all pipes non_blocking
            fcntl(pipe_child_reading_from_user[0], F_SETFL, fcntl(pipe_child_reading_from_user[0], F_GETFL)| O_NONBLOCK);
            fcntl(pipe_child_reading_from_user[1], F_SETFL, fcntl(pipe_child_reading_from_user[1], F_GETFL)| O_NONBLOCK);
            fcntl(pipe_child_writing_to_user[0], F_SETFL, fcntl(pipe_child_writing_to_user[0], F_GETFL)| O_NONBLOCK);
            fcntl(pipe_child_writing_to_user[1], F_SETFL, fcntl(pipe_child_writing_to_user[1], F_GETFL)| O_NONBLOCK);
            fcntl(pipe_SERVER_reading_from_child[0], F_SETFL, fcntl(pipe_SERVER_reading_from_child[0], F_GETFL)| O_NONBLOCK);
            fcntl(pipe_SERVER_reading_from_child[1], F_SETFL, fcntl(pipe_SERVER_reading_from_child[1], F_GETFL)| O_NONBLOCK);
            fcntl(pipe_SERVER_writing_to_child[0], F_SETFL, fcntl(pipe_SERVER_writing_to_child[0], F_GETFL)| O_NONBLOCK);
            fcntl(pipe_SERVER_writing_to_child[1], F_SETFL, fcntl(pipe_SERVER_writing_to_child[1], F_GETFL)| O_NONBLOCK);
			
			//fork
            pid = fork(); 
            if (pid < 0){
                printf("fork failed.\n");
                return -1;
            }
            /****************************Child Process**********************************/
            else if (pid == 0){
                char messageServer[MAX_MSG];
                char messageUser[MAX_MSG];
                //closing non_use pipes
                close(pipe_SERVER_writing_to_child[1]);
                close(pipe_SERVER_reading_from_child[0]);
                close(pipe_child_writing_to_user[0]);
                close(pipe_child_reading_from_user[1]);
                
                while (1){
                /************************Poll on SERVER**************************/
                memset(messageServer, 0, sizeof(messageServer));
                int readServer = read(pipe_SERVER_writing_to_child[0], messageServer, MAX_MSG);
                if ((readServer < 0) && (errno == EAGAIN)){
                    //keep polling
                }
                else if (readServer != 0){
                    if (write(pipe_child_writing_to_user[1], messageServer, strlen(messageServer)+1) < 0)
                    {
                        printf("Failed to write to USER: %s\n", user_id);
                        exit(-1);
                    }
                }
                else{
                    printf("Failed to read from Stdin\n");
                    exit(-1);
                }
                /*****************************Poll on USER***************************/
                memset(messageUser, 0, sizeof(messageUser));
                int readUser = read(pipe_child_reading_from_user[0], messageUser, MAX_MSG);
                if ( (readUser < 0) && (errno == EAGAIN) ){
                    //keep polling
                }
                else if (readUser != 0){
                    if (write(pipe_SERVER_reading_from_child[1], messageUser, strlen(messageUser)+1) < 0){
                        printf("Failed to write to SERVER.\n");
                        exit(-1);
                    }
                }
                else{
                    printf("The user %s seems to be terminate.\n", user_id);
                    break;
                }
                usleep(1000);
            }
        }

        /*******************************Parent Process******************************/
        else{
            //close non_use pipes
            close(pipe_SERVER_reading_from_child[1]);
            close(pipe_SERVER_writing_to_child[0]);
            close(pipe_child_writing_to_user[0]);
            close(pipe_child_writing_to_user[1]);
            close(pipe_child_reading_from_user[0]);
            close(pipe_child_reading_from_user[1]);

            //if find_user_index != -1 => user already exist
            if(find_user_index(user_list,user_id) != -1){
                add_user(empty_slot, user_list, pid, user_id, pipe_SERVER_writing_to_child[1], pipe_SERVER_reading_from_child[0]);
                printf("User id: %s already taken.\n", user_id);
                kick_user(empty_slot, user_list);
            }
            else{
                add_user(empty_slot, user_list, pid, user_id, pipe_SERVER_writing_to_child[1], pipe_SERVER_reading_from_child[0]);
                printf("\nNEW USER CONNECTED: %s at slot: %d\n", user_id, empty_slot);
            }
        }
        }
    }
    /*******************************POLL FROM CHILD******************************/
    int i;
    for (i = 0; i<MAX_USER; i++){
		// poll child processes and handle user commands
        if (user_list[i].m_status != SLOT_EMPTY){
          char buffer[MAX_MSG];
          memset(buffer, 0, sizeof(buffer));
          int readChild = read(user_list[i].m_fd_to_server, buffer, MAX_MSG);
          if ((readChild < 0) && (errno == EAGAIN)){
              //keep polling
          }
          else if (readChild != 0){
                ////////////////Handling Command From User//////////////////
                buffer[strlen(buffer) -1] ='\0';
                enum command_type user_command = get_command_type(buffer);
                if(user_command == LIST){
                    if(list_users(i,user_list) != 0){
                        printf("list failed.\n");
                    }
                }
                else if (user_command == P2P){
                    send_p2p_msg(i, user_list, buffer);
                }
                else if (user_command == EXIT){
                    kick_user(i,user_list);
                }
                //else if(user_command == BROADCAST){
                else{
                    broadcast_msg(user_list, buffer, user_list[i].m_user_id);
                }
          }
          else{
                printf("Failed to read from CHILD: %s.\n", user_id);
                exit(-1);
          }
        }
    }
    /******************************POLL STDIN***************************/
    // Poll stdin (input from the terminal) and handle admnistrative command
    char messageStdin[MAX_MSG];
    char message_error[MAX_MSG];
    memset(message_error,0,sizeof(message_error));
    memset(messageStdin, 0, sizeof(messageStdin));
    int readStdin = read(0, messageStdin, MAX_MSG);
    strcpy(message_error, messageStdin);
    if ((readStdin < 0) && (errno == EAGAIN)){
        //keep polling
    }
    else if (readStdin != 0){
        ////////////////Handling Command From STDIN////////////////
       if(strcmp(strtok(message_error," "), "\n") == 0){
            //if server type " \n" => skip it
        }
        else{
            messageStdin[readStdin -1]= '\0';
            enum command_type server_command = get_command_type(messageStdin);
            if(server_command == LIST){
                if(list_users(-1,user_list) != 0){
                    printf("list user failed.\n");
                }
            }
            else if(server_command == KICK){
                char kick_user_id[MAX_USER_ID];
                int extract_result = extract_name(messageStdin, kick_user_id);
                if(extract_result == -1){
                    printf("Can't find the kick user id.\n");
                }
                else{
                    int idx = find_user_index(user_list, kick_user_id);
                    if(idx != -1){
                        kick_user(idx,user_list);
                    }
                    else{
                        printf("Can't find the kick user id.\n");
                    }
                }
            }
            else if(server_command == EXIT){
                int k;
                for(k = 0; k < MAX_USER ; k++){
                    if(user_list[k].m_status != SLOT_EMPTY){
                        kick_user(k,user_list);
                    }
                }
                cleanup_users(user_list);
                exit(0);
            }
            //else if(server_command == BROADCAST){
            else{
                int j;
                for (j = 0; j<MAX_USER; j++){
                    if (user_list[j].m_status != SLOT_EMPTY){
                        char admin [MAX_MSG] = "admin-notice: ";
                        strcat(admin, messageStdin);
                        if (write(user_list[j].m_fd_to_user, admin, strlen(admin)+1) < 0){
                            printf("Failed to write to USER: %s.\n", user_list[j].m_user_id);
                        }
                    }
                }
            }
        }
        print_prompt("admin");
    }
    else{
        printf("Failed to read from STDIN.\n");
        exit(-1);
    }
    usleep(1000);
    }
}

/***********************************End of the main function************************************/

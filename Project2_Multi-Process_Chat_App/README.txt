/*CSci4061 F2018 Assignment 2
*login:  jungx457
*date:  10/05/2018
*name:  Khoa Tran, Chan Jung, Sara Nelson
*id:  tran0707(291547), jungx457(5108030), nels8907(5163126)*/

PURPOSE:
The purpose of this program is to create an local multi-party chat. User’s will be able to see who is in the chat room, direct message any user using \p2p <user> <message> and exit the chat room using \exit or ^c. 

DESCRIPTION:
The program is composed of a server parent process that forks into multiple children that handle each of the users present in the chat room. 
client.c
First connects to server and sets up non-blocking pipes. Then enters an endless look where program continues to poll from Stdin and poll from child process using the nonblocking pipes. Will either write to server in pipe_user_writing_to_server[1] if received something from stdin or will print out message from server if receives message from pipe_user_reading_from_server[0]. Errors are handled accordingly.

server.c
Polls for new connections in an endless while loop, forks when connection is present. Child process sits in its  own while loop, continuously polling form both server and user. Parent process (server) continues to poll for new users, as well as polling from child process and stdin using nonblocking pipes. Helper functions are utilized to \list, \kick <user>, \exit, and broadcast a message. As well as \p2p for user stdin.

For error handling, checked return values of all helper functions called to indicate if error occurred and continue accordingly.
INSTRUCTIONS:
1.Locate folder location
2. Run make 
3. run server first using ./server
4. run as many users as MAX_USER allows using ./client <user id>
5. Utilize \p2p, \exit, and \list to see who is available to chat.

ASSUMPTIONS:
The chat room is only accessible by the use of one-machine. Server.c will be run before client.c
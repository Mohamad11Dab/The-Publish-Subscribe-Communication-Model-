#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>
#include <csse2310a3.h>
#define EXIT_NORMALY 0
#define INVALID_COMMAND_LINES 1
#define INVALID_NAME 2
#define INVALID_TOPIC 5
#define INVALID_CONNECTION 3
#define MINIMUM_ARGS 3
#define DISCONNECTED 4


/*exit_program()
 *---------------
 *
 *exitCode: the exit status which the program exits with in case it tackles
 *any kind of invalid command line arguments
 *
 *This function prints to stderr and exits with an exit status according
 *to the reason why the command line arguments are invalid:
 *if arguemnts are missing exit with 1
 *if name is invalid exit with 2
 *if topic is invalid exit with 2
 */
void exit_program(int exitCode) {

    switch (exitCode) {
        case EXIT_NORMALY:
            break; 
        case INVALID_COMMAND_LINES: 
            fprintf(stderr, "Usage: psclient portnum name [topic] ...\n"); 
            break; 
        case INVALID_NAME: 
            fprintf(stderr, "psclient: invalid name\n"); 
            break; 
        case INVALID_TOPIC: 
	    exitCode = 2;
            fprintf(stderr, "psclient: invalid topic\n"); 
            break; 
        default:
	    break;
    }
    exit(exitCode); 
}

/*check_validity()
 *----------------
 *
 *argc : number of arguments on the command line
 *argv: pointer to the arguments on the command line
 *
 *This function checks for any kind of invalid command line arguments:
 *-insufficient command line arguments
 *-The name argument must not contain any spaces, colons, or newlines.
 *-The name argument must not be an empty string.
 *-All topic arguments must not contain any spaces, colons, or newlines
 *-All topic arguments must also not be empty strings.
 *
 *if one of those conditions were applied then exit_program() will execute
 */
void check_validity(int argc, char** argv) {

    if (argc < MINIMUM_ARGS) {
	exit_program(INVALID_COMMAND_LINES);
    }
    if (strcmp(argv[2], "") == 0) {
	exit_program(INVALID_NAME);
    }
    for (int i = 0; i < strlen(argv[2]); i++) {
	if (argv[2][i] == ' ' || argv[2][i] == ':' || argv[2][i] == '\n') {
	    exit_program(INVALID_NAME);
	}
    }
    if (argc > MINIMUM_ARGS) {
	for (int j = 0; j < argc - MINIMUM_ARGS; j++) {
	    if (strcmp(argv[j + MINIMUM_ARGS], "") == 0) {
		exit_program(INVALID_TOPIC);
	    } 
	    for (int k = 0; k < strlen(argv[j + MINIMUM_ARGS]); k++) {
		if (argv[j + MINIMUM_ARGS][k] == ' ' ||
			argv[j + MINIMUM_ARGS][k] == ':' ||
			argv[j + MINIMUM_ARGS][k] == '\n') {
		    exit_program(INVALID_TOPIC);
		}
	    }
        }
    } 
}

/*connect_to_port()
 *----------------
 *
 *port: the port number that the client needs to connect to
 *
 *This function creates a socket and connects it to the port
 *if the connection fails then it will send a message to stderr
 *and execute exit_program();
 *return the socket if the connection succeeds
 *the whole point of this is to connect the client with it's server
 */
int connect_to_port(char* port) {
    struct addrinfo* ai = NULL; 
    struct addrinfo hints; 

    memset(&hints, 0, sizeof(struct addrinfo)); 
    hints.ai_family = AF_INET;  
    hints.ai_socktype = SOCK_STREAM;
    int err;
    if ((err = getaddrinfo(NULL, port, &hints, &ai))) {
        freeaddrinfo(ai);
        fprintf(stderr, "psclient: unable to connect to port %s\n", port); 
        exit_program(INVALID_CONNECTION); 
    }
    int fd = socket(AF_INET, SOCK_STREAM, 0);  
    if (connect(fd, (struct sockaddr*)ai->ai_addr, sizeof(struct sockaddr))) {
	fprintf(stderr, "psclient: unable to connect to port %s\n", port);
        exit_program(INVALID_CONNECTION);
    }
    
    return fd; 
}

/*read_thread()
 *-----------
 *
 *arg: void pointer (it can be anything);
 *
 *Here we are passing the FILE* which writes to the socket that is connected
 *to. This thread function is reading from stdin and passing the input to the 
 *the FILE*. If we read NULL from stdin then exit normally. 
 *
 *
 */
void* read_thread(void* arg) {
    FILE* file = (FILE*)arg;
    char* line;
    while ((line = read_line(stdin))) {
        fprintf(file, "%s\n", line);
        fflush(file);
    }
    if (line == NULL) {
        fclose(file);
        exit_program(EXIT_NORMALY);
    }
    return ((void*)0);

}

/*write_thread()
 * -------------
 *
 *arg: void pointer (it can be anything)
 *
 *Here we are passing the FILE* which reads from the socket that is connected 
 *to. This thread function is reading from the FILE* and outputing the message
 *to stdout, if it reads NULL this means that the server terminated so the
 *client has to diconnect too and terminate
 *
 */
void* write_thread(void* arg) {
    FILE* file = (FILE*)arg;
    char* line;
    while ((line = read_line(file))) {
        printf("%s\n", line);
        fflush(stdout);

    } 
 
    if (line == NULL) {
        fprintf(stderr, "psclient: server connection terminated\n");
        fclose(file);
        exit(DISCONNECTED);
    }
    return ((void*)0);

}

/*execute()
 *----------
 *
 *fd: the socket that we connected it with the server 
 *argc: number of arguments on the command line 
 *argv: pointer to arguments on the command line
 *
 *this function dup() the socket, one for reading and one for writing
 *it prints to the writing FILE* the starting messages which includes name
 *and subscription requests if present. Then it creates two threads: one 
 *for reading from stdin and then passing the input to the server, and 
 *the other one is to read from the server and output the message to stdout 
 *(see write_thread() and read_thread())
 *
 */
void execute(int fd, int argc, char** argv) {
    int fd2 = dup(fd);
    FILE* read = fdopen(fd, "r");
    FILE* write = fdopen(fd2, "w");
    fprintf(write, "name %s\n", argv[2]);
    if (argc > 3) {
        for (int i = 0; i < argc - 3; i++) {
            fprintf(write, "sub %s\n", argv[i + 3]);

        }
    }
    fflush(write);
    pthread_t threadId;
    pthread_create(&threadId, NULL, read_thread, write);
    pthread_create(&threadId, NULL, write_thread, read);
    pthread_exit((void*)0);
}

int main(int argc, char** argv) {

    check_validity(argc, argv);
    int port = connect_to_port(argv[1]);
    execute(port, argc, argv);
    return 0;
}

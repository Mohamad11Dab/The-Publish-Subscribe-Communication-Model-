#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <csse2310a3.h>
#include <stringmap.h>
#include <csse2310a4.h>
#define INVALID_COMMAND_LINE 1
#define MIN_ARGUMENTS 2
#define MAX_ARGUMENTS 3
#define MIN_PORT_NUMBER 1024
#define MAX_PORT_NUMBER 65535
#define EPHERMAL_PORT 0
#define LISTEN_PROBLEM 2
#define MAX_QUEUE 100
#define MAX_CONNECTIONS 1000

/*data structure carrying StringMap and file descriptors*/
typedef struct Param {
    int* to;
    int* from;
    StringMap** sm;
    sem_t* lock;
}Param;

/*Linked list which carries information about each client*/
typedef struct Client {
    char* name;
    FILE* to;
    FILE* from;
    struct Client* next;
}Client;

/*invalid_command_line()
 *----------------------
 *Prints out message to standard error and exits for any invalid 
 *command line arguments
 *
 */
void invalid_command_line(void) {
    fprintf(stderr, "Usage: psserver connections [portnum]\n");
    exit(INVALID_COMMAND_LINE);

}

/*check_integer()
 *--------------
 *arg: pointer to string (char*) that we're checking
 *
 *check if the string is a positive integer, if not
 *send message to stderr and exit (invalid command line argument),
 *else do nothing
 *ps: it is basically cheking for the number of connections argument
 */
void check_integer(char* arg) {
    for (int j = 0; j < strlen(arg); j++) {
        if (!isdigit(arg[j])) {
            invalid_command_line();
        }
    } 

    if (atoi(arg) < 0) {
        invalid_command_line();
    }
    for (int i = 0; i < strlen(arg); i++) {
	if (arg[i] == '.') {
	    invalid_command_line();
	}
    }
}

/*check_port()
 *-------------
 *
 *arg: pointer to string(char*) the we need to check
 *
 *This function is checking if the provided port argument is numerical
 *or not, if it is numerical then we are checking if it is a positive 
 *integer, if it is not numerical then there should be no numbers in it
 *
 *It sends a message to stderr and exits if any of the conditions fail
 */
void check_port(char* arg) {
    int checkAlphabetic = 0;
    for (int j = 0; j < strlen(arg); j++) {
        if (!isalpha(arg[j])) {   
            checkAlphabetic = 1;
        }
    }
    if (checkAlphabetic == 1) {
        for (int j = 0; j < strlen(arg); j++) {
            if (!isdigit(arg[j])) {
                invalid_command_line();
            }
        } 
        if (atoi(arg) < 0) {
            invalid_command_line();
        }
        for (int i = 0; i < strlen(arg); i++) {
	    if (arg[i] == '.') {
	        invalid_command_line();
	    }
        }
    }    
}

/*check_validity()
 *----------------
 *
 *argc: number of arguments on the command line
 *argv: pointer to the arguemnts on the command line
 *
 *This function checks entirely for any invalid command line arguments:
 *– no max connections number specified 157
 * – the connections argument is not a non-negative integer 
 * – the port number argument (if present) is not an integer value,
 * or is an integer value and is not either zero, or in the range of
 * 1024 to 65535 inclusive 
 * – too many arguments are supplied
 *
 *if one of the conditions fail then it will send a message to stderr and exit
 */
void check_validity(int argc, char** argv) {

    if (argc < MIN_ARGUMENTS || argc > MAX_ARGUMENTS) {
	invalid_command_line();

    }
    if (argc == MIN_ARGUMENTS) {
        check_integer(argv[1]);
    }
    if (argc == MAX_ARGUMENTS) {
        check_integer(argv[1]);
        check_port(argv[2]);
        if (atoi(argv[2]) < MIN_PORT_NUMBER ||
        	atoi(argv[2]) > MAX_PORT_NUMBER) {
            if (atoi(argv[2]) == 0) {
		;
	    } else {
	        invalid_command_line();
            }
        }
    }
}

/*port_number()
 *--------------
 *
 *argc: number of arguments on the command line
 *argv: pointer to the arguments on the command line 
 *
 *This function checks if a port number have been provided as a
 *command line arguemnt, if yes then return the number(as a string)as is
 *if not then return "0" to use epheremal port later on
 *
 */
char* port_number(int argc, char** argv) {
   
    if (argc == MIN_ARGUMENTS) {
        return "0";
    }
    if (argc == MAX_ARGUMENTS) {
	return argv[2];
    }
    return "0";
}

/*open_listen()
 *---------------
 *
 *port: pointer to a string which is the port number to open
 *
 *this function takes the port number and checks for adress , if adress
 *could no be determined then print message to stderr and exit,
 *else try and create a socket to bind it to the port then 
 *try opening the socket for listening if listening fails then send message
 *to stderr and exit, if listening succeeds then print the port 
 *number to stderr and return socket
 */
int open_listen(const char* port)
{
    struct addrinfo* ai = 0;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;   // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;    // listen on all IP addresses

    int err;
    if ((err = getaddrinfo(NULL, port, &hints, &ai))) {
        freeaddrinfo(ai);
        invalid_command_line();   // Could not determine address
    }

    // Create a socket and bind it to a port
    int listenfd = socket(AF_INET, SOCK_STREAM, 0); // 0=default protocol (TCP)

    // Allow address (port number) to be reused immediately
    int optVal = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
            &optVal, sizeof(int)) < 0) {
        perror("Error setting socket option");
    }

    if (bind(listenfd, (struct sockaddr*)ai->ai_addr, 
            sizeof(struct sockaddr)) < 0) {
        fprintf(stderr, "psserver: unable to open socket for listening\n");
        exit(LISTEN_PROBLEM);

        
    }

    // Up to 100 connection requests can queue
    if (listen(listenfd, MAX_QUEUE) < 0) {
        fprintf(stderr, "psserver: unable to open socket for listening\n");
        exit(LISTEN_PROBLEM);
    }
    struct sockaddr_in ad;
    memset(&ad, 0, sizeof(struct sockaddr_in));
    socklen_t len = sizeof(struct sockaddr_in);
    if (getsockname(listenfd, (struct sockaddr*) &ad, &len)) {
        perror("sockname");
    } 
    fprintf(stderr, "%u\n",ntohs(ad.sin_port));
    fflush(stderr);
    // Have listening socket - return it
    return listenfd;
}

/*check_invalid_commands()
 *------------------------
 *
 *command: pointer to strings (char**) which is the command sent from
 *the client to the server
 *
 *this function checks for any invalid command coming from the client:
 *- Invalid command word – an empty string or a command word which is not
 * pub, sub, name or unsub 
 *- Invalid arguments such as empty strings (for any field) or names
 * or topics containing spaces or colons 
 *- Too few or too many arguments 
 *- Any other kind of malformed message
 *
 *if any condition fails then print :invalid (implemented in client_thread)
 *
 *return 1 if coomand line is valid else return 0
 */
int check_invalid_commands(char** command) {

    if (strcmp(command[0], "name") != 0 && strcmp(command[0], "pub") != 0 &&
	    strcmp(command[0], "sub") != 0 &&
	    strcmp(command[0], "unsub") != 0) {
	return 1; 
    }
    if (strcmp(command[0], "sub") == 0 || strcmp(command[0], "unsub") == 0 ||
	    strcmp(command[0], "name") == 0) {
	int count = 0;
	for (int i = 0; command[i] != NULL; i++) {
	    count++;
        }
        if (count != 2) {
            return 1;
        }
	if (strcmp(command[1], "") == 0) {

            return 1;
        }
        for (int j = 0; j < strlen(command[1]); j++) {
	    if (command[1][j] == ':') {
		return 1;
	    }
	} 
    }
    if (strcmp(command[0], "pub") == 0) {
	int count = 0;
	for (int i = 0; command[i] != NULL;i++) {
	    count++;
        }
        if (count < 3) {
            return 1;
        }
        for (int j = 0; j < strlen(command[1]); j++) {
	    if (command[1][j] == ':') {
		return 1;
	    }
	}
        if (strcmp(command[2], "") == 0) {
	    return 1;
	} 
    }

    return 0;

}

/*terminate()
 *-----------
 *
 *StringMap** sm: pointer to StringMap which holds the entries to a topic 
 *and Client struct linked list
 *Client* client: pointer to a Client struct which is the client we are 
 *dealing with now
 *
 *This function is implemented for when a client terminates: 
 *it accesses the StringMap and iterates over all the entries 
 *(StringMapItem) then checks if the linked list inside each entry has
 *the client's name in it, if yes then remove the client (unsubscribes to
 *all the topics that this client is subscribed to) else do nothing
 *
 */
void terminate(struct StringMap** sm, struct Client* client) {
    StringMapItem* stringItem = NULL;
    StringMapItem* base;
    while ((base = stringmap_iterate(*sm, stringItem))!= NULL){
     //  struct Client* current = (struct Client*) base->item;
     //   struct Client* ptr = (struct Client*) base->item;
        struct Client* root =(struct Client*) base->item;
	struct Client* current = root;
	struct Client* ptr = root;
        while (current != NULL) {
	    if (current->name == client->name) {
	        if (current == root) {
       		    root = root->next;
		//	free(current);
                    if (stringmap_remove(*sm, base->key)) {
		        if (stringmap_add(*sm, base->key,
			        (struct Client*)root)) {
			    ;
		        }
                    }     
	            break;

		} else {
		    ptr->next = current->next;
                    if (stringmap_remove(*sm, base->key)) {
		        if (stringmap_add(*sm, base->key,
		               	(struct Client*)root)) {
			    ;
		        }
                    }
		    break;
		}


	    } else {
		ptr = current;
		current = current->next;
	    }
	}
        stringItem = base;
    }
}

/*unsubscribe()
 *--------------
 *
 *StringMap** sm: pointer to StringMap which holds the entries to a topic 
 *and Client struct linked list
 *Client* client: pointer to a Client struct which is the client we are 
 *dealing with now
 *command: a pointer to strings (char**) which is the command sent from the
 *client
 *
 *This function is implemented to handle unsubscription requests:
 *it searches for the topic listed in the command inside the StringMap
 *if NULL is returned this means that there is no such topic, else if a Client
 *struct is returned, we have to iterate over the linked list until we find
 *the client that is trying to unsubscribe then remove it,if the client was
 *never subscribed to this topic then do nothing(linked list reached NULL)
 *
 */
void unsubscribe(struct StringMap** sm, char** command,
	struct Client* client) {

    if (strcmp(client->name, "NONAME") != 0) {
        struct Client* root = (struct Client*) stringmap_search(*sm,
                command[1]);
        if (root != NULL) {
	    struct Client* current = root;
	    struct Client* ptr = root;
            while (current != NULL) {
		if (current->name == client->name) {

		    if (current == root) {
			root = root->next;
		//	free(current);
                        if (stringmap_remove(*sm, command[1])) {
		            if (stringmap_add(*sm, command[1],
			  	    (struct Client*)root)) {
			        ;
		            }
                        }     
			break;

		    } else {
			ptr->next = current->next;
		//	free(current);
                        if (stringmap_remove(*sm, command[1])) {
		            if (stringmap_add(*sm, command[1],
				    (struct Client*)root)) {
			        ;
		            }
                        }
			break;
		    }


		} else {
		    ptr = current;
		    current = current->next;
		}
	    }
	}
    }
}

/*subscribe()
 *------------
 *
 *StringMap** sm: pointer to StringMap which holds the entries to a topic 
 *and Client struct linked list
 *Client* originalClient: pointer to a Client struct which is the client we  
 *are dealing with now
 *command: a pointer to strings (char**) which is the command sent from the
 *client
 *
 *this function is implemented to handle subscrbtion requests:
 *search for the topic listed in the command inside the StringMap
 *if NULL is returned this means that no client had subscribed to the topic
 *before, so we add the client as is as a root as a new entry with the topic
 *as the key
 *however if a Client struct is returned then iterate over the linked list
 *and add the client in the end and set its next to NULL,after that
 *we have to remove the entry associated with that topic and replace it
 *with a new entry(the one with the new client added)
 */
void subscribe(struct StringMap** sm, char** command,
	struct Client* originalClient){
  
    if (strcmp(originalClient->name, "NONAME") != 0) {
        struct Client* client = malloc(sizeof(struct Client));
        client->name = originalClient->name;
        client->to = originalClient->to;
        client->from = originalClient->from;
        client->next = NULL;
        struct Client* root = (struct Client*) stringmap_search(*sm,
		command[1]);
        if (root == NULL) {
            if (stringmap_add(*sm, command[1], (struct Client*)client)) {
                ;
            } else {
	        perror("subscribe problem");
            }

        } else {
            struct Client* current = root;
            if (strcmp(current->name, client->name) != 0) {  
                while (1) {
	            if (current->next == NULL) {
	                client->next = NULL;
	  	        current->next = client;
	                if (stringmap_remove(*sm, command[1])) {
		            if (stringmap_add(*sm, command[1],
			            (struct Client*)root)) {
			        ;
		            }
                        }
		        break;	
	            }
	            current = current->next;
	            if (strcmp(current->name, client->name) == 0) {
                        break;
                    }
                }
            }
        }
    }
}

/*publish()
 *------------
 *
 *StringMap** sm: pointer to StringMap which holds the entries to a topic 
 *and Client struct linked list
 *Client* client: pointer to a Client struct which is the client we are 
 *dealing with now
 *command: a pointer to strings (char**) which is the command sent from the
 *client
 *
 *This function was implemented to handle publish requests:
 *search for the topic listed in the command and wish to publish to inside 
 *the StringMap, is NULL is returned then there is no such topic so do nothing
 *else if Client struct is returned then iterate over the linked list and 
 *send to each and every client struct in it the publish message with the 
 *listed value in the command (using Client->from which is a FILE*)
 */
void publish(struct StringMap** sm, char** command,
	struct Client* client) {
    if (strcmp(client->name, "NONAME") != 0) {
        struct Client* root = (struct Client*) stringmap_search(*sm,
		command[1]);
        if (root != NULL) {
            struct Client* current = root;              
            while (current != NULL) {
                fprintf(current->from, "%s:%s:%s\n", client->name, command[1],
	                command[2]);
                fflush(current->from);
                current = current->next;
            }
        }
    }
}

/*client_thread()
 *---------------
 *
 *arg: void pointer (it can carry anything)
 *
 *This function is a thread function which handles the client behavior
 *each client has his own Client struct and from which we read the messages
 *sent from the client (using Client->to) and on that basis we output back
 *the messages from the server to the client( using Client->from). Generally,
 *this function handles all the invalid command line, sub, unsub,
 *and pub requests sent from the client
 */
void* client_thread(void* arg) {
    struct Param* param = (struct Param*) arg;
    char* line;
    struct Client* client = (struct Client*)malloc(sizeof(struct Client));
    client->name = "NONAME";
    client->to = fdopen(*param->to, "r");
    client->from = fdopen(*param->from, "w");
    int count = 0;
    char** command;
    while ((line = read_line(client->to))) {
        command = split_by_char(line, ' ', 3);
        if (count == 0) {
            if (strcmp(command[0], "name") == 0) {
                client->name = command[1];
                count = 1;
            }
        }
        if (check_invalid_commands(command)) {
	    fprintf(client->from, ":invalid\n");
	    fflush(client->from);
	    continue;
	}

        if (strcmp(command[0], "sub") == 0) {
           
            subscribe(param->sm, command, client);
	  
        }
	if (strcmp(command[0], "pub") == 0) {
	    publish(param->sm, command, client);
        }
        if (strcmp(command[0], "unsub") == 0) {
	    unsubscribe(param->sm, command, client);
        }
     
    }
    terminate(param->sm, client);
  

    sem_post(param->lock);
    return ((void*)0);
} 

/*process_connection()
 *-------------------
 *
 *fdServer: this is the socket sent from open_listen() function 
 *StringMap** sm: pointer to StringMap which holds the entries to a topic 
 *and Client struct linked list
 *argv: pointer to the arguments on the command line
 *
 *This function is responsible of processing the connections between
 *the clients and the server,it handles also limiting client connections
 *accoring to the number of connection specified on the command line
 *argument,then for every accepted connection it creates a thread for the
 *client 
 *
 */
void process_connections(int fdServer, StringMap* sm, char** argv)
{
    int fd;
    struct sockaddr_in fromAddr;
    socklen_t fromAddrSize;
    sem_t mutex;
    sem_t lock;
    sem_init(&mutex, 0, 1);
    if (atoi(argv[1]) == 0) {
        sem_init(&lock, 0, MAX_CONNECTIONS);
    } else {
        sem_init(&lock, 0, atoi(argv[1]));
    }
    // Repeatedly accept connections and process data 
    while (1) {
        fromAddrSize = sizeof(struct sockaddr_in);
	// Block, waiting for a new connection. (fromAddr will be populated
	// with address of client)
	sem_wait(&lock);
        fd = accept(fdServer, (struct sockaddr*)&fromAddr, &fromAddrSize);
        if (fd < 0) {
            perror("Error accepting connection");
            exit(1);
        }

	// Start a thread to deal with client
        int fd2 = dup(fd);
        Param param;
        param.to = &fd2;
        param.from = &fd;
        param.sm = &sm;
        param.lock = &lock;
      //  param.mutual_ex = &mutex;

	pthread_t threadId;
	pthread_create(&threadId, NULL, client_thread, &param);
	pthread_detach(threadId);
    }
}

int main(int argc, char** argv) {
    StringMap* sm = stringmap_init();
    check_validity(argc, argv);
    const char* port = port_number(argc, argv);
    int fdserver = open_listen(port);
    process_connections(fdserver, sm, argv);  
   
    return 0;
}

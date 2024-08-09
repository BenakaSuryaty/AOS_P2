#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <time.h>

#define ERROR (-1)

#define P_ID 1

#define IP "127.0.0.1" 

#define PRIMARY_PORT 4000


#define MANGER_IP "127.0.0.1"
#define MANAGER_PORT 5000


typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

enum MessageType { CAN_COMMIT, PRE_COMMIT, DO_COMMIT, ACK, ABORT, TERMINATE};

typedef struct {
    enum MessageType type;
    int value;
} Message;

bool failure_occurred = false;


void check(int exp, const char *msg);
void handle_messages(int client_socket, Message msg);
void send_message(int socket, Message msg);
Message receive_message(int socket);
bool should_fail();
bool can_fail();


int main() {
    srand(time(NULL));  // Seed for random number generation

    int server_socket;
    SA_IN server_addr;

    // Create a socket
    check((server_socket = socket(AF_INET, SOCK_STREAM, 0)), "ERROR: Failed to create socket\n");

    // Setup server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(IP);
    server_addr.sin_port = htons(PRIMARY_PORT);

    // Bind the socket
    check(bind(server_socket, (SA*)&server_addr, sizeof(server_addr)), "ERROR: Failed to bind socket\n");

    // Listen for incoming connections
    check(listen(server_socket, 3), "ERROR: Failed to listen on socket\n");

    printf("=== Participant is waiting for connections ===\n");

    // Accept connections from the coordinator
 
        int client_socket = accept(server_socket, NULL, NULL);
        if (client_socket < 0) {
            perror("ERROR: Failed to accept connection");
            exit(0);
        }
        printf("=== Coordinator connected! ===\n");

             while (true)
             {
                Message msg = receive_message(client_socket);

            if (should_fail()) {
                if (msg.type == CAN_COMMIT || msg.type == PRE_COMMIT) {
                    printf("=== Failure simulation in progress before %s phase. ===\n", 
                    msg.type == CAN_COMMIT ? "CAN_COMMIT" : "PRE_COMMIT");
                    close(client_socket);
                    printf("=== Resuming operations after simulated failure. ===\n");
                     int client_socket = accept(server_socket, NULL, NULL);
                     if (client_socket < 0) {
                        perror("ERROR: Failed to accept connection");
                        exit(0);
                        }
                        printf("=== Coordinator reconnected! ===\n");
                        continue;
                }
            }else{
            handle_messages(client_socket, msg);
            }
        }
        
        
        
        close(server_socket);
    
    return 0;
}

void check(int exp, const char *msg) {
    if (exp == ERROR) {
        perror(msg);
        exit(1);
    }
}

void handle_messages(int client_socket, Message msg) {


        switch (msg.type) {
            case CAN_COMMIT:
                printf("CAN_COMMIT received, acknowledging...\n");
                send_message(client_socket, (Message){ACK, 0});
                break;

            case PRE_COMMIT:
                printf("PRE_COMMIT received, acknowledging...\n");
                send_message(client_socket, (Message){ACK, 0});
                break;

            case DO_COMMIT:
                printf("DO_COMMIT received with value, committing value %d...\n", msg.value);
                
                // Open F1.txt in append mode
                FILE *file = fopen("F1.txt", "a");
                if (file == NULL) {
                    perror("ERROR: Failed to open file");
                    exit(1);
                }

                // Write the value to the file
                fprintf(file, "%d\n", msg.value);

                // Close the file
                fclose(file);
                
                printf("Transaction committed successfully.\n");
                break;

            case ABORT:
                printf("ABORT received. Transaction aborted.\n");
                break;

            case TERMINATE:
                printf("Termination issued from the Coordinator. Terminating...\n");
                close(client_socket);
                exit(0);
                break;

            default:
                printf("Participant: Unknown message type.\n");
                exit(0);
                break;
        }
    
}

void send_message(int socket, Message msg) {
    if (send(socket, &msg, sizeof(msg), 0) == ERROR) {
        perror("ERROR: Failed to send message");
        exit(1);
    }
}

Message receive_message(int socket) {
    Message msg;
    if (recv(socket, &msg, sizeof(msg), 0) == ERROR) {
        perror("ERROR: Failed to receive message");
        exit(1);
    }
    return msg;
}

// Function to randomly decide whether to fail only once
bool should_fail() {
  
    if (failure_occurred) {
       
        return false;  // Failure has already occurred
    }

    bool fail = rand() % 2 == 0 && can_fail();  // Check if it can fail before random decision
    if (fail) {
        failure_occurred = true;  // Mark that a failure has occurred
    }


    return fail;
}

// Function to check if failure can happen by contacting other participants
bool can_fail() {
    int sock;
    struct sockaddr_in server_addr;
    char response[128];
    char request[128];


    snprintf(request, sizeof(request), "ALLOW_FAIL:%d", P_ID); // Send participant ID

    // Contact the centralized failure manager
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("ERROR: Could not create socket to Failure Manager");
        return false;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(MANAGER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(MANGER_IP);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("ERROR: Connection to Failure Manager failed");
        close(sock);
        return false;
    }

    send(sock, request, strlen(request), 0);
    
    ssize_t bytes_received = recv(sock, response, sizeof(response), 0);
    if (bytes_received < 0) {
        perror("ERROR: Failed to receive response from Failure Manager");
        close(sock);
        return false;
    }
    response[bytes_received] = '\0';
    
    close(sock);


    return strcmp(response, "OK") == 0;  // Return true if allowed to fail
}


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>
#include <sys/select.h>
#include <sys/types.h>
#include<errno.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define ERROR (-1)

#define COHORT_1_PORT 4000
#define COHORT_1_IP "127.0.0.1"

#define COHORT_2_PORT 4001
#define COHORT_2_IP "127.0.0.1"

#define COHORT_3_PORT 4002
#define COHORT_3_IP "127.0.0.1"

#define MANGER_IP "127.0.0.1"
#define MANAGER_PORT 5000

#define TIMEOUT 5 

#define PART_CNT 3

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

enum MessageType { CAN_COMMIT, PRE_COMMIT, DO_COMMIT, ACK, ABORT, TERMINATE, T_OUT, DISCONNECTED};

typedef struct {
    enum MessageType type;
    int value;
} Message;

typedef struct {
    int p_num;
    int socket;
    struct sockaddr_in address;
    enum MessageType last_action;
} Participant;

int check(int exp, const char *msg);
void threepc(Participant *participants, int user_int);
void send_message(Message msg, Participant *participant);
Message receive_message(Participant *participant);
void reconnect(Participant *participant);
void recover_participant(Participant *participant);
void ping_manager();

int main(){
    int CohortSock1, CohortSock2, CohortSock3, choice;
    SA_IN server_addr;
    Participant participants[3];
    int user_int;
    
    printf("=== Starting Coordinator ===\n");
    
    check((CohortSock1 = socket(AF_INET, SOCK_STREAM, 0)), "ERROR: Failed to create Cohort 1 socket \n");
    check((CohortSock2 = socket(AF_INET, SOCK_STREAM, 0)), "ERROR: Failed to create Cohort 2 socket \n");
    check((CohortSock3 = socket(AF_INET, SOCK_STREAM, 0)), "ERROR: Failed to create Cohort 3 socket \n");

   

    printf("=== Establishing connection with the Cohorts ===\n");

    // NOTE: Cohort 1
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(COHORT_1_PORT); 
    inet_pton(AF_INET, COHORT_1_IP, &server_addr.sin_addr.s_addr);
    check(connect(CohortSock1, (SA*)&server_addr, sizeof(server_addr)), "ERROR: Connection with Server 1 failed! \n");
    participants[0].p_num = 1;
    participants[0].socket = CohortSock1;
    participants[0].address = server_addr;
    participants[0].last_action = ABORT; // Initialize last action
    printf("=== Cohort 1 Connected! ===\n");

    // NOTE: Cohort 2
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(COHORT_2_PORT); 
    inet_pton(AF_INET, COHORT_2_IP, &server_addr.sin_addr.s_addr);
    check(connect(CohortSock2, (SA*)&server_addr, sizeof(server_addr)), "ERROR: Connection with Server 2 failed! \n");
    participants[1].p_num = 2;
    participants[1].socket = CohortSock2;
    participants[1].address = server_addr;
    participants[1].last_action = ABORT; // Initialize last action
    printf("=== Cohort 2 Connected! ===\n");

    // NOTE: Cohort 3
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(COHORT_3_PORT); 
    inet_pton(AF_INET, COHORT_3_IP, &server_addr.sin_addr.s_addr);
    check(connect(CohortSock3, (SA*)&server_addr, sizeof(server_addr)), "ERROR: Connection with Server 3 failed! \n");
    participants[2].p_num = 3;
    participants[2].socket = CohortSock3;
    participants[2].address = server_addr;
    participants[2].last_action = ABORT; // Initialize last action
    printf("=== Cohort 3 Connected! ===\n");


    // NOTE: Event Loop
    while (true) { 
        printf("Menu:\n");
        printf("1. Input an integer\n");
        printf("2. Terminate\n");
        printf("Enter your choice: ");

        if (scanf("%d", &choice) != 1) {
            printf("ERROR: Invalid input. Please enter a number.\n");
            while (getchar() != '\n'); // Clear the input buffer
            continue;
        }

        switch (choice) {
            case 1:
                printf("Enter an integer: ");
                if (scanf("%d", &user_int) != 1) {
                    printf("ERROR: Invalid input. Please enter a number.\n");
                    while (getchar() != '\n'); // Clear the input buffer
                    continue;
                }
                threepc(participants, (int)user_int);
                break;
            case 2:
                printf("Terminating...\n");
                for (int i = 0; i < PART_CNT; i++) {
                    Message msg;
                    msg.type = TERMINATE;
                    msg.value = 0;
                    send_message(msg, &participants[i]);
                }
                exit(0);
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }

    

    return 0;
}

int check(int exp, const char *msg) {
    if (exp == ERROR) {
        perror(msg);
        exit(1);
    } 
    return exp;
}

void send_message(Message msg,Participant *participant) {
    if (send(participant->socket, &msg, sizeof(msg), 0) == ERROR) {
        perror("ERROR: Failed to send message");
        participant->last_action = ABORT;
        return;
    }
    participant->last_action = msg.type; // Update the last action
}

Message receive_message(Participant *participant) {
    fd_set read_fds;
    struct timeval tv;
    Message msg;
    int socket = participant->socket;

    FD_ZERO(&read_fds);
    FD_SET(socket, &read_fds);

    // Set timeout
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;

    int retval = select(socket + 1, &read_fds, NULL, NULL, &tv);

    if (retval == -1) {
        perror("ERROR: select() failed");
        exit(1);
    } else if (retval == 0) {
        printf("ERROR: Timeout occurred while waiting for response.\n");
        msg.type = T_OUT; // Indicate timeout
        return msg;
    } else {
        ssize_t bytes_received = recv(socket, &msg, sizeof(msg), 0);

        if (bytes_received == -1) {
            perror("ERROR: Failed to receive message");
            participant->socket = -1;
            msg.type = T_OUT;  // Indicate a failure (you might want to introduce a new error type)
        } else if (bytes_received == 0) {
            // This means the connection was closed by the participant
            printf("ERROR: Connection closed by participant.\n");
            participant->socket = -1;
            msg.type = DISCONNECTED;  // Indicate a disconnection
        }

        return msg;
    }
}
void reconnect(Participant *participant) {

     if (participant->socket != ERROR) {
        close(participant->socket);
    }

    int sock;
    SA_IN server_addr = participant->address;

    printf("Reconnecting to participant %d...\n", participant->p_num);

    check((sock = socket(AF_INET, SOCK_STREAM, 0)), "ERROR: Failed to create socket \n");

    while (connect(sock, (SA*)&server_addr, sizeof(server_addr)) == ERROR) {
        printf("Retrying connection...\n");
        sleep(1);
    }

    participant->socket = sock;
    printf("Reconnected to participant %d.\n", participant->p_num);
}

void recover_participant(Participant *participant) {
    reconnect(participant);
    // Resend the last message
    Message msg = {ABORT, 0};
    send_message(msg, participant);
    participant->last_action = ABORT;
    printf("Participant %d recovered and sent ABORT message.\n", participant->p_num);
}

void abort_transaction(Participant *participants, const char *reason) {
    printf("Aborting transaction: %s\n", reason);
    for (int i = 0; i < PART_CNT; i++) {
        if (participants[i].socket != ERROR) {
            // If the socket is still valid, send an ABORT message
            Message msg = {ABORT, 0};
            send_message(msg, &participants[i]);
        } else {
            // Attempt to recover the participant first
            recover_participant(&participants[i]);
        }
    }
    ping_manager();
    printf("Transaction aborted.\n");
}

void threepc(Participant *participants, int user_int) {
    printf("=== Initiating 3PC ===\n");

    // Phase 1: CanCommit
    int yes_count = 0;
    printf("Sending CanCommit message to participants.\n");
    for (int i = 0; i < PART_CNT; i++) {
        Message msg = {CAN_COMMIT, user_int};
        send_message(msg, &participants[i]);
    }

    for (int i = 0; i < PART_CNT; i++) {
        Message response = receive_message(&participants[i]);
        if (response.type == ACK) {
            yes_count++;
        } else if (response.type == T_OUT || response.type == DISCONNECTED) {
            abort_transaction(participants, "Participant failure in CanCommit phase.");
            ping_manager();
            return;
        }
    }

    // Phase 2: PreCommit
    if (yes_count == PART_CNT) {
        printf("Sending PreCommit message to participants.\n");
        int ack_count = 0;
        for (int i = 0; i < PART_CNT; i++) {
            Message msg = {PRE_COMMIT, user_int};
            send_message(msg, &participants[i]);
        }

        for (int i = 0; i < PART_CNT; i++) {
            Message response = receive_message(&participants[i]);
            if (response.type == ACK) {
                ack_count++;
            } else if (response.type == T_OUT || response.type == DISCONNECTED) {
                abort_transaction(participants, "Participant failure in PreCommit phase.");
                ping_manager();
                return;
            }
        }

        // Phase 3: DoCommit
        if (ack_count == PART_CNT) {
            printf("Sending DoCommit message to participants.\n");
            for (int i = 0; i < PART_CNT; i++) {
                Message msg = {DO_COMMIT, user_int};
                send_message(msg, &participants[i]);
            }

            // Log commit on the coordinator
            FILE *file = fopen("F4.txt", "a");
            if (file == NULL) {
                perror("ERROR: Failed to open file");
                exit(1);
            }
            fprintf(file, "%d\n", user_int);
            fclose(file);

            ping_manager();
            printf("Transaction committed.\n");
        }
    }
}


void ping_manager() {
    
    int ManagerSock;
    SA_IN server_addr;

    check((ManagerSock = socket(AF_INET, SOCK_STREAM, 0)),"ERROR: Failed to create Manager socket \n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(MANAGER_PORT); 
    inet_pton(AF_INET, MANGER_IP, &server_addr.sin_addr.s_addr);
    check(connect(ManagerSock, (SA*)&server_addr, sizeof(server_addr)), "ERROR: Connection with Manger failed! \n");


    const char *ping_message = "PING:ROUND_COMPLETED";  // Message to send to the manager
    char response[128];  // Buffer to hold the response from the manager

    // Send the ping message to the failure manager
    if (send(ManagerSock, ping_message, strlen(ping_message), 0) < 0) {
        perror("ERROR: Failed to send ping to the manager");
        return;
    }

    // Receive the response from the manager
    ssize_t bytes_received = recv(ManagerSock, response, sizeof(response) - 1, 0);
    if (bytes_received < 0) {
        perror("ERROR: Failed to receive response from the manager");
        return;
    }

    response[bytes_received] = '\0';  // Null-terminate the response

    // Handle the response
    if (strcmp(response, "OK") == 0) {
        printf("Manager acknowledged the round completion.\n");
    }

    close(ManagerSock);
}
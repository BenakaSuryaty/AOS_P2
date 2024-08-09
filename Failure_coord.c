#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MANAGER_IP "127.0.0.1"
#define MANAGER_PORT 5000

#define OK "OK"
#define FAILING "FAILING"
#define ALLOW_FAIL "ALLOW_FAIL"
#define MAX_PARTICIPANTS 3

typedef struct {
    char ip[16];
    int failed; // 1 if failed, 0 if not
} Participant;

Participant participants[MAX_PARTICIPANTS];
int current_round = 0;      // To track the current round
int failures_in_round = 0;  // To track failures in the current round

// Function to initialize participant data
void initialize_participants() {

    strcpy(participants[0].ip, "127.0.0.1");// Participant 1
    participants[0].failed = 0;

    strcpy(participants[1].ip, "127.0.0.1");// Participant 2
    participants[1].failed = 0;

    strcpy(participants[2].ip, "127.0.0.1"); // Participant 3
    participants[2].failed = 0;
}

void reset_round() {
    // Reset the failure tracking for the next round
    for (int i = 0; i < MAX_PARTICIPANTS; i++) {
        participants[i].failed = 0; // Reset all to not failed
    }
    failures_in_round = 0; // Reset failure count
    current_round++;        // Increment the round
}

void handle_client(int client_socket) {
    char buffer[128];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        perror("ERROR: Failed to receive message from client");
        close(client_socket);
        return;
    }
    
    buffer[bytes_received] = '\0';  // Null-terminate the received string

    // Check for ping message from the coordinator
    if (strcmp(buffer, "PING:ROUND_COMPLETED") == 0) {
        printf("Received ping from coordinator for round %d.\n", current_round);
        printf("==============================================================\n");

        send(client_socket, OK, strlen(OK), 0);
        reset_round();
        close(client_socket);
        return;
    }

    int participant_id;
    if (sscanf(buffer, "ALLOW_FAIL:%d", &participant_id) == 1) {
        // Allow failure only if it's the first failure in the current round
        printf("Received fail request from participant %d\n", participant_id);
        if (failures_in_round < 1) {
            // Identify the participant by ID
            for (int i = 0; i < MAX_PARTICIPANTS; i++) {
                if (!participants[i].failed && i == participant_id) { 
                    participants[i].failed = 1; // Mark this participant as failed
                    failures_in_round++;        // Increment the failure count
                    
                    send(client_socket, OK, strlen(OK), 0);
                    close(client_socket);
                    return;
                }
            }
        }
        // If all participants have failed
        send(client_socket, FAILING, strlen(FAILING), 0);
    }

    close(client_socket);
}



int main() {
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    initialize_participants();

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(MANAGER_PORT);

    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, 3);

    printf("=== Failure Manager is running ===\n");

    while (1) {
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("ERROR: Failed to accept connection\n");
            continue;
        }
        printf("Connection Received\n");
        handle_client(client_socket);
    }

    close(server_socket);
    return 0;
}

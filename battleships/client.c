#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "grid.h"

#define LOCAL_BUFFER_SIZE 2028
#define SOCKET_PC 5088

void deserialize_grid(Grid *grid, const char *buffer) {
    memcpy(grid, buffer, sizeof(Grid));
}

int request_random_layout(int server_fd, Grid *grid) {
    if (write(server_fd, "random", strlen("random") + 1) == -1) {
        perror("Failed to request random layout from server");
        return -1;
    }

    if (read(server_fd, grid, sizeof(Grid)) == -1) {
        perror("Failed to receive random layout from server");
        return -1;
    }

    printf("Received new random layout from server.\n");
    return 0;
}

void *run_client(shared_names *names) {
    synchronized_buffer buff;
    syn_shm_buffer_open(&buff, names);

    int server_fd = connect_to_server("localhost", SOCKET_PC); 
    if (server_fd < 0) {
        perror("Failed to connect to server");
        printf("%d\n", server_fd);
        syn_shm_buffer_close(&buff);
        exit(EXIT_FAILURE);
    } 

    printf("Connected to the server. Enter your moves.\n");
    char buffer[LOCAL_BUFFER_SIZE];

    
    Grid temp_grid_my, opponent_grid;
    init_grid(&temp_grid_my);

    char command[256];
    while (1) {
        draw_grid(&temp_grid_my, NULL);

        printf("Enter 'random' to generate new ship layout or 'accept' to confirm:\n");
        if (fgets(command, sizeof(command), stdin) == NULL) {
            printf("Input error or EOF. Exiting...\n");
            break;
        }

        if (strncmp(command, "random", 6) == 0) {
            if (request_random_layout(server_fd, &temp_grid_my) != 0) {
                printf("Failed to get random layout from server. Exiting...\n");
                break;
            }
        } else if (strncmp(command, "accept", 6) == 0) {
            if (write(server_fd, "accept", strlen("accept") + 1) == -1) {
                perror("Failed to send acceptance to server");
                break;
            }
            printf("Grid accepted.\n");
            break;
        } else {
            printf("Invalid command. Use 'random' or 'accept'.\n");
        }
    }

    init_grid(&opponent_grid);

    while (1) {
        game_action action;
        syn_shm_buffer_pop(&buff, &action);
        opponent_grid = action.opponent;
        temp_grid_my = action.ships;

        draw_grid(&opponent_grid, &temp_grid_my);

        printf("Enter action type (0=move, 1=attack), x, y:\n");
        if (scanf("%d %d %d", &action.action_type, &action.x, &action.y) != 3) {
            printf("Invalid input or EOF. Exiting...\n");
            break;
        }

        if (write(server_fd, &action, sizeof(game_action)) == -1) {
            perror("Failed to send data to server");
            break;
        }

    }

    close(server_fd);
    syn_shm_buffer_close(&buff);
    active_socket_destroy(server_fd);

    return NULL;
}

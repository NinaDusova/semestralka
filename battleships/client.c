#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "grid.h"

#define LOCAL_BUFFER_SIZE 2028
#define SOCKET_PC 5089

void deserialize_grid(Grid *grid, const char *buffer) {
    memcpy(grid, buffer, sizeof(Grid));
}

bool readAction(game_action *action) {
    char temp_x;
    printf("Enter action type x, y [A1]:\n");

    if (scanf(" %c%d", &temp_x, &action->y) != 2) {
        printf("Invalid input or EOF.\n");
        return false; 
    }

    // Overenie a konverzia znaku
    if (temp_x >= 'A' && temp_x <= 'J') {
        action->x = temp_x - 'A' + 1;
        return true; 
    } else {
        printf("Invalid input for x. Must be a letter between A and J.\n");
        return false;
    }

    if (action->y >= 1 && action->y <= 10) {
        return true; 
    } else {
        printf("Invalid input for y. Must be a number between 1 and 10.\n");
        return false;
    }
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
        syn_shm_buffer_close(&buff);
        exit(EXIT_FAILURE);
    } 

    int client_id;
    if (read(server_fd, &client_id, sizeof(client_id)) == -1) {
        perror("Failed to receive client ID");
        close(server_fd);
        syn_shm_buffer_close(&buff);
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server. Enter your moves.\n");


    Grid temp_grid_my, opponent_grid;
    init_grid(&temp_grid_my);

    char command[256];
    while (1) {
        draw_grid(&temp_grid_my, NULL);

        printf("[PLAYER%d]Enter 'random' to generate new ship layout or 'accept' to confirm:\n", client_id + 1);
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
    printf("Grid initialized.\n");
    ssize_t bytes_read;

    while (1) {
        char buffer[LOCAL_BUFFER_SIZE];
        game_action action;

        ssize_t bytes_read = read(server_fd, buffer, sizeof(buffer));
        if (bytes_read <= 0) {
            printf("Failed to receive confirmation from server.\n");
            break;
        }
        
        //printf("Server response: %s\n", buffer); 
        //printf(".");
        buffer[bytes_read] = '\0';

        if (strcmp(buffer, "wait") == 0) {  // Správne porovnanie
            continue;
        }   else if (strcmp(buffer, "go") == 0) {
            if (client_id == 0) {
                syn_shm_buffer_pop(&buff, &action);
                action.ships = temp_grid_my;
                action.opponent = opponent_grid;
                draw_grid(&temp_grid_my, &opponent_grid);
                printf("AAAAAAAAAAAAAAAAAA.\n");
            }
        }/* else if (strcmp(buffer, "end") == 0) {
            printf("Game ended.\n");
            break;
        } */
        printf("%d Response from server: %s\n",client_id,  buffer);

       /* bytes_read = read(server_fd, &action, sizeof(game_action));
        if (bytes_read < 1) {
            perror("Failed to receive action from server");
            break;
        }*/

        while (true) {
            if (readAction(&action)) {
            break;
        }
            printf("Please try again.\n");
        }

        
        if (write(server_fd, &action, sizeof(game_action)) == -1) {
            perror("Failed to send data to server");
            break;
        }

        // Získať akciu zo servera cez buffer
        syn_shm_buffer_pop(&buff, &action);
        printf("Action popped from buffer.\n");
        
        // Aktualizovať mriežky podľa akcie
        opponent_grid = action.opponent;
        temp_grid_my = action.ships;

        switch (action.result) {
            case 0:
                printf("----------------\nHit.\n----------------\n");
                break;
            case 1:
                printf("----------------\nMiss.\n----------------\n");
                break;
            case 2:
                draw_grid(&temp_grid_my, &opponent_grid);
                printf("----------------\nYOU WIN. C:\n----------------\n");
                printf("W       W   IIIII   N     N\n");
                printf("W       W     I     NN    N\n");
                printf("W   W   W     I     N N   N\n");
                printf("W  W W  W     I     N  N  N\n");
                printf("W W   W W     I     N   N N\n");
                printf("WW     WW     I     N    NN\n");
                printf("W       W   IIIII   N     N\n");
                goto end_game;
            default:
                if (write(server_fd, &action, sizeof(game_action)) == -1) {
                perror("Failed to send data to server");
                break;
                }
                draw_grid(&temp_grid_my, &opponent_grid);
                printf("----------------\nYOU LOSE :C.\n----------------\n");
                printf("L         OOOOO   SSSSS   EEEEE\n");
                printf("L        O     O  S       E    \n");
                printf("L        O     O   SSSSS   EEEE \n");
                printf("L        O     O        S  E    \n");
                printf("LLLLLLL   OOOOO   SSSSS   EEEEE\n");
                goto end_game;
        }
        
        draw_grid(&temp_grid_my, &opponent_grid);

        printf("Wait for respond of other player.\n");
    }
end_game:

    close(server_fd);
    syn_shm_buffer_close(&buff);
    active_socket_destroy(server_fd);

    return NULL;
}

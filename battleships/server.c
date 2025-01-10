#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "grid.h"

#define LOCAL_BUFFER_SIZE 2028
#define SOCKET_PC 5088

bool shoot_at(Grid *g, int column_letter, int row_number) {
    int col = column_letter - 1;
    int row = row_number - 1;
    printf("Zadali ste stĺpec %d a riadok %d.\n", column_letter, row_number);

    if (row >= 0 && row < ROWS && col >= 0 && col < COLS) {
        if (g->grid[row][col].occupied) {
            printf("Zasiahli ste cieľ na %d %d!\n", column_letter, row_number);
            g->grid[row][col].symbol = 'X';
            return true;
        } else {
            printf("Netrafili ste, zadané miesto %d %d je prázdne.\n", column_letter, row_number);
            g->grid[row][col].symbol = 'O';
            return false;
        }
    } else {
        printf("Neplatné miesto: %d%d.\n", column_letter, row_number);
        return false;
    }
}

void process_action(game_action * action, Grid * grid) {
    if (action->action_type == 0) { 
        printf("Processing move action at (%d, %d)\n", action->x, action->y);
    } else if (action->action_type == 1) { 
        printf("Processing attack action at (%d, %d)\n", action->x, action->y);
        shoot_at(grid, action->x, action->y);
    }
}

void serialize_grid(const Grid *grid, char *buffer) {
    memcpy(buffer, grid, sizeof(Grid));
}

void handle_client(int client_fd, Grid * player_grid) {

    char command[256];
    while (1) {
        if (read(client_fd, command, sizeof(command)) <= 0) {
            printf("Client disconnected.\n");
            break;
        }

        if (strncmp(command, "random", 6) == 0) {
            randomly_place_ships(player_grid);
            if (write(client_fd, player_grid, sizeof(Grid)) == -1) {
                perror("Failed to send grid to client");
                break;
            }
        } else if (strncmp(command, "accept", 6) == 0) {
            printf("Player confirmed grid.\n");
            break;
        } else {
            printf("Unknown command from client: %s\n", command);
        }
    }
}


void *run_server(shared_names *names) {
    synchronized_buffer buff;
    syn_shm_buffer_open(&buff, names);

    int fd_passive = passive_socket_init(SOCKET_PC);

    Grid player1_grid, player2_grid;
    init_grid(&player1_grid);
    init_grid(&player2_grid);

    randomly_place_ships(&player1_grid);
    randomly_place_ships(&player2_grid);

    game_action action1 = {0}, action2 = {0};
    action1.player_id = 1;
    action1.ships = player1_grid;
    action1.opponent = player2_grid;

    action2.player_id = 2;
    action2.ships = player2_grid;
    action2.opponent = player1_grid;

    syn_shm_buffer_push(&buff, &action1);
    syn_shm_buffer_push(&buff, &action2);

    printf("Waiting for players to connect...\n");

    int fd_active1 = passive_socket_wait_for_client(fd_passive);
    if (fd_active1 < 0) {
        perror("Failed to accept player 1");
        close(fd_passive);
        exit(EXIT_FAILURE);
    }
    printf("Player 1 connected.\n");

    int fd_active2 = passive_socket_wait_for_client(fd_passive);
    if (fd_active2 < 0) {
        perror("Failed to accept player 2");
        close(fd_active1);
        close(fd_passive);
        exit(EXIT_FAILURE);
    }
    printf("Player 2 connected.\n");

    printf("Setting up Player 1's grid...\n");
    handle_client(fd_active1, &player1_grid);

    printf("Setting up Player 2's grid...\n");
    handle_client(fd_active2, &player2_grid);

    int current_player = 1;

    Grid *current_grid, *opponent_grid;
    while (1) {
        ssize_t bytes_read;
        game_action *current_action;
        int fd_current, fd_opponent;

        if (current_player == 1) {
            current_action = &action1;
            current_grid = &player1_grid;
            opponent_grid = &player2_grid;
            fd_current = fd_active1;
            fd_opponent = fd_active2;
        } else {
            current_action = &action2;
            current_grid = &player2_grid;
            opponent_grid = &player1_grid;
            fd_current = fd_active2;
            fd_opponent = fd_active1;
        }

        bytes_read = read(fd_current, current_action, sizeof(game_action));
        if (bytes_read > 0) {
            printf("Player %d action: type=%d, x=%d, y=%d\n", 
                   current_action->player_id, 
                   current_action->action_type, 
                   current_action->x, 
                   current_action->y);

            process_action(current_action, opponent_grid);

            if(current_player == 1) {
                action1.ships = player1_grid;
                action1.opponent = player2_grid;
            } else {
                action2.ships = player2_grid;
                action2.opponent = player1_grid;
            }

            syn_shm_buffer_push(&buff, current_action);

            printf("Player %d grid updated and sent.\n", current_player);

            current_player = (current_player == 1) ? 2 : 1;
        } else if (bytes_read <= 0) {
            printf("Player %d disconnected.\n", current_player);
            break;
        }
    }

    close(fd_active1);
    close(fd_active2);
    close(fd_passive);
    syn_shm_buffer_close(&buff);

    return NULL;
}

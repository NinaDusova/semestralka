#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "grid.h"

#define LOCAL_BUFFER_SIZE 2028
#define SOCKET_PC 5088

bool shoot_at(Grid *g, int column_letter, int row_number, int action_type) {
    int col = column_letter - 1;
    int row = row_number - 1;
    if (action_type == 2) {
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
        }   else {
            printf("Neplatné miesto: %d%d.\n", column_letter, row_number);
            return false;
        }
    } else {
        if (row >= 0 && row < ROWS && col >= 0 && col < COLS) {
            if (action_type == 0) {
                g->grid[row][col].symbol = 'X';
                return true;
            } else {
                g->grid[row][col].symbol = 'O';
                return false;
            }
        } else {
            return false;
        }
    }
}

int process_action(game_action * action, Grid * grid, int action_type) {
    printf("Processing attack action at (%d, %d)\n", action->x, action->y);

    if (shoot_at(grid, action->x, action->y, action_type)) {
        if (has_exactly_20_X(grid)) {
            return 2;
        }
        return 0;
    } else {
        return 1;
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

    Grid player1_grid, player2_grid, check_grid1, check_grid2;
    init_grid(&player1_grid);
    init_grid(&player2_grid);
    init_grid(&check_grid1);
    init_grid(&check_grid2);

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

    game_action action1 = {0}, action2 = {0};
    action1.player_id = 1;
    action1.ships = player1_grid;
    action1.opponent = check_grid1;
    

    action2.player_id = 2;
    action2.ships = player2_grid;
    action2.opponent = check_grid2;

    syn_shm_buffer_push(&buff, &action1);
    //syn_shm_buffer_push(&buff, &action2);

    Grid *current_grid, *opponent_grid;
    current_grid = &player1_grid;
    opponent_grid = &check_grid1;
    Grid * check_grid = &player2_grid;
    int result_num_before = 0;

    while (1) {
        ssize_t bytes_read;
        game_action *current_action;
        int fd_current, fd_opponent;

        if (current_player == 1) {
            current_action = &action1;
            current_grid = &player1_grid;
            opponent_grid = &check_grid1;
            fd_current = fd_active1;
            fd_opponent = fd_active2;
        } else {
            current_action = &action2;
            current_grid = &player2_grid;
            opponent_grid = &check_grid2;
            fd_current = fd_active2;
            fd_opponent = fd_active1;
        }

        current_action->player_id = current_player;
        if (write(fd_current, current_action, sizeof(game_action)) == -1) {
            perror("Failed to send action to player");
            break;
        }

        bytes_read = read(fd_current, current_action, sizeof(game_action));
        if (bytes_read > 0) {
            printf("Player %d action: type=%d, x=%d, y=%d\n", 
                   current_action->player_id, 
                   current_action->action_type, 
                   current_action->x, 
                   current_action->y);

            int result_num = process_action(current_action, check_grid, 2);
            process_action(current_action, opponent_grid, result_num);

            if(current_player == 1) {
                action1.player_id = 2;
                action1.ships = player2_grid;
                action1.opponent = check_grid2;
                action1.result = result_num_before;
            } else {
                action2.player_id = 1;
                action2.ships = player1_grid;
                action2.opponent = check_grid1;
                action2.result = result_num_before;
            }
            
            printf("Akcia z minule %d akcia teraz %d\n", result_num_before, result_num);
            if (result_num == 2) {
                current_action->result = 2;
                syn_shm_buffer_push(&buff, current_action);
                current_action->result = 3;
                syn_shm_buffer_push(&buff, current_action);
                printf("--------------\nPLAYER %d WON !!.\n--------------\n", current_player);

                break;
            }

            result_num_before = result_num;

            syn_shm_buffer_push(&buff, current_action);

            printf("Player %d grid updated and sent.\n --------------------------\n", current_player);
            
            check_grid = current_player == 1 ? &player1_grid : &player2_grid;

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

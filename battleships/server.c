#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "grid.h"
#include "syn_buffer.h"

#define LOCAL_BUFFER_SIZE 2028
#define SOCKET_PC 5088

typedef struct {
    int client_fd;
    int client_id;
    Grid *player_grid;
    Grid * check_grid;
    Grid *opponent_grid;
    int * turn;
    shared_names *names;
    synchronized_buffer *buff;
} client_info_t;

bool shoot_at(Grid *g, int column_letter, int row_number, int action_type) {
    int col = column_letter - 1;
    int row = row_number - 1;
    printf("Zadali ste stĺpec %d a riadok %d %d .\n", column_letter, row_number, action_type);
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
        } else {
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

int process_action(game_action *action, Grid *grid, int action_type) {
    printf("Processing attack action at (%d, %d)\n", action->x, action->y);

    if (shoot_at(grid, action->x, action->y, action_type)) {
        if (has_exactly_20_X(grid)) {
            return 2;  // Víťazstvo
        }
        return 0;  // Pokračovanie
    } else {
        return 1;  // Neúspešný pokus
    }
}

void serialize_grid(const Grid *grid, char *buffer) {
    memcpy(buffer, grid, sizeof(Grid));
}

// Funkcia pre spracovanie klienta
void *handle_client(void *arg) {
    client_info_t *client_info = (client_info_t *)arg;
    int client_fd = client_info->client_fd;
    int client_id = client_info->client_id;
    Grid *player_grid = client_info->player_grid;
    Grid *opponent_grid = client_info->opponent_grid;
    shared_names *names = client_info->names;
    synchronized_buffer *buff = client_info->buff;


    if (write(client_fd, &client_id, sizeof(client_id)) == -1) {
        perror("Failed to send client ID");
        close(client_fd);
        free(client_info);
        return NULL;
    }

    char command[256];
    ssize_t bytes_read;

    printf("Handling client (fd: %d)...\n", client_fd);

    while (1) {
        bytes_read = read(client_fd, command, sizeof(command));
        if (bytes_read <= 0) {
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
            printf("Player %d confirmed grid.\n", client_fd);
            if (write(client_fd, "Grid accepted.", strlen("Grid accepted.") + 1) == -1) {
                perror("Failed to send grid accepted message to client");
                break;
            }
            printf("Grid accepted message sent to client.\n");
            break;
        } else {
            printf("Unknown command from client: %s\n", command);
        }
    }

    game_action action;
    action.player_id = client_id;
    action.ships = *client_info->player_grid;
    action.opponent = *client_info->check_grid;

    Grid check_grid;
    check_grid = *opponent_grid;

    syn_shm_buffer_push(buff, &action);

    while (1) {
        if (*client_info->turn != client_id) {
        if (write(client_fd, "wait", strlen("wait") + 1) == -1) {
            perror("Failed to send wait message");
            break;
        }
        usleep(100000);
        continue;
        }

        
        /*if (write(client_fd, &action, sizeof(game_action)) == -1) {
            perror("Failed to send action to player");
            break;
        }*/

        bytes_read = read(client_fd, &action, sizeof(game_action));
        if (bytes_read <= 0) {
            printf("Client disconnected.\n");
            break;
        }

        printf("Player %d: x=%d, y=%d\n", action.player_id + 1 , action.x, action.y);

        //int result_num = process_action(&action, &action.opponent, 2);
        //int result_num = process_action(&action, &check_grid, 2);
        int result_num = process_action(&action, client_info->opponent_grid, 2);
        process_action(&action, client_info->check_grid, result_num);

        action.ships = *client_info->player_grid;
        action.opponent = *client_info->check_grid;
        action.result = result_num;

        if (write(client_fd, &action, sizeof(game_action)) == -1) {
            perror("Failed to send action result to client");
            break;
        }
        if (result_num == 2) {
            printf("Player %d wins!\n", action.player_id);
            syn_shm_buffer_push(buff, &action);
            action.result = 3;
            syn_shm_buffer_push(buff, &action);
            break;
        }

        // Prepnutie ťahu na druhého hráča
        *client_info->turn = (*client_info->turn == 0) ? 1 : 0;

        printf("Turn switched. Now it's player %d's turn.\n", *client_info->turn);
        
        syn_shm_buffer_push(buff, &action);
        printf("Action pushed to buffer.\n");

        printf("------------------\n");
    }

    close(client_fd);
    free(client_info);
    printf("Client thread for player %d finished.\n", action.player_id);
    return NULL;
}

void *run_server(shared_names *names) {
    synchronized_buffer buff;
    syn_shm_buffer_open(&buff, names);

    // Inicializácia mutexu pre synchronizáciu
    pthread_mutex_init(&buff.mutex, NULL);

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

    int turn = 0;

    client_info_t *client1_info = malloc(sizeof(client_info_t));
    client1_info->client_fd = fd_active1;
    client1_info->client_id = 0;
    client1_info->player_grid = &player1_grid;
    client1_info->opponent_grid = &player2_grid;
    client1_info->check_grid = &check_grid1;
    client1_info->names = names;
    client1_info->buff = &buff;
    client1_info->turn = &turn;

    client_info_t *client2_info = malloc(sizeof(client_info_t));
    client2_info->client_fd = fd_active2;
    client2_info->client_id = 1;
    client2_info->player_grid = &player2_grid;
    client2_info->opponent_grid = &player1_grid;
    client2_info->check_grid = &check_grid2;
    client2_info->names = names;
    client2_info->buff = &buff;
    client2_info->turn = &turn;

    pthread_t player1_thread, player2_thread;

    if (pthread_create(&player1_thread, NULL, handle_client, client1_info) != 0) {
        perror("Failed to create thread for player 1");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&player2_thread, NULL, handle_client, client2_info) != 0) {
        perror("Failed to create thread for player 2");
        exit(EXIT_FAILURE);
    }

    pthread_join(player1_thread, NULL);
    pthread_join(player2_thread, NULL);

    close(fd_active1);
    close(fd_active2);
    close(fd_passive);

    pthread_mutex_destroy(&buff.mutex);

    syn_shm_buffer_close(&buff);

    return NULL;
}

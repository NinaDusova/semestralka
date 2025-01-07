#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h> 
#include <pthread.h>
#include <string.h>

#include "syn_buffer.h"
#include "../sockets-lib/socket.h"

#define ROWS 10
#define COLS 10

typedef struct {
    bool occupied;
    char symbol;
} Cell;

typedef struct {
    Cell grid[ROWS][COLS];
} Grid;

void init_grid(Grid *g) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            g->grid[i][j].occupied = false;
            g->grid[i][j].symbol = '.';
        }
    }
}

void draw_grid(Grid *g) {
    printf("   ");
    for (int j = 0; j < COLS; j++) {
        printf("%2c ", 'A' + j);
    }
    printf("\n");

    for (int i = 0; i < ROWS; i++) {
        printf("%2d ", i + 1);

        for (int j = 0; j < COLS; j++) {
            printf("%2c ", g->grid[i][j].symbol);  // Zarovnanie políčok
        }

        printf("\n");
    }
}

int letter_to_column(char letter) {
    return tolower(letter) - 'a';
}

bool shoot_at(Grid *g, char column_letter, int row_number) {
    int col = letter_to_column(column_letter);
    int row = row_number - 1;

    if (row >= 0 && row < ROWS && col >= 0 && col < COLS) {
        if (g->grid[row][col].occupied) {
            printf("Zasiahli ste cieľ na %c%d!\n", column_letter, row_number);
            g->grid[row][col].symbol = 'X';
            return true;
        } else {
            printf("Netrafili ste, zadané miesto %c%d je prázdne.\n", column_letter, row_number);
            g->grid[row][col].symbol = 'O';
            return false;
        }
    } else {
        printf("Neplatné miesto: %c%d.\n", column_letter, row_number);
        return false;
    }
}

char * add_suffix(const char * name, const char * suffix) {
  size_t nameLen = strlen(name);
  size_t suffixLen = strlen(suffix);
  char * result = calloc(nameLen + suffixLen + 2, sizeof(char));
  strcpy(result, name);
  result[nameLen] = '-';
  strcpy(result + nameLen + 1, suffix);
  return result;
}

void clear_names(shared_names * names) {
  free(names->shm_name_);
  free(names->mut_pc_);
  free(names->sem_produce_);
  free(names->sem_consume_);
}

void user_input(Grid *g) {
    char column;
    int row;

    while (true) {
        printf("Zadajte výstrel (napr. D3) alebo 'q' pre ukončenie: ");
        
        if (scanf(" %c%d", &column, &row) == 2) {
            // Skontrolujeme platnosť vstupu
            if (row < 1 || row > ROWS || tolower(column) < 'a' || tolower(column) >= 'a' + COLS) {
                printf("Neplatný vstup! Skúste znova.\n");
                continue;
            }

            bool hit = shoot_at(g, column, row);
            if (hit) {
                draw_grid(g);
                break;
            }
            draw_grid(g);
        } else {
            char input[10];
            if (fgets(input, sizeof(input), stdin) == NULL) {
                printf("Chyba pri čítaní vstupu.\n");
                continue;
            } 
            if (input[0] == 'q' || input[0] == 'Q') {
                printf("Ukončujeme hru.\n");
                break;
            }
            printf("Neplatný formát! Zadajte písmeno stĺpca a číslo riadku (napr. D3).\n");
        }
    }
}

int main() {
    const char *user = getenv("USER");

    shared_names names; 
    names.shm_name_ = add_suffix("DIJKSTRA-SHM", user);
    names.mut_pc_ = add_suffix("MUT-PC", user);
    names.sem_produce_ = add_suffix("SEM-P", user);
    names.sem_consume_ = add_suffix("SEM-C", user);

    Grid g;
    init_grid(&g);

    g.grid[2][3].occupied = true;  // Umiestnenie objektu na D3 (stĺpec D, riadok 3)
    g.grid[4][5].occupied = true;  // Umiestnenie objektu na F5 (stĺpec F, riadok 5

    draw_grid(&g);

    user_input(&g);

    clear_names(&names);
    return 0;
}
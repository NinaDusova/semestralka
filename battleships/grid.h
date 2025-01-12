#ifndef GRID_H
#define GRID_H

#include "buffer.h"

static inline bool has_exactly_20_X(Grid *grid) {
    int count = 0; 
    
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (grid->grid[i][j].occupied && grid->grid[i][j].symbol == 'X') {
                count++;
            }
            if (count > 4) {
                return false;
            }
        }
    }

    return count == 4;
}

static inline bool can_place_ship(const Grid *g, int row, int col, int length, bool horizontal) {
    if (horizontal) {
        if (col + length > COLS) return false;
        for (int i = 0; i < length; i++) {
            if (g->grid[row][col + i].occupied) return false;
        }
    } else {
        if (row + length > ROWS) return false;
        for (int i = 0; i < length; i++) {
            if (g->grid[row + i][col].occupied) return false;
        }
    }
    return true;
}

static inline void place_ship(Grid *g, int length) {
    bool placed = false;
    while (!placed) {
        int row = rand() % ROWS;
        int col = rand() % COLS;
        bool horizontal = rand() % 2;

        if (can_place_ship(g, row, col, length, horizontal)) {
            for (int i = 0; i < length; i++) {
                if (horizontal) {
                    g->grid[row][col + i].occupied = true;
                    g->grid[row][col + i].symbol = '#';
                } else {
                    g->grid[row + i][col].occupied = true;
                    g->grid[row + i][col].symbol = '#';
                }
            }
            placed = true;
        }
    }
}

static inline void clear_grid(Grid *g) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            g->grid[i][j].occupied = false;
            g->grid[i][j].symbol = '.';  
        }
    }
}

static inline void randomly_place_ships(Grid *g) {
    clear_grid(g);
    place_ship(g, 4); 
    //for (int i = 0; i < 2; i++) place_ship(g, 3);
    //for (int i = 0; i < 3; i++) place_ship(g, 2);
    //for (int i = 0; i < 4; i++) place_ship(g, 1);
}

static inline void init_grid(Grid *g) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            g->grid[i][j].occupied = false;
            g->grid[i][j].symbol = '.';
        }
    }
}

static inline void draw_grid(const Grid *g1, const Grid *g2) {
    if (g1 == NULL) {
        printf("Error: g1 is NULL.\n");
        return;
    }

    printf("\033[1;34m");
    printf("   ");
    for (int j = 0; j < COLS; j++) {
        printf("%2c ", 'A' + j);
    }
    printf("\n");

    for (int i = 0; i < ROWS; i++) {
        printf("%2d ", i + 1);

        for (int j = 0; j < COLS; j++) {
            printf("%2c ", g1->grid[i][j].symbol);
        }
        printf("\n");
    }

    if (g2 == NULL) {
        printf("\033[0m\n");
        return;
    }

    printf("\033[0m\n");

    printf("\033[1;31m");
    printf("   ");
    for (int j = 0; j < COLS; j++) {
        printf("%2c ", 'A' + j);
    }
    printf("\n");

    for (int i = 0; i < ROWS; i++) {
        printf("%2d ", i + 1);

        for (int j = 0; j < COLS; j++) {
            printf("%2c ", g2->grid[i][j].symbol);
        }
        printf("\n");
    }

    printf("\033[0m\n");
}

#endif // GRID_H
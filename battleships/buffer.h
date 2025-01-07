#ifndef BUFFER
#define BUFFER
#include <stddef.h>

#define BUFFER_CAPACITY 3

typedef struct game_action {
    int action_type;
    int player_id;
    int x;
    int y;
    int result;
    char ships[10][10]; //plocha s lodami
} game_action;

typedef struct buffer {
    game_action data_[BUFFER_CAPACITY];
    size_t capacity_;
    size_t size_;
    size_t in_;
    size_t out_;
} buffer;

void buffer_init(buffer * this);
void buffer_destroy(buffer * this);
void buffer_push(buffer * this, const game_action * input);
void buffer_pop(buffer * this, game_action * output);

#endif
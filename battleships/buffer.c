#include "buffer.h"

void buffer_init(buffer * this) {
  this->capacity_ = BUFFER_CAPACITY;
  this->size_ = 0;
  this->in_ = 0;
  this->out_ = 0;
}

void buffer_destroy(buffer * this) {
}

void buffer_push(buffer * this, const game_action * input) {
  this->data_[this->in_++] = *input;
  this->size_++;
  this->in_ %= this->capacity_;
}

void buffer_pop(buffer * this, game_action * output) {
  *output = this->data_[this->out_++];
  this->size_--;
  this->out_ %= this->capacity_;
}
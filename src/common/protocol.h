#ifndef CHAT_PROTOCOL_H
#define CHAT_PROTOCOL_H

#include <stddef.h>

#define CHAT_INPUT_CAPACITY 4096
#define CHAT_LINE_CAPACITY 1024

typedef struct chat_input_buffer {
  char data[CHAT_INPUT_CAPACITY];
  size_t used;
} chat_input_buffer;

int chat_send_all(int fd, const void *data, size_t length);
int chat_send_text(int fd, const char *text);
int chat_buffer_append(chat_input_buffer *buffer, const char *data, size_t length);
int chat_buffer_next_line(chat_input_buffer *buffer, char *line, size_t capacity);

#endif

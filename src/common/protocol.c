#include "protocol.h"

#include <errno.h>
#include <string.h>
#include <sys/socket.h>

int
chat_send_all(int fd, const void *data, size_t length)
{
  const char *cursor = data;
  size_t sent = 0;

  while(sent < length){
    ssize_t result = send(fd, cursor + sent, length - sent, 0);
    if(result > 0){
      sent += (size_t)result;
      continue;
    }
    if(result < 0 && errno == EINTR)
      continue;
    return -1;
  }

  return 0;
}

int
chat_send_text(int fd, const char *text)
{
  return chat_send_all(fd, text, strlen(text));
}

int
chat_buffer_append(chat_input_buffer *buffer, const char *data, size_t length)
{
  if(length > CHAT_INPUT_CAPACITY - buffer->used)
    return -1;

  memcpy(buffer->data + buffer->used, data, length);
  buffer->used += length;
  return 0;
}

int
chat_buffer_next_line(chat_input_buffer *buffer, char *line, size_t capacity)
{
  size_t line_length;
  char *newline = memchr(buffer->data, '\n', buffer->used);

  if(newline == NULL)
    return 0;

  line_length = (size_t)(newline - buffer->data) + 1;
  if(line_length + 1 > capacity)
    return -1;

  memcpy(line, buffer->data, line_length);
  line[line_length] = '\0';
  memmove(buffer->data, buffer->data + line_length, buffer->used - line_length);
  buffer->used -= line_length;
  return 1;
}

#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void
require(int condition, const char *message)
{
  if(!condition){
    fprintf(stderr, "protocol test: FAIL - %s\n", message);
    exit(EXIT_FAILURE);
  }
}

static void
test_split_and_coalesced_lines(void)
{
  chat_input_buffer input = {{0}, 0};
  char line[CHAT_LINE_CAPACITY];
  const char remainder[] = "in 1\n/list\n";

  require(chat_buffer_append(&input, "/jo", 3) == 0, "append fragment");
  require(chat_buffer_next_line(&input, line, sizeof(line)) == 0,
          "fragment must remain buffered");
  require(chat_buffer_append(&input, remainder, sizeof(remainder) - 1) == 0,
          "append remaining and coalesced command");
  require(chat_buffer_next_line(&input, line, sizeof(line)) == 1,
          "first complete line");
  require(strcmp(line, "/join 1\n") == 0, "reassemble split command");
  require(chat_buffer_next_line(&input, line, sizeof(line)) == 1,
          "second complete line");
  require(strcmp(line, "/list\n") == 0, "separate coalesced command");
  require(chat_buffer_next_line(&input, line, sizeof(line)) == 0,
          "buffer empty after both lines");
}

static ssize_t
receive_exact(int fd, void *data, size_t length)
{
  char *cursor = data;
  size_t received = 0;

  while(received < length){
    ssize_t result = recv(fd, cursor + received, length - received, 0);
    if(result <= 0)
      return result;
    received += (size_t)result;
  }
  return (ssize_t)received;
}

static void
test_capacity_limit(void)
{
  chat_input_buffer input = {{0}, 0};
  char oversized[CHAT_INPUT_CAPACITY + 1];

  memset(oversized, 'x', sizeof(oversized));
  require(chat_buffer_append(&input, oversized, sizeof(oversized)) == -1,
          "reject input larger than buffer");
  require(input.used == 0, "failed append must not change buffer");
}

static void
test_send_all(void)
{
  int pair[2];
  char received[32] = {0};
  const char payload[] = "binary-safe\0payload";
  ssize_t count;

  require(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0, "create socket pair");
  require(chat_send_all(pair[0], payload, sizeof(payload)) == 0,
          "send complete payload");
  count = receive_exact(pair[1], received, sizeof(payload));
  require(count == (ssize_t)sizeof(payload), "receive exact payload length");
  require(memcmp(received, payload, sizeof(payload)) == 0,
          "payload bytes preserved");
  close(pair[0]);
  close(pair[1]);
}

int
main(void)
{
  test_split_and_coalesced_lines();
  test_capacity_limit();
  test_send_all();
  puts("protocol test: PASS");
  return EXIT_SUCCESS;
}

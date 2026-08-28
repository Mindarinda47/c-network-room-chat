#include "protocol.h"

#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_PORT 12345
#define MAX_CLIENTS 15
#define ROOM_COUNT 3
#define ROOM_CAPACITY 5

typedef struct client_state {
  int fd;
  unsigned int id;
  int room;
  chat_input_buffer input;
} client_state;

static volatile sig_atomic_t stop_requested;

static void
handle_signal(int signal_number)
{
  (void)signal_number;
  stop_requested = 1;
}

static void
initialize_clients(client_state clients[])
{
  size_t index;

  for(index = 0; index < MAX_CLIENTS; index++){
    clients[index].fd = -1;
    clients[index].id = 0;
    clients[index].room = -1;
    clients[index].input.used = 0;
  }
}

static int
room_population(const client_state clients[], int room)
{
  size_t index;
  int count = 0;

  for(index = 0; index < MAX_CLIENTS; index++){
    if(clients[index].fd >= 0 && clients[index].room == room)
      count++;
  }
  return count;
}

static void
send_menu(int fd)
{
  (void)chat_send_text(fd,
    "Commands: /list, /join <room>, /leave, /quit\n"
    "Join a room before sending chat messages.\n");
}

static void
send_room_list(int fd, const client_state clients[])
{
  int room;
  char response[256];
  size_t used = 0;

  used += (size_t)snprintf(response + used, sizeof(response) - used,
                          "Rooms:\n");
  for(room = 0; room < ROOM_COUNT && used < sizeof(response); room++){
    used += (size_t)snprintf(response + used, sizeof(response) - used,
                            "  %d: %d/%d\n", room,
                            room_population(clients, room), ROOM_CAPACITY);
  }
  (void)chat_send_text(fd, response);
}

static void
broadcast_room(const client_state clients[], int room, int excluded_fd,
               const char *message)
{
  size_t index;

  for(index = 0; index < MAX_CLIENTS; index++){
    if(clients[index].fd >= 0 && clients[index].fd != excluded_fd &&
       clients[index].room == room)
      (void)chat_send_text(clients[index].fd, message);
  }
}

static void
leave_room(client_state clients[], size_t index, int notify_client)
{
  char notice[128];
  int previous_room = clients[index].room;

  if(previous_room < 0){
    if(notify_client)
      (void)chat_send_text(clients[index].fd, "You are already in the lobby.\n");
    return;
  }

  clients[index].room = -1;
  (void)snprintf(notice, sizeof(notice), "[system] client %u left room %d.\n",
                 clients[index].id, previous_room);
  broadcast_room(clients, previous_room, clients[index].fd, notice);
  if(notify_client)
    (void)chat_send_text(clients[index].fd, "Moved to the lobby.\n");
}

static void
disconnect_client(client_state clients[], size_t index)
{
  if(clients[index].fd < 0)
    return;

  leave_room(clients, index, 0);
  close(clients[index].fd);
  clients[index].fd = -1;
  clients[index].id = 0;
  clients[index].room = -1;
  clients[index].input.used = 0;
}

static int
parse_room(const char *line, int *room)
{
  char trailing;
  int value;

  if(sscanf(line, "/join %d %c", &value, &trailing) != 1)
    return -1;
  if(value < 0 || value >= ROOM_COUNT)
    return -1;
  *room = value;
  return 0;
}

static int
handle_line(client_state clients[], size_t index, const char *line)
{
  char message[CHAT_LINE_CAPACITY + 64];
  int target_room;

  if(strcmp(line, "/quit\n") == 0)
    return -1;

  if(strcmp(line, "/list\n") == 0){
    send_room_list(clients[index].fd, clients);
    return 0;
  }

  if(strcmp(line, "/leave\n") == 0){
    leave_room(clients, index, 1);
    return 0;
  }

  if(strncmp(line, "/join", 5) == 0){
    if(parse_room(line, &target_room) < 0){
      (void)chat_send_text(clients[index].fd, "Usage: /join <0-2>\n");
      return 0;
    }
    if(room_population(clients, target_room) >= ROOM_CAPACITY){
      (void)chat_send_text(clients[index].fd, "That room is full.\n");
      return 0;
    }
    if(clients[index].room == target_room){
      (void)chat_send_text(clients[index].fd, "You are already in that room.\n");
      return 0;
    }

    leave_room(clients, index, 0);
    clients[index].room = target_room;
    (void)snprintf(message, sizeof(message),
                   "[system] client %u joined room %d.\n",
                   clients[index].id, target_room);
    broadcast_room(clients, target_room, clients[index].fd, message);
    (void)snprintf(message, sizeof(message), "Joined room %d.\n", target_room);
    (void)chat_send_text(clients[index].fd, message);
    return 0;
  }

  if(line[0] == '/'){
    (void)chat_send_text(clients[index].fd, "Unknown command.\n");
    send_menu(clients[index].fd);
    return 0;
  }

  if(clients[index].room < 0){
    (void)chat_send_text(clients[index].fd, "Join a room before chatting.\n");
    return 0;
  }

  (void)snprintf(message, sizeof(message), "[client %u] %s",
                 clients[index].id, line);
  broadcast_room(clients, clients[index].room, clients[index].fd, message);
  return 0;
}

static int
create_listener(unsigned short port)
{
  int listener;
  int reuse = 1;
  struct sockaddr_in address;

  listener = socket(AF_INET, SOCK_STREAM, 0);
  if(listener < 0)
    return -1;

  if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0){
    close(listener);
    return -1;
  }

  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(port);

  if(bind(listener, (struct sockaddr *)&address, sizeof(address)) < 0 ||
     listen(listener, MAX_CLIENTS) < 0){
    close(listener);
    return -1;
  }
  return listener;
}

static void
accept_client(int listener, client_state clients[], unsigned int *next_id)
{
  int fd = accept(listener, NULL, NULL);
  size_t index;

  if(fd < 0)
    return;

  for(index = 0; index < MAX_CLIENTS; index++){
    if(clients[index].fd < 0){
      clients[index].fd = fd;
      clients[index].id = (*next_id)++;
      clients[index].room = -1;
      clients[index].input.used = 0;
      (void)chat_send_text(fd, "Connected to the chat server.\n");
      send_menu(fd);
      return;
    }
  }

  (void)chat_send_text(fd, "Server is full.\n");
  close(fd);
}

static void
receive_client_data(client_state clients[], size_t index)
{
  char incoming[1024];
  char line[CHAT_LINE_CAPACITY];
  ssize_t received = recv(clients[index].fd, incoming, sizeof(incoming), 0);

  if(received <= 0){
    disconnect_client(clients, index);
    return;
  }

  if(chat_buffer_append(&clients[index].input, incoming, (size_t)received) < 0){
    (void)chat_send_text(clients[index].fd, "Input buffer exceeded.\n");
    disconnect_client(clients, index);
    return;
  }

  for(;;){
    int result = chat_buffer_next_line(&clients[index].input, line, sizeof(line));
    if(result == 0)
      break;
    if(result < 0 || handle_line(clients, index, line) < 0){
      disconnect_client(clients, index);
      break;
    }
  }
}

int
main(int argc, char *argv[])
{
  unsigned short port = DEFAULT_PORT;
  unsigned int next_id = 1;
  client_state clients[MAX_CLIENTS];
  int listener;

  if(argc > 2){
    fprintf(stderr, "Usage: %s [port]\n", argv[0]);
    return EXIT_FAILURE;
  }
  if(argc == 2){
    char *end;
    long parsed = strtol(argv[1], &end, 10);
    if(*end != '\0' || parsed < 1 || parsed > 65535){
      fprintf(stderr, "Invalid port.\n");
      return EXIT_FAILURE;
    }
    port = (unsigned short)parsed;
  }

  signal(SIGINT, handle_signal);
  signal(SIGTERM, handle_signal);
  signal(SIGPIPE, SIG_IGN);
  initialize_clients(clients);
  listener = create_listener(port);
  if(listener < 0){
    perror("create listener");
    return EXIT_FAILURE;
  }

  printf("Chat server listening on port %u.\n", (unsigned int)port);
  while(!stop_requested){
    fd_set readable;
    struct timeval timeout;
    int max_fd = listener;
    int ready;
    size_t index;

    FD_ZERO(&readable);
    FD_SET(listener, &readable);
    for(index = 0; index < MAX_CLIENTS; index++){
      if(clients[index].fd >= 0){
        FD_SET(clients[index].fd, &readable);
        if(clients[index].fd > max_fd)
          max_fd = clients[index].fd;
      }
    }

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    ready = select(max_fd + 1, &readable, NULL, NULL, &timeout);
    if(ready < 0){
      if(errno == EINTR)
        continue;
      perror("select");
      break;
    }
    if(ready == 0)
      continue;
    if(FD_ISSET(listener, &readable))
      accept_client(listener, clients, &next_id);
    for(index = 0; index < MAX_CLIENTS; index++){
      if(clients[index].fd >= 0 && FD_ISSET(clients[index].fd, &readable))
        receive_client_data(clients, index);
    }
  }

  for(size_t index = 0; index < MAX_CLIENTS; index++)
    disconnect_client(clients, index);
  close(listener);
  puts("Chat server stopped.");
  return EXIT_SUCCESS;
}

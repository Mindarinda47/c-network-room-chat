#include "protocol.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 12345

static int
write_all(int fd, const void *data, size_t length)
{
  const char *cursor = data;
  size_t written = 0;

  while(written < length){
    ssize_t result = write(fd, cursor + written, length - written);
    if(result > 0){
      written += (size_t)result;
      continue;
    }
    if(result < 0 && errno == EINTR)
      continue;
    return -1;
  }
  return 0;
}

static int
connect_to_server(const char *host, unsigned short port)
{
  int fd;
  struct sockaddr_in address;

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if(fd < 0)
    return -1;

  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  if(inet_pton(AF_INET, host, &address.sin_addr) != 1 ||
     connect(fd, (struct sockaddr *)&address, sizeof(address)) < 0){
    close(fd);
    return -1;
  }
  return fd;
}

int
main(int argc, char *argv[])
{
  const char *host = DEFAULT_HOST;
  unsigned short port = DEFAULT_PORT;
  int fd;

  if(argc > 3){
    fprintf(stderr, "Usage: %s [host] [port]\n", argv[0]);
    return EXIT_FAILURE;
  }
  if(argc >= 2)
    host = argv[1];
  if(argc == 3){
    char *end;
    long parsed = strtol(argv[2], &end, 10);
    if(*end != '\0' || parsed < 1 || parsed > 65535){
      fprintf(stderr, "Invalid port.\n");
      return EXIT_FAILURE;
    }
    port = (unsigned short)parsed;
  }

  fd = connect_to_server(host, port);
  if(fd < 0){
    perror("connect");
    return EXIT_FAILURE;
  }

  for(;;){
    fd_set readable;
    int max_fd = fd > STDIN_FILENO ? fd : STDIN_FILENO;
    int ready;

    FD_ZERO(&readable);
    FD_SET(fd, &readable);
    FD_SET(STDIN_FILENO, &readable);
    ready = select(max_fd + 1, &readable, NULL, NULL, NULL);
    if(ready < 0){
      if(errno == EINTR)
        continue;
      perror("select");
      break;
    }

    if(FD_ISSET(fd, &readable)){
      char buffer[1024];
      ssize_t received = recv(fd, buffer, sizeof(buffer), 0);
      if(received <= 0){
        puts("Server disconnected.");
        break;
      }
      if(write_all(STDOUT_FILENO, buffer, (size_t)received) < 0)
        break;
    }

    if(FD_ISSET(STDIN_FILENO, &readable)){
      char line[CHAT_LINE_CAPACITY];
      if(fgets(line, sizeof(line), stdin) == NULL){
        (void)chat_send_text(fd, "/quit\n");
        break;
      }
      if(strchr(line, '\n') == NULL){
        int character;
        while((character = getchar()) != '\n' && character != EOF)
          ;
        fprintf(stderr, "Input line is too long.\n");
        continue;
      }
      if(chat_send_text(fd, line) < 0)
        break;
      if(strcmp(line, "/quit\n") == 0)
        break;
    }
  }

  close(fd);
  return EXIT_SUCCESS;
}

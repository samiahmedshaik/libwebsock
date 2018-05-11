

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/select.h>
#include <websock/websock.h>

#define PORT 9001
#define MAXMSG 4 * 1024

void startprocessing(int socket)
{
  fd_set write_fd_set, read_fd_set;

  libwebsock_client_state *client_state = libwebsock_client_init();

  //client_state->

  while (1)
  {
    /* Initialize the set of active sockets. */
    FD_ZERO(&read_fd_set);
    FD_ZERO(&write_fd_set);
    FD_SET(socket, &read_fd_set);
    FD_SET(socket, &write_fd_set);

    /* Block until input arrives on one or more active sockets. */
    if (select(1, &read_fd_set, &write_fd_set, NULL, NULL) < 0)
    {
      perror("select failed");
      exit(EXIT_FAILURE);
    }

    if (FD_ISSET(socket, &read_fd_set))
    {
      /* Data arriving on an already-connected socket. */
      if (read_from_client(socket, client_state) < 0)
      {
        close(socket);
        break;
      }
    }

    if (FD_ISSET(socket, &write_fd_set))
    {
      /* Data to be sent on connected socket. */
      if (write_to_client(socket, client_state) < 0)
      {
        close(socket);
        break;
      }
    }
  }

  libwebsock_client_destroy(client_state);
}

int read_from_client(int socket, libwebsock_client_state *state)
{
  char buffer[MAXMSG] = {'\0'};
  int nbytes;

  nbytes = read(socket, buffer, MAXMSG);
  if (nbytes <= 0)
  {
    perror("read failed");
    return -1;
  }

  /* Data read. */
  if (state->flags & STATE_CONNECTING)
  {
    return libwebsock_populate_handshake(state, buffer, nbytes);
  }
  else if (libwebsock_handle_recv(state, buffer, nbytes) == -1)
  {
    if (state->flags & STATE_NEEDS_MORE_DATA == 0)
    {
      return -1;
    }
  }
  return 0;
}

int write_to_client(int socket, libwebsock_client_state *state)
{
  char buffer[MAXMSG];
  int nbytes;

  if (state->out_data)
  {
    nbytes = write(socket, buffer, nbytes);

    if (nbytes <= 0)
    {
      /* Write error. */
      perror("write failed");
      return -1;
    }
  }

  // echo the data back to client
  if (state->in_data)
  {

    libwebsock_message *msg = state->in_data;

    switch (msg->opcode)
    {
    case WS_OPCODE_TEXT:
      if (libwebsock_make_text_data_frame_with_length(state, msg->payload, msg->payload_len) == -1)
      {
        return -1;
      }
      break;
    case WS_OPCODE_BINARY:
      if (libwebsock_make_binary_data_frame(state, msg->payload, msg->payload_len) == -1)
      {
        return -1;
      }
      break;
    default:
      perror("Unknown opcode");
      return -1;
    }

    if (state->out_data)
    {
      nbytes = write(socket, buffer, nbytes);

      if (nbytes <= 0)
      {
        /* Write error. */
        perror("write failed");
        return -1;
      }
    }
  }

  return 0;
}

int main(int argc, char *argv[])
{
  int listening_socket, accepted_socket, i, client_size, port;
  struct sockaddr_in client_addr, server_addr;
  pid_t pid;

  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s <port to listen on>\n\nNote: You must be root to bind to port below 1024\n", argv[0]);
    exit(0);
  }

  port = atoi(argv[1]);

  /* Create the socket and set it up to accept connections. */
  listening_socket = socket(PF_INET, SOCK_STREAM, 0);
  if (listening_socket < 0)
  {
    perror("failed to create listening socket");
    exit(1);
  }

  /* Give the socket a name. */
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(listening_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    perror("bind failed");
    exit(1);
  }

  if (listen(listening_socket, 5) < 0)
  {
    perror("listen failed");
    exit(1);
  }

  client_size = sizeof(client_addr);

  while (1)
  {
    accepted_socket = accept(listening_socket, (struct sockaddr *)&client_addr, &client_size);

    if (accepted_socket < 0)
    {
      perror("accepting client connecton failed");
      exit(1);
    }

    /* Create child process */
    pid = fork();

    if (pid < 0)
    {
      perror("failed to fork");
      exit(1);
    }

    if (pid == 0)
    {

      /* This is the client process */
      close(listening_socket);

      startprocessing(accepted_socket);
      exit(0);
    }
    else
    {
      close(accepted_socket);
    }
  } /* end of while */
}

/*
int
onmessage(libwebsock_client_state *state, libwebsock_message *msg)
{
  switch (msg->opcode) {
    case WS_OPCODE_TEXT:
      libwebsock_make_text_data_frame_with_length(state, msg->payload, msg->payload_len);
      break;
    case WS_OPCODE_BINARY:
      libwebsock_make_binary_data_frame(state, msg->payload, msg->payload_len);
      break;
    default:
      fprintf(stderr, "Unknown opcode: %d\n", msg->opcode);
      break;
  }
  return 0;
}

int
onopen(libwebsock_client_state *state)
{
  fprintf(stderr, "onopen: %d\n", state->sockfd);
  return 0;
}

int
onclose(libwebsock_client_state *state)
{
  fprintf(stderr, "onclose: %d\n", state->sockfd);
  return 0;
}
*/

/*
int
main(int argc, char *argv[])
{
  libwebsock_context *ctx = NULL;
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <port to listen on>\n\nNote: You must be root to bind to port below 1024\n", argv[0]);
    exit(0);
  }

  ctx = libwebsock_init();
  if (ctx == NULL ) {
    fprintf(stderr, "Error during libwebsock_init.\n");
    exit(1);
  }
  libwebsock_bind(ctx, "0.0.0.0", argv[1]);
  fprintf(stderr, "libwebsock listening on port %s\n", argv[1]);
  ctx->onmessage = onmessage;
  ctx->onopen = onopen;
  ctx->onclose = onclose;
  libwebsock_wait(ctx);
  //perform any cleanup here.
  fprintf(stderr, "Exiting.\n");
  return 0;
}
*/
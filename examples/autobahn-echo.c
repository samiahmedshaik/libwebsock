#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/select.h>
#include <fcntl.h>
#include <websock/websock.h>

#define PORT 9001
#define MAXMSG 4 * 1024

int write_to_client(int socket, libwebsock_client_state *state)
{
  int nbytes,retval = 0;

  if (state->out_data)
  {
    nbytes = write(socket, state->out_data->data, state->out_data->data_sz);
    if (nbytes <= 0)
    {
      /* Write error. */
      perror("write failed");
      retval = -1;
    }

    libwebsock_cleanup_outdata(state);
  }

  return retval;
}

int read_from_client(int socket, libwebsock_client_state *state)
{
  char buffer[MAXMSG] = {'\0'};
  int nbytes, retval = -1;

  nbytes = read(socket, buffer, MAXMSG);
  if (nbytes <= 0)
  {
    perror("read failed");
    return -1;
  }

  /* Data read. */
  if (state->flags & STATE_CONNECTING)
  {
    if (libwebsock_populate_handshake(state, buffer, nbytes) != -1)
    {
      retval = write_to_client(socket, state);
    }
  }
  else if (libwebsock_handle_recv(state, buffer, nbytes) == -1)
  {
    if (state->flags & STATE_NEEDS_MORE_DATA == 0)
    {
      retval = -1;
    }
    else
    {
      retval = 0;
    }
  }
  else
  {
    retval = write_to_client(socket, state);

    if (state->flags & STATE_SHOULD_CLOSE)
    {
      retval = -1;
    }
  }

  return retval;
}

int onmessage_callback(libwebsock_client_state *state, libwebsock_message *msg)
{
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
}

void do_processing(int socket)
{
  libwebsock_client_state *client_state = libwebsock_client_init();
  client_state->onmessage = onmessage_callback;

  while (1)
  {
    /* Data arriving on an already-connected socket. */
    if (read_from_client(socket, client_state) < 0)
    {
      close(socket);
      break;
    }
  }

  libwebsock_client_destroy(client_state);
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
      do_processing(accepted_socket);
      exit(0);
    }
    else
    {
      close(accepted_socket);
    }
  } /* end of while */
}

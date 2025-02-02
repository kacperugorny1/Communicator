/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>


#define BACKLOG 10   // how many pending connections queue will hold

void sigchld_handler(int s)
{
  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;
  printf("%d:%d\n", s,errno);
  while(waitpid(-1, NULL, WNOHANG) > 0);

  errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int configure_socket(char port[static 1]){
  int fd = -1;
  struct addrinfo hints =  {.ai_flags = AI_PASSIVE, AF_UNSPEC, SOCK_STREAM}, *servinfo, *p;
  struct sigaction sa;
  int yes=1;
  int rv;

  if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((fd = socket(p->ai_family, p->ai_socktype,
          p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes,
          sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    if (bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(fd);
      perror("server: bind");
      continue;
    }

    break;
  }

  freeaddrinfo(servinfo); // all done with this structure

  if (p == NULL)  {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }

  if (listen(fd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }


  return fd;
}



int main(int args, char** argv)
{ 
  struct sockaddr_storage their_addr; // connector's address information
  socklen_t sin_size;
  int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
  char s[INET6_ADDRSTRLEN];

  if(args != 2){
    fprintf(stderr, "Usage: ./main port\n");
    exit(1);
  }
  if(atoi(argv[1]) < 1024 || atoi(argv[1]) > 65535){
    fprintf(stderr, "Port should be in range 1024 - 65535\n");
    exit(1);
  }

  sockfd = configure_socket(argv[1]);
  printf("server: waiting for connections...\n");

  while(1) {  // main accept() loop
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      perror("accept");
      continue;
    }

    inet_ntop(their_addr.ss_family,
      get_in_addr((struct sockaddr *)&their_addr),
      s, sizeof s);
    printf("server: got connection from %s\n", s);

    if (!fork()) { // this is the child process
      close(sockfd); // child doesn't need the listener
      puts("Before send"); 
      char msg[1024];
      for(int i = 0; i < 1; ++i){
        scanf("%s",msg);
        if (send(new_fd, msg, strlen(msg), 0) == -1)
          perror("send");
        else puts("sent data");
      }
      close(new_fd);
      exit(0);
    }
    close(new_fd);  // parent doesn't need this
  }

  return 0;
}

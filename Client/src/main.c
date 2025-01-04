#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

int main(int argc, char *argv[]){
  int fd, status;
  char buf[1024];
  int len;
  struct addrinfo hints = {.ai_flags = AI_PASSIVE, .ai_socktype = SOCK_STREAM,.ai_family = AF_UNSPEC}
                , *res, *tmp;
  //Port check; 
  if(argc != 3){
    fprintf(stderr, "usage: ./main hostname port");
    exit(1);
  }
  if(atoi(argv[2]) <= 1024){
    fprintf(stderr, "port must be numerical port > 1024");
    exit(2);
  }

  
  //Getting addrinfo
  if((status = getaddrinfo(argv[1], argv[2], &hints, &res) != 0)){
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(3);
  }
  
  //find ipv4
  tmp = res;
  while(tmp->ai_family != PF_INET){
    tmp = tmp->ai_next;
    if(tmp == NULL) break;
  }

  if(tmp == NULL){
    fprintf(stderr, "Didnt find ipv4\n");
    exit(4);
  }
  
  //create socket
  fd = socket(tmp->ai_family, tmp->ai_socktype,tmp->ai_protocol);
  if(fd == -1){
    fprintf(stderr, "Couldnt create socket\n");
    exit(5);
  }

  puts("Socket created");
  while((status = connect(fd, tmp->ai_addr, tmp->ai_addrlen)) == -1) ;
  puts("Connected");
  len = recv(fd, buf, 1024, 0);
  if(len == -1) perror("recv");
  buf[len] = '\0';
  printf("Message: %s\n", buf);
}

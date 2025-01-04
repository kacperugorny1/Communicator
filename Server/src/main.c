#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main(int argc, char *argv[]){
  int fd, status, yes = 1;
  int fd2 = -1;
  socklen_t addrlen = { 0 };
  struct addrinfo hints = {.ai_flags = AI_PASSIVE, .ai_socktype = SOCK_STREAM,.ai_family = AF_UNSPEC}
                , *res, *tmp;
  struct sockaddr_storage their_addr = {0};
  //Port check; 
  if(argc != 2){
    fprintf(stderr, "usage: ./main port");
    exit(1);
  }
  if(atoi(argv[1]) <= 1024){
    fprintf(stderr, "port must be numerical port > 1024");
    exit(2);
  }
  
  //Getting addrinfo
  if((status = getaddrinfo(NULL, argv[1], &hints, &res) != 0)){
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
  
  //Create socket bind
  fd = socket(tmp->ai_family, tmp->ai_socktype,tmp->ai_protocol);
  if(fd == -1){
    fprintf(stderr, "Couldnt create socket\n");
    exit(5);
  }
  if(setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes, sizeof(yes)) == -1){
    perror("setsockopt");
    exit(7);
  }
  status = bind(fd, tmp->ai_addr, tmp->ai_addrlen);
  if(status == -1){
    fprintf(stderr,"Bind error");
    exit(6);
  }

  freeaddrinfo(res);

  if(listen(fd, 10) == -1){
    fprintf(stderr, "Listen error");
    exit(10);
  }
  puts("Listen");

  char buf[100];
  while(1){
    fd2 = accept(fd, (struct sockaddr*)&their_addr,&addrlen);
    if(fd2 == -1){
      perror("accept");
      continue;
    }
    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr) ,buf,100);
    printf("ADDRESS:%s\n",buf);
    puts("Accepted");
    if(!fork()){
      
      printf("Child, fd:%d, fd2:%d\n", fd,fd2);
      close(fd);
      if(send(fd2, "Hello world!", 13, 0) == -1) perror("send");
      close(fd2);
      puts("child says papa");
      exit(0);
    } 
    close(fd2);
  }
  return 0;   
}

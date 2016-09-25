#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include<time.h>
#define LEN     64
struct player{
 int id;
 int listenport;
 int speakport;
 char *hostname;
 int left_id;
 int left_port;
 char *left_hostname;
 int right_id;
 int right_port;
 char *right_hostname;
 int player_socket;
};

main (int argc, char *argv[])
{
  char buf[512];
  char host[64];
  int s, p, fp, rc, len, port;
  struct hostent *hp, *ihp;
  struct sockaddr_in sin, incoming;
  char str[LEN];
  int num_players,hops;
  struct player* players[1000];
  char *potato;
  fd_set readfds; 
  int n,returnval,potato_player;
  struct timeval tv;
  char *potato_trace;
  potato_trace=malloc(5000);

  /* read port number from command line */
  if ( argc < 4 ) {
    fprintf(stderr, "Usage: %s <port-number> <no of players> <hops>\n", argv[0]);
    exit(1);
  }

  port = atoi(argv[1]);
  num_players=atoi(argv[2]);
  hops=atoi(argv[3]);

  if(hops < 0){
   printf("ERROR: number of hops should be more than 0\n");
   exit(EXIT_SUCCESS);
  }

  if(num_players < 2){
   printf("ERROR: number of players should be more than 1\n");
   exit(EXIT_SUCCESS);
  }
 
  if(port > 65535){
   printf("ERROR: port number must be within 1024 and 65535\n");
   exit(EXIT_SUCCESS);
  }

  /* fill in hostent struct for self */
  gethostname(host, sizeof host);
  hp = gethostbyname(host);
  if ( hp == NULL ) {
    fprintf(stderr, "%s: host not found (%s)\n", argv[0], host);
    exit(1);
  }

  /* use address family INET and STREAMing sockets (TCP) */
  s = socket(AF_INET, SOCK_STREAM, 0);
  if ( s < 0 ) {
    perror("socket:");
    exit(s);
  }

 int enable=1; 
 if(setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&enable,sizeof(int))<0){
    error("setsockopt(SO_REUSEADDR) failed");
  }

  /* set up the address and port */
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  memcpy(&sin.sin_addr, hp->h_addr_list[0], hp->h_length);
  
  /* bind socket s to address sin */
  rc = bind(s, (struct sockaddr *)&sin, sizeof(sin));
  if ( rc < 0 ) {
    perror("bind:");
    exit(rc);
  }

  rc = listen(s, 5);
  if ( rc < 0 ) {
    perror("listen:");
    exit(rc);
  }

  printf("Potato Master on %s \n",hp->h_name);
  printf("Players = %d \n",num_players);
  printf("Hops = %d \n",hops);

  /* accept connections */
  int pl;
  for(pl=0;pl<num_players;pl++) {
    len = sizeof(sin);
    p = accept(s, (struct sockaddr *)&incoming, &len);
    if ( p < 0 ) {
      perror("bind:");
      exit(rc);
    }
    ihp = gethostbyaddr((char *)&incoming.sin_addr, 
			sizeof(struct in_addr), AF_INET);
//    printf("player %d is on %s \n",pl,ihp->h_name);

    struct player *addme;
    addme = (struct player*)malloc(sizeof(struct player));
    addme->id=pl;
    addme->hostname=ihp->h_name;
    addme->player_socket=p;
   
    players[pl]=addme;
    int player_port;
    len = recv(p,&player_port, sizeof(int), 0);
      if ( len < 0 ) {
        perror("recv");
        exit(1);
      }
    players[pl]->listenport=player_port;
    printf("Player %d is on hostname %s \n",pl,addme->hostname);


    if(pl > 0 && pl != num_players-1){ //second player onwards
     addme->left_id=players[pl-1]->id;
     addme->left_port=players[pl-1]->listenport;
     addme->left_hostname=players[pl-1]->hostname;
     players[pl-1]->right_id=addme->id;
     players[pl-1]->right_port=addme->listenport;
     players[pl-1]->right_hostname=addme->hostname;
    }else if(pl == num_players-1){
     addme->left_id = players[pl-1]->id;
     addme->left_port = players[pl-1]->listenport;
     addme->left_hostname=players[pl-1]->hostname;
     addme->right_id=players[0]->id;
     addme->right_port = players[0]->listenport;
     addme->right_hostname = players[0]->hostname;
     players[0]->left_id=addme->id;
     players[0]->left_port = addme->listenport;
     players[0]->left_hostname = addme->hostname;
     players[pl-1]->right_id=addme->id;
     players[pl-1]->right_port=addme->listenport;
     players[pl-1]->right_hostname=addme->hostname;
    }

    players[pl]=addme;
    
      send(p,&pl,sizeof(int),0);
    
    sleep(1);
  }

  for(pl=0;pl<num_players;pl++){
   //printf("my left : %d and right : %d\n",players[pl]->left_port,players[pl]->right_port);
   
   int n = send(players[pl]->player_socket,&players[pl]->left_id,sizeof(int),0);
   if(n<=0){
    perror("Neighbor info Send Error for Left ID\n");
   }
   n = send(players[pl]->player_socket,&players[pl]->left_port,sizeof(int),0); 
   if(n<=0){
    perror("Neighbor info Send Error for Left Port\n");
   }
   n = send(players[pl]->player_socket,players[pl]->left_hostname,1000,0);   
   if(n<=0){
    perror("Neighbor info Send Error for Left Hostname\n");
   }
   n = send(players[pl]->player_socket,&players[pl]->right_id,sizeof(int),0);
   if(n<=0){
    perror("Neighbor info Send Error for Left ID\n");
   }
   n = send(players[pl]->player_socket,&players[pl]->right_port,sizeof(int),0);     
   if(n<=0){
    perror("Neighbor info Send Error for Right Port\n");
   }
   n = send(players[pl]->player_socket,players[pl]->right_hostname,1000,0);     
   if(n<=0){
    perror("Neighbor info Send Error for Right Hostname\n");
   }
  }

  int starting_player;
  time_t t;
  srand((unsigned) time(&t));
  starting_player=rand()%num_players;
//  printf("All players present, sending potato %d to player %d\n",hops,starting_player);
 
 if(hops == 0){
  for(pl=0;pl<num_players;pl++){
   send(players[pl]->player_socket,&hops,sizeof(int),0);
  }
 }
 else{
  send(players[starting_player]->player_socket,&hops,sizeof(int),0);
 }
 
  if(hops != 0){
  printf("All players present, sending potato to player %d\n",starting_player);
  potato=malloc(1000);
  sprintf(potato,"%s%d",potato,starting_player);
 // printf("initial potato : %s\n",potato);
  n=players[0]->player_socket;

  for(pl=0;pl<num_players;pl++){
    if(players[pl]->player_socket > n)
      n=players[pl]->player_socket;
  }
  n=n+1;

  tv.tv_sec=10;
  tv.tv_usec=500000;

  char *formme;
  formme=malloc(100);
 
 
 int br=1;
 while(br){
   FD_ZERO(&readfds);

   for(pl=0;pl<num_players;pl++){
    FD_SET(players[pl]->player_socket,&readfds);
    }
    int returnval ;
    returnval= select(n,&readfds,NULL,NULL,&tv);
    if(returnval < 0)
     perror("select:");
    else if(returnval == 0)
     printf("Timeout on select\n");
    else{
     for(pl=0;pl<num_players;pl++){
   //   printf ("checking for player %d\n",pl);
      if(FD_ISSET(players[pl]->player_socket,&readfds) && recv(players[pl]->player_socket,potato_trace,5000,0)>0){
       br=0;   
       break;
       }
     }
    }
 }
 
 
 printf("Trace of potato : \n %s\n",potato_trace);

 for(pl=0;pl<num_players;pl++){
  send(players[pl]->player_socket,"close",5,0);
 }

 }
 //printf("trace is : %s\n",potato_trace);
 
  for(pl=0;pl<num_players;pl++){
   close(players[pl]->player_socket);
  }
   
// printf("Closing connection \n");
  close(s); 
  exit(0);
}

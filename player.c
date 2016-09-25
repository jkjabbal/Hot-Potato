#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#define LEN	64
struct neighbor_info{
 int id;
 int listenport;
 int speakport;
 char *hostname;
 int left_id;
 int left_port;
 char left_hostname[1000];
 int right_id;
 int right_port;
 char right_hostname[1000];
 int player_socket;
 int left_fd;
 int right_fd;
};

void prepend(char* s,const char* t){
 char ss[5000];
 char tt[5000];
 strcpy(ss,s);
 strcpy(tt,t);
 size_t len=strlen(tt);
 size_t i;

 memmove(ss+len,ss,strlen(ss)+1);

 for(i=0;i<len;++i){
  ss[i]=tt[i];
 }
 //printf("prepend output : %s\n",ss);
}


main (int argc, char *argv[])
{
  int s, rc, len, port, p, player_sock, n,r;
  char host[LEN], str[LEN];
  struct hostent *hp;
  struct sockaddr_in sin,incoming,playerin,rightsin;
  int player_port;
  struct neighbor_info info;
  fd_set readfds;
  int returnval; 
  struct timeval tv,tvl;
  char *buf;
  int left_sock,right_sock; 
  int rcc=-1;
  int raa=-1;
  int rmm=0;
  int hops;
  int dummyhops; 
  int potato_received=0;
  int potato;
  char buff[5000];
  char formme[10000];
  char trace[5000];
 
  time_t t1;
  srand((unsigned) time(&t1));

  /* read host and port number from command line */
  if ( argc != 3 ) {
    fprintf(stderr, "Usage: %s <host-name> <port-number>\n", argv[0]);
    exit(1);
  }
  
  /* fill in hostent struct */
  hp = gethostbyname(argv[1]); 
  if ( hp == NULL ) {
    fprintf(stderr, "%s: host not found (%s)\n", argv[0], host);
    exit(1);
  }
  port = atoi(argv[2]);

  /* create and connect to a socket */

  /* use address family INET and STREAMing sockets (TCP) */
  s = socket(AF_INET, SOCK_STREAM, 0);
  if ( s < 0 ) {
    perror("socket:");
    exit(s);
  }

  /* set up the address and port */
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  memcpy(&sin.sin_addr, hp->h_addr_list[0], hp->h_length);
  
  /* connect to socket at above addr and port */
  rc = connect(s, (struct sockaddr *)&sin, sizeof(sin));
  if ( rc < 0 ) {
    perror("connect:");
    exit(rc);
  }

  player_sock = socket(AF_INET, SOCK_STREAM, 0);
  if ( player_sock < 0 ) {
    perror("socket:");
    exit(player_sock);
  }

  while(1){
  int min_port=1024;
  int max_port=49151;
  player_port = rand() % (max_port+1-min_port)+min_port;

  playerin.sin_family = AF_INET;
  playerin.sin_port = htons(player_port);
  playerin.sin_addr.s_addr=INADDR_ANY;

  rc = bind(player_sock, (struct sockaddr *)&playerin, sizeof(playerin));
  if ( rc < 0 ) {
//    perror("bind:");
    continue; 
  }
  else
    break;
  }

  rc = listen(player_sock, 5);
  if ( rc < 0 ) {
    perror("listen:");
    exit(rc);
  }

  len=send(s,&player_port,sizeof(int),0);
  if(len != sizeof(player_port) ){
   perror("send error in player\n");
   exit(1);
  }
  int player_number;
  recv(s,&player_number,sizeof(player_number),0);
  printf("Connected as player %d\n",player_number);

  recv(s,&info.left_id,sizeof(int),0);
 // printf("Player Left ID : %d\n",info.left_id);

  recv(s,&info.left_port,sizeof(int),0);
 // printf("Player Left Port : %d\n",info.left_port);

  recv(s,info.left_hostname,1000,0);
  // printf("Player Left Hostname : %s\n",info.left_hostname);
 
  recv(s,&info.right_id,sizeof(int),0);
 // printf("Player Right ID : %d\n",info.right_id);
 
  recv(s,&info.right_port,sizeof(int),0);
 // printf("Player Right Port : %d\n",info.right_port);

  int l=recv(s,info.right_hostname,1000,0);
  info.right_hostname[l]="\0";
 //  printf("Player Right Hostname : %s\n",info.right_hostname);
 
  
  info.left_fd=0;
  info.right_fd=0;

   left_sock = socket(AF_INET, SOCK_STREAM, 0);
   if ( left_sock < 0 ) {
      perror("socket:");
      exit(left_sock);
   }

   // fill in hostent struct 
   hp = gethostbyname(info.left_hostname);
   if ( hp == NULL ) {
      fprintf(stderr, "%s: host not found (%s)\n", argv[0], host);
      exit(1);
   }
  
   // set up the address and port 
   sin.sin_family = AF_INET;
   sin.sin_port = htons(info.left_port);
   memcpy(&sin.sin_addr, hp->h_addr_list[0], hp->h_length);

  n=player_sock;
  if(player_sock<s)
    n=s;
  n=n+1;

  tv.tv_sec=10;
  tv.tv_usec=500000; 

  while(1){
   FD_ZERO(&readfds);
   FD_SET(s,&readfds);
   FD_SET(player_sock,&readfds);

  if(info.left_fd != 0 && info.right_fd != 0)
   break;
  else{
   returnval=select(n,&readfds,NULL,NULL,&tv);
     
    if(returnval == -1)
      perror("select:");
    else if(returnval == 0)
      printf("timeout on select. Nothing sent to you for 10.5secs");  
    else{
      if(FD_ISSET(s,&readfds) && rmm==0){
       rmm=1;
       l= recv(s,&hops,sizeof(int),0);
       if(l>0){
          if(hops == 0)
           exit(0);
          potato_received=1;
       //   strcpy(trace,"");
        //  sprintf(trace,"%d;",hops);
        //  sprintf(formme,"%d",player_number);
        //  strcat(trace,formme);
   //       printf("received hops %d\n",hops);
          if(info.left_fd==0){
          rcc = connect(left_sock, (struct sockaddr *)&sin, sizeof(sin));
          if ( rcc < 0 ) {
           perror("connect:");
           exit(rcc);
          }
          info.left_fd=left_sock;
     //    printf("%d connected to left %d\n",player_number,info.left_id);
        }          
        }
      }
      else{
        if(info.right_fd==0){
        len = sizeof(rightsin); 
        raa=accept(player_sock,(struct sockaddr *)&rightsin,&len);    
        if(raa<0){
         perror("accept:");
         exit(rc);
        }
        info.right_fd=raa;
      //   printf("%d accepted from right %d\n",player_number,info.right_id);
        }
        if(info.left_fd==0){
         rcc = connect(left_sock, (struct sockaddr *)&sin, sizeof(sin));
          if ( rcc < 0 ) {
           perror("connect:");
           exit(rcc);
          }
        info.left_fd=left_sock;
       //  printf("%d connected to left %d\n",player_number,info.left_id);
       }
      } 
     }
    }
    }

// printf("network done\n"); 
// send(s,"network done",12,0);


 fd_set readfds1;

 n=info.left_fd;
 if(info.right_fd>n)
  n=info.right_fd;
 if(s>n)
  n=s;
 n=n+1;

 tvl.tv_sec=30;
 tvl.tv_usec=500000;

//  time_t t1;
//  srand((unsigned) time(&t1));
 
 while(1){
 FD_ZERO(&readfds1);
 FD_SET(info.left_fd,&readfds1);
 FD_SET(info.right_fd,&readfds1);
 FD_SET(s,&readfds1);

   if(potato_received == 1){
     potato_received=0;
     hops--;
    if(hops == 0){
      printf("I'm it\n");
      //potato=100;
      sprintf(trace,"%d",player_number);
    //  printf("sending trace %s to master\n",trace);
      l=send(s,trace,5000,0);
      if(l<0) {
        perror("Error in sending to socket");
        exit(l);
      }
      break;
    }else{
     // printf ("honey is doing this\n");
      strcpy(trace,"");
      sprintf(trace,"%d;",hops);
      sprintf(formme,"%d",player_number);
      strcat(trace,formme);
      int next_player;
      time_t t;
      //srand((unsigned) time(&t));
      next_player=rand()%2; 
      if(next_player == 0){
       next_player=info.right_id;
       sprintf(formme,",%d",next_player);
       strcat(trace,formme);
       printf("Sending potato to %d\n",next_player);
       l=send(info.right_fd,trace,5000,0);
       if(l<0) {
        perror("Error in sending to socket");
        exit(l);
      }
      }
      else{
       next_player=info.left_id;      
       sprintf(formme,",%d",next_player);
       strcat(trace,formme);
       printf("Sending potato to %d\n",next_player);
       l=send(info.left_fd,trace,5000,0);
      if(l<0) {
        perror("Error in sending to socket");
        exit(l);
      }
      }
   }
  }else{
    //  printf("i should come here\n");
      returnval=select(n,&readfds1,NULL,NULL,&tvl);
      if(returnval <0)
        perror("select:");
      else if(returnval == 0){
       printf("Timeout for select reached\n");
       exit(1);
       }
      else{
      // if(FD_ISSET(info.left_fd,&readfds1) || FD_ISSET(info.right_fd,&readfds1)){
       if(FD_ISSET(info.left_fd,&readfds1)&&recv(info.left_fd,trace,5000,0) > 0){
      //  printf("trace received from left %s\n",trace);
        char charhops[5000];
        char dummytrace[5000];
        strcpy(dummytrace,trace);
        strcpy(charhops,"");
        strcpy(charhops,strtok(dummytrace,";"));
        hops=atoi(charhops);
        hops--;
        if(hops == 0){
         printf("I'm it\n");
         //potato=100;
         strcpy(trace,strtok(NULL,";"));
        // printf("sending trace %s to master\n",trace);
         l=send(s,trace,5000,0);
         if (l<0) {
            perror("Error in sending");
            exit(l);
         }
         break;
        }else{
         sprintf(charhops,"%d;",hops);
         strcpy(dummytrace,trace); 
         strtok(dummytrace,";");
         strcpy(dummytrace,strtok(NULL,";"));
         prepend(charhops,dummytrace);
         strcat(charhops,dummytrace);
         int next_player;
         time_t t;
         //srand((unsigned) time(&t));
         next_player=rand()%2;
         if(next_player == 0){
           next_player=info.right_id;
           sprintf(formme,",%d",next_player);
           strcat(charhops,formme);       
           printf("Sending potato to %d\n",next_player);
           l=send(info.right_fd,charhops,5000,0);
         if (l<0) {
            perror("Error in sending");
            exit(l);
         }
         }
         else{
          next_player=info.left_id;
          sprintf(formme,",%d",next_player);
          strcat(charhops,formme);
          printf("Sending potato to %d\n",next_player);
          l=send(info.left_fd,charhops,5000,0);
         if (l<0) {
            perror("Error in sending");
            exit(l);
         }
         }
        }
       }
       else if(FD_ISSET(info.right_fd,&readfds1) && recv(info.right_fd,trace,5000,0)>0 ){
       // printf("trace received from right %s\n",trace);
        //hops=dummyhops;
        char charhops[5000];
        char dummytrace[5000];
        strcpy(dummytrace,trace);
        strcpy(charhops,"");
        strcpy(charhops,strtok(dummytrace,";"));
        hops=atoi(charhops);
        hops--;
        if(hops == 0){
         printf("I'm it\n");
        // potato=100;
        strcpy(trace,strtok(NULL,";")); 
      //  printf("sending trace %s to master\n",trace);
        l=send(s,trace,5000,0);
         if (l<0) {
            perror("Error in sending");
            exit(l);
         }
         break;
        }else{
         sprintf(charhops,"%d;",hops);
         strcpy(dummytrace,trace);
         strtok(dummytrace,";");
         strcpy(dummytrace,strtok(NULL,";"));
         prepend(charhops,dummytrace);
         strcat(charhops,dummytrace);
         int next_player;
         time_t t;
         //srand((unsigned) time(&t));
         next_player=rand()%2;
         if(next_player == 0){
          next_player=info.right_id;
          sprintf(formme,",%d",next_player);
          strcat(charhops,formme);
          printf("Sending potato to %d\n",next_player);
          l=send(info.right_fd,charhops,5000,0);
         if (l<0) {
            perror("Error in sending");
            exit(l);
         }
         }
         else{
          next_player=info.left_id;
          sprintf(formme,",%d",next_player);
          strcat(charhops,formme);
          printf("Sending potato to %d\n",next_player);
          l=send(info.left_fd,charhops,5000,0);
         if (l<0) {
            perror("Error in sending");
            exit(l);
         }
         }
        }
       }
       else if(FD_ISSET(s,&readfds1) && recv(s,buff,5,0)>0){
          if(strcmp(buff,"close")==0){
        //   printf("received close from master\n");
          break;
          }
       }
     // }
    }
   }
 }

// printf("Player Done\n");
 close(left_sock);
  close(right_sock);
  close(player_sock);
   close(s);
  exit(0);
}

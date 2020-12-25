#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include "user.h"
#define BUFF_SIZE 512
#define MAX 25
#define TRUE 1
#define NameChatRoom "L2K"
#define NameFileChat "chatroom.txt"
#define MAX_CLIENTS 100
#define BUFFER_SZ 2048

static _Atomic unsigned int cli_count = 0;
static int uid = 10;
 int process=0;
 char fileName[] = "nguoidung.txt";
 char username[MAX];
   FILE *f = NULL;
    FILE *fo = NULL;
/* Client structure */
typedef struct{
  struct sockaddr_in address;
  int sockfd;
  int uid;
  char name[32];
} client_t;

client_t *clients[MAX_CLIENTS];


// typedef struct VulgarWords{
//   char word[50];
//   int length;
//   struct VulgarWords *next;
// } VulgarWords;
// VulgarWords *rootTree;

// VulgarWords *makeNodeWord(VulgarWords *nodeInput){
//     VulgarWords *temp = (VulgarWords *)malloc(sizeof(VulgarWords *));
//     temp->word = nodeInput->word;
//     temp->length = nodeInput->length;
//     temp->next = NULL;
//     return temp;
// }
// VulgarWords *insertNodeWord(VulgarWords *node){
//  node->next = rootTree;
//  rootTree = node;
//  return rootTree;
// }
// VulgarWords* findNodeWord(char word[]) {
//   VulgarWords* node = rootTree;
//   if (node == NULL)
//     return NULL;
//   else {
//     do {
//       if (strcmp(node->word, word) == 0) {
//         return node;
//       }
//       node = node->next;
//     } while (node != NULL);
//   }
//   return NULL;
// }
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
User *root, *cur;
// void str_overwrite_stdout() {
//     printf("\r%s", "> ");
//     fflush(stdout);
// }

void str_trim_lf (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) { // trim \n
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}
void readFile(char fileName[], FILE *f) {
  f = fopen(fileName, "r");
  User readUser;

  freeList();

  if (f == NULL)
    printf("Cannot read input file!\n");
  else {
    while(!feof(f)) {
      fscanf(f, "%s\t%s\t%d\n" ,readUser.username, readUser.password, &readUser.state);
      insertNode(readUser);
    }
  }

  fclose(f);

  return;
}
void writeAllToFile(char fileName[], FILE *f) {
  User *node = root;
  remove(fileName);
  f = fopen(fileName, "w");

  if (f == NULL)
    printf("Cannot read input file!\n");
  else {
    do {
      fputs(node->username, f);
      fputs("\t", f);
      fputs(node->password, f);
      fputs("\t", f);
      fprintf(f, "%d", node->state);
      if (node->next != NULL) {
        fputs("\n", f);
      }
      node = node->next;
    } while(node != NULL);
  }

  fclose(f);

  return;
}
char* CutString (char buff[]) {
  char *res;
  res = (char *)malloc(BUFF_SIZE);

  for (int i=3; i<strlen(buff); ++i) {
    *(res + i - 3) = buff[i];
  }
  *(res + strlen(buff) - 3 + 1) = '\0';

  return res;
}
/*
00 - LoginAndExitMenu
01 - Login - username
02 - Login - password
03 - MenuChat
04 - Chat 1-1
41 - Chat 1-1 send and receive message
*/
char* EncodeMessage (char mess[], int code) {
  char *EncodedMess;
  EncodedMess = (char *)malloc(BUFF_SIZE);

  if (code == 0) {
    *(EncodedMess) = '0';
    *(EncodedMess + 1) = '0';
  } 
  else if (code == 1) {
    *(EncodedMess) = '0';
    *(EncodedMess + 1) = '1';
  }
  else if (code == 2) {
    *(EncodedMess) = '0';
    *(EncodedMess + 1) = '2';
  }
  else if (code == 3) {
    *(EncodedMess) = '0';
    *(EncodedMess + 1) = '3';
  }
  else if (code == 4) {
    *(EncodedMess) = '0';
    *(EncodedMess + 1) = '4';
  }
  else if(code == 5){
    *(EncodedMess) ='0';
    *(EncodedMess+1) ='5'; 
  }
  else if(code == 51){
    *(EncodedMess) = '5';
    *(EncodedMess+1) = '1';
  }
  else if (code == 41) {
    *(EncodedMess) = '4';
    *(EncodedMess + 1) = '1';
  }

  *(EncodedMess + 2) = ' ';
  for (int i=0; i<strlen(mess); ++i) {
    *(EncodedMess + 3 + i) = mess[i];
  }
  *(EncodedMess + 3 + strlen(mess) + 1) = '\0';

  return EncodedMess;
}

char* DecodeMessage (char mess[]) {
  char *code;
  code = (char *)malloc(BUFF_SIZE);

  *(code) = mess[0];
  *(code + 1) = mess[1];
  *(code + 2) = '\0';

  return code;
}

void SendLoginAndExitMenu (int socket) {
  // code: 00
  char mess[512];
  memset(mess, 0, sizeof(mess));

  sprintf(mess, "---Welcome---\n1. Login\n2. Exit\nYour choice: ");
  char encodeMenu[512];
  strcpy(encodeMenu, EncodeMessage(mess, 0));
  send(socket, encodeMenu, strlen(encodeMenu), 0);
}

void SendLoginMenu (int socket) {
  // Login successfully
  char mess[512];
  memset(mess, 0, sizeof(mess));

  sprintf(mess, "Login successfully\n---Menu chat---\n1. Chat 1-1\n2. Chat group\n3. Exit\nYour choice: ");
  char encodeMenu[512];
  strcpy(encodeMenu, EncodeMessage(mess, 3));
  send(socket, encodeMenu, strlen(encodeMenu), 0);
}

void HandleLoginAndExitMenu (int socket, char mess[]) {
  // code: 01
  char sendMess[MAX];

  if (mess[3] == '1') {
    // LoginMenu
    strcpy(sendMess, EncodeMessage("Your account: ", 1));
    send(socket, sendMess, strlen(sendMess), 0);
  }
  else if (mess[3] == '2') {
    // Exit
    strcpy(sendMess, EncodeMessage("Goodbye bro..", 1));
    send(socket, sendMess, strlen(sendMess), 0);
    close(socket);
    // FD_CLR(socket, &readfds);
  }
  else {
    // Failed
    strcpy(sendMess, EncodeMessage("Wrong choice, try again: ", 0));
    send(socket, sendMess, strlen(sendMess), 0);
  }
}

char* HandleLoginUsername (int socket, char mess[], char username[]) {
  // code: 01
  char sendMess[MAX];

  strcpy(username, CutString(mess));

  User *foundUser = findNode(username);
  if (foundUser == NULL) {
    strcpy(sendMess, EncodeMessage("Username does not exist. Try again: ", 1));
    send(socket, sendMess, strlen(sendMess), 0);

    return "";
  } 
  else {
    strcpy(sendMess, EncodeMessage("Enter password: ", 2));
    send(socket, sendMess, strlen(sendMess), 0);

    return username;
  }
}

void HandleLoginPassword (int socket, char username[], char mess[], char fileName[], FILE *f) {
  // code: 02
  char sendMess[MAX];
  char password[MAX];

  strcpy(password, CutString(mess));

  User *foundUser = findNode(username);
  if (strcmp(foundUser->password, password) != 0) {
    strcpy(sendMess, EncodeMessage("Wrong password. Try again: ", 2));
    send(socket, sendMess, strlen(sendMess), 0);
  } 
  else {
    foundUser->state = 1;
    writeAllToFile(fileName, f);
    readFile(fileName, f);
    SendLoginMenu(socket);
  }
}
void HandleMenuChat (int socket, char mess[]) {
  char sendMess[MAX];
  if (mess[3] == '1') {
    strcpy(sendMess, EncodeMessage("Enter receiver: ", 4));
    send(socket, sendMess, strlen(sendMess), 0);
  }
  else if (mess[3] == '2') {
    strcpy(sendMess,EncodeMessage("Enter Room Chat:",5));
    send(socket,sendMess,strlen(sendMess),0);
  }
  else if (mess[3] == '3') {
    strcpy(sendMess, EncodeMessage("Goodbye bro..", 3));
    send(socket, sendMess, strlen(sendMess), 0);
    close(socket);
  }
}
int  HandleLoginChat(int socket,char mess[]){
  //05
  char sendMess[MAX];
  char groupName[MAX];

  strcpy(groupName,CutString(mess));
  if(strcmp(groupName,NameChatRoom)!= 0 ){
      strcpy(sendMess,EncodeMessage("Name of Chat Room does not exist.Try again:",5));
      send(socket,sendMess,strlen(sendMess),0);
      return -1;
  }
  else{
      
      char state[2] = "1";
      send(socket,state,strlen(state),0);
      bzero(state,sizeof(state));
      return 1;
  }
 
  return -1;
}
int HandleChatRoom(client_t *cli,char mess[],int receive){


    if (receive > 0){
      if(strlen(mess) > 0){
        send_message(mess, cli->uid);

        str_trim_lf(mess, strlen(mess));
        printf("%s -> %s\n", mess, cli->name);
      }
    } else if (receive == 0 || strcmp(mess, "exit") == 0){
      sprintf(mess, "%s has left\n", cli->name);
      printf("%s", mess);
      send_message(mess, cli->uid);
      return 1;
    } else {
      printf("ERROR: -1\n");
     return 1;
    }
}
// void *handle_client(void *arg){
//   char buff_out[BUFFER_SZ];
//   char name[32];
//   int leave_flag = 0;

//   cli_count++;
//   client_t *cli = (client_t *)arg;

//   // Name
//   if(recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) <  2 || strlen(name) >= 32-1){
//     printf("Didn't enter the name.\n");
//     leave_flag = 1;
//   } else{
//     strcpy(cli->name, name);
//     sprintf(buff_out, "%s has joined\n", cli->name);
//     printf("%s", buff_out);
//     send_message(buff_out, cli->uid);
//   }

//   bzero(buff_out, BUFFER_SZ);

//   while(1){
//     if (leave_flag) {
//       break;
//     }

//     int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
//     if (receive > 0){
//       if(strlen(buff_out) > 0){
//         send_message(buff_out, cli->uid);

//         str_trim_lf(buff_out, strlen(buff_out));
//         printf("%s -> %s\n", buff_out, cli->name);
//       }
//     } else if (receive == 0 || strcmp(buff_out, "exit") == 0){
//       sprintf(buff_out, "%s has left\n", cli->name);
//       printf("%s", buff_out);
//       send_message(buff_out, cli->uid);
//       leave_flag = 1;
//     } else {
//       printf("ERROR: -1\n");
//       leave_flag = 1;
//     }

//     bzero(buff_out, BUFFER_SZ);
//   }

//   /* Delete client from queue and yield thread */
//   close(cli->sockfd);
//   queue_remove(cli->uid);
//   free(cli);
//   cli_count--;
//   pthread_detach(pthread_self());

//   return NULL;
// }

void print_client_addr(struct sockaddr_in addr){
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

/* Add clients to queue */
void queue_add(client_t *cl){
  pthread_mutex_lock(&clients_mutex);

  for(int i=0; i < MAX_CLIENTS; ++i){
    if(!clients[i]){
      clients[i] = cl;
      break;
    }
  }

  pthread_mutex_unlock(&clients_mutex);
}

/* Remove clients to queue */
void queue_remove(int uid){
  pthread_mutex_lock(&clients_mutex);

  for(int i=0; i < MAX_CLIENTS; ++i){
    if(clients[i]){
      if(clients[i]->uid == uid){
        clients[i] = NULL;
        break;
      }
    }
  }

  pthread_mutex_unlock(&clients_mutex);
}

/* Send message to all clients except sender */
void send_message(char *s, int uid){
  pthread_mutex_lock(&clients_mutex);

  for(int i=0; i<MAX_CLIENTS; ++i){
    if(clients[i]){
      if(clients[i]->uid != uid){
        if(write(clients[i]->sockfd, s, strlen(s)) < 0){
          perror("ERROR: write to descriptor failed");
          break;
        }
      }
    }
  }

  pthread_mutex_unlock(&clients_mutex);
}

  void *handle_client_chatroom(void *arg){
    char buff[BUFFER_SZ];
    char buff_out[BUFFER_SZ];
    int leave_flag = 0;
    int left_flag =0;
    int receive;
    int process =0;
    int state =0;
    char buffer[256] = "51 Enter message to send: ";
    cli_count++;
    client_t *cli = (client_t *)arg;
    if(left_flag==0){
        while(1){
          if(left_flag){
            break;
          }
          if ((receive = recv(cli->sockfd, buff, sizeof(buff), 0)) > 0) {
                              strtok(buff, "\n");
                              printf("%s\n",buff );

                              // Handle LoginAndExitMenu
                              if (strcmp(DecodeMessage(buff), "00") == 0) {
                                HandleLoginAndExitMenu(cli->sockfd, buff);
                              }
                              // Handle username
                              else if (strcmp(DecodeMessage(buff), "01") == 0) {
                                strcpy(username, HandleLoginUsername(cli->sockfd, buff, username));
                              }
                              // Handle password
                              else if (strcmp(DecodeMessage(buff), "02") == 0) {
                                HandleLoginPassword(cli->sockfd, username, buff, fileName, f);
                                strcpy(cli->name,username);
                              }
                              // Handle menu-chat
                              else if (strcmp(DecodeMessage(buff), "03") == 0) {
                                HandleMenuChat(cli->sockfd, buff);
                              }
                              else if(strcmp(DecodeMessage(buff),"05") == 0){
                                  HandleLoginChat(cli->sockfd,buff);
                                  left_flag = 1;
                                  queue_add(cli);
                              }     
                     bzero(buff,sizeof(buff));  
          }
        }
    }
  if(left_flag ==1){
            while(1){
          if (leave_flag) {
            break;
          }
          int rec= recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
        if (rec > 0){
          if(strlen(buff_out) > 0){
            char *strcat = (char *)malloc(sizeof(char)*BUFFER_SZ);
             sprintf(strcat, "%s :%s", cli->name,buff_out);
            send_message(strcat, cli->uid);
            free(strcat);
            str_trim_lf(buff_out, strlen(buff_out));
            printf("%s -> %s\n", buff_out, cli->name);
          }
        } else if (rec== 0 || strcmp(buff_out, "exit") == 0){
          sprintf(buff_out, "%s has left\n", cli->name);
          printf("%s", buff_out);
          send_message(buff_out, cli->uid);
          leave_flag = 1;
        } else {
          printf("ERROR: -1\n");
          leave_flag = 1;
        }

        bzero(buff_out, BUFFER_SZ);
              
      }
  }
  /* Delete client from queue and yield thread */
    close(cli->sockfd);
    queue_remove(cli->uid);
    free(cli);
    cli_count--;
    pthread_detach(pthread_self());

    return NULL;
}



int main(int argc, char* argv[]) {
  if (argc == 1) {
    printf("Missing port number.\n");
    return 0;
  }
  else if (argc > 2) {
    printf("Wrong param.\n");
    return 0;
  }
  else {
     
    readFile(fileName, f);
    printNode();
  char *ip = "127.0.0.1";
  int port = atoi(argv[1]);
  int option = 1;
  int listenfd = 0, connfd = 0;

    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    pthread_t tid1,tid2;
   
    /* Socket settings */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);

  /* Ignore pipe signals */
  signal(SIGPIPE, SIG_IGN);

  if(setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0){
    perror("ERROR: setsockopt failed");
      return EXIT_FAILURE;
  }

    /* Bind */
   if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
      perror("ERROR: Socket binding failed");
      return EXIT_FAILURE;
   }

    /* Listen */
   if (listen(listenfd, 10) < 0) {
      perror("ERROR: Socket listening failed");
      return EXIT_FAILURE;
  }
  printf("=== WELCOME TO THE CHAT ===\n");
   
    while(1){
            socklen_t clilen = sizeof(cli_addr);
            connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

            /* Check if max clients is reached */
            if((cli_count + 1) == MAX_CLIENTS){
              printf("Max clients reached. Rejected: ");
              print_client_addr(cli_addr);
              printf(":%d\n", cli_addr.sin_port);
              close(connfd);
              continue;
            }

            /* Client settings */
            client_t *cli = (client_t *)malloc(sizeof(client_t));
            cli->address = cli_addr;
            cli->sockfd = connfd;
            cli->uid = uid++;

            /* Add client to the queue and fork thread */
            //queue_add(cli);
              SendLoginAndExitMenu(cli->sockfd);        
         pthread_create(&tid1, NULL, &handle_client_chatroom, (void*)cli);
            /* Reduce CPU usage */
            sleep(1);
  }
                               
}
  return EXIT_SUCCESS;
}
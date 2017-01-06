#include <sys/types.h>              /* definitions for required types */
#include <netinet/in.h>             /* for ultra60 machine */
#include <sys/socket.h>             /* socket() */
#include <stdio.h>                  /* printf(), perror(), fgets() */
#include <stdlib.h>                 /* atoi() */
#include <netdb.h>                  /* gethostbyname(), struct sockaddr_in */
#include <strings.h>                /* bzero() */
#include <arpa/inet.h>              /* inet_addr() */
#include <string.h>                 /* strcpy(), strcmp() */
#include <unistd.h>                 /* fork() */


/* Creates a socket whose sole purpose is to
 * listen for and display received messages */
int listen_for_messages() {
    int sockfd;                             /* Socket descriptor */
    struct sockaddr_in listen_addr;         /* User's address */
    struct sockaddr_in sending_addr;        /* Sender's address */
    char buf[65536];
    char *msg = "[[OK]]";
    /* Fill memory with a constant 0 byte */
    memset(&listen_addr, 0, sizeof(listen_addr));
    /* TCP address, port: 8945, all addresses */
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(8932);
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* TCP Datagram socket */
    printf("Creating socket for listening...\n");
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        perror("socket");
        return -1;
    }
    printf("Binding socket...\n");
    /* Bind socket on a user's address */
    if(bind(sockfd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
        perror("bind");
        return -1;
    }

    int sending_addr_length = sizeof(sending_addr);
    /* Listen for messages and display them */
    while(1) {
        /* Receive message */
        if(recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&sending_addr,
                    (socklen_t *)&sending_addr_length) < 0) {
            perror("recvfrom");
            return -1;
        }
        /* Only local socket can send this message
         * to the other one. */
        if(strcmp(buf, "koniec\n") == 0) {
            break;
        }
        if(sendto(sockfd, msg, strlen(msg), 0,
                  (struct sockaddr *)&sending_addr,
                  (socklen_t)sending_addr_length) < 0) {
            perror("sendto");
            return -1;
        }

        /* Print received message */
        printf("\n[%s] %s", inet_ntoa(sending_addr.sin_addr), buf);

        /* Clear the buffer */
        memset(buf, 0, sizeof(buf));
    }

    close(sockfd);
    return 1;
}

/* Takes input from the user and sends a
 * datagram with it to a given address */
int write_messages(char *hostname) {
    /* Message to be sent */
    char buf[65536];
    /* Get the hostent struct for a host  */
    struct hostent *host = gethostbyname(hostname);
    /* Save up to 10 addresses */
    struct in_addr address[10];

    printf("Creating sending socket...\n");
    /* TCP Datagram socket */
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        perror("socket");
        return -1;
    }

    /* Iterate through all addresses and save them into address array */
    int i = 0;
    while (*host->h_addr_list) {
        bcopy(*host->h_addr_list++, (char *)&address[i], sizeof(address));
        i++;
    }

    /* For sending messages to the other user
     * TCP address, port: 8945, destination address */
    struct sockaddr_in user_address;
    memset(&user_address, 0, sizeof(user_address));
    user_address.sin_family = AF_INET;
    user_address.sin_port = htons(8932);
    user_address.sin_addr.s_addr = inet_addr(inet_ntoa(address[0]));
    int user_address_length = sizeof(user_address);

    /* For sending messages to yourself
     * e.g. for closing sockets. */
    struct sockaddr_in local;
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_port = htons(8932);
    local.sin_addr.s_addr = inet_addr("127.0.0.1");
    int local_length = sizeof(local);

    int success;
    int j = 1;
    while(1) {
        printf("-->");
        /* Take input from the user and save it
         * into message.text */
        fgets(buf, sizeof(buf), stdin);
        /* Print your own message */
        printf("[%s] %s", "You", buf);

        /* If you wrote "koniec", exit the application */
        if(strcmp(buf, "koniec\n") == 0) {
            sendto(sockfd, buf, strlen(buf), 0,
                   (struct sockaddr *) &local,
                   (socklen_t)local_length);
            break;
        }
        /* Send message to the other user */
        success = sendto(sockfd, buf, strlen(buf), 0,
               (struct sockaddr *)&user_address,
               (socklen_t)user_address_length);

        /* j is the index for address so it can't be
         * bigger than the amount of addresses which is i */
        if(success < 0 && j < i) {
            /* Try next address */
            user_address.sin_addr.s_addr = inet_addr((inet_ntoa(address[j])));
            j++;
        } else if(success < 0) {
            perror("sendto");
            return -1;
        }
    }
    /* Close socket */
    close(sockfd);
    return 1;
}

int main(int argc, char **argv) {

    /* Check the number of cmd arguments */
    if(argc != 2) {
        printf("Usage:\tchat <host>\n");
        return -1;
    }

    /* Get hostname from cmd argument */
    char *hostname = argv[1];

    /* TCP Datagram socket */
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    /* Create a child process. One process for
     * sending the other one for receiving.*/
    pid_t pid = fork();
    if(pid < 0) {
        perror("fork");
        return -1;
    }

    /* Child process */
    if(pid == 0) {
        printf("Starting listen_for_messages()...\n");
        if (listen_for_messages() < 0) {
            printf("An error occured in listen_for_messages().Quiting...\n");
            return -1;
        }
    /* Parent process */
    } else {
        printf("Starting write_messages()...\n");
        if(write_messages(hostname) < 0) {
            printf("An error occured in write_messages().Quiting...\n");
            return -1;
        }
    }

    printf("\n\nGoodbye.\n\n");
    return 0;
}


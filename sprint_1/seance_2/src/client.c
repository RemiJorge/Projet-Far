#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

// DOCUMENTATION
// This program acts as a client which connects to a server
// to talk with another client
// It uses the TCP protocol
// It takes two arguments, the server ip and the server port
// If at some point one of the clients sends "fin",
// the server will close the discussion between the clients

// You can use gcc to compile this program:
// gcc -o client client.c

// Use : ./client <server_ip> <server_port>

#define max_length 50
char msg [max_length];
char input[max_length]; 


void *afficher(int color, char *msg, void *args){
    
    //Efface la ligne
    printf("\033[2K\r");
    //Remonte le curseur d'une ligne
    printf("\033[1A");
    //Efface la ligne et place le curseur en debut de ligne
    printf("\033[2K\r");
    //Change la couleur du texte
    printf("\033[%dm", color);
    //Affiche le message
    printf(msg, args);
    //Change la couleur du texte en rouge
    printf("\n\033[35m");
    printf("---------- Entrez un message (max %d caracteres) -----------\n", max_length - 1);
    //Met le texte en gras
    printf("\033[1m");
    printf("Saisie : ");
    printf("\033[0m"); //Remet le texte en normal
    fflush(stdout); //Flush le buffer de stdout
    return NULL;
}


void *readMessage(void *arg) {
    int dS = *(int *)arg;
    int nb_recv;

    while(1) {

        // Read message

        nb_recv = recv(dS, msg, max_length, 0);
        if (nb_recv == -1) {
            perror("Erreur lors de la reception du message");
            close(dS);
            exit(EXIT_FAILURE);
        } else if (nb_recv == 0) {
            // Connection closed by client or server
            break;
        }

        // Check if the message is "fin"
        // If it is, close the socket and exit
        if (strcmp(msg, "fin") == 0) {
            afficher(31, "Un client à quitter la discussion\n", NULL);
        } else {
            // Afficher le message reçu
            afficher(34, "Message recu: %s\n", msg);   

        }

    }

    pthread_exit(0);
}


void *writeMessage(void *arg) {
    int dS = *(int *)arg;
    int nb_send;

    afficher(31, "", NULL);

    while(1) {

        fgets(input, max_length, stdin); 
        char *pos = strchr(input, '\n');
        *pos = '\0';

        //Remonte le curseur d'une ligne
        printf("\033[1A"); 
        
        afficher(32, "Message envoye: %s\n", input);
        
        // Send message
        nb_send = send(dS, input, max_length, 0);
        if (nb_send == -1) {
            perror("Erreur lors de l'envoi du message");
            close(dS);
            exit(EXIT_FAILURE);
        } else if (nb_send == 0) {
            // Connection closed by remote host
            afficher(31, "Le serveur a ferme la connexion\n", NULL);
            close(dS);
            break;
        }

        // Check if the message is "fin"
        // If it is, close the socket and exit
        if (strcmp(input, "fin") == 0) {
            afficher(31, "Vous mettez fin a la discussion\n", NULL);
            //Ferme la socket
            close(dS);
            break;
        }

    }

    pthread_exit(0);
}


void handle_sigint(int sig) {
    afficher(31, "Pour quitter, veuillez saisir le mot 'fin'.\n", NULL);
}



int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("Error: You must provide exactly 2 arguments.\n\
                Usage: %s <server_ip> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    printf("\033[2J"); //Clear the screen
    printf("Debut programme client\n");
    printf("Bienvenue sur la messagerie instantanee !\n");

    int dS = socket(PF_INET, SOCK_STREAM, 0);
    if (dS == -1) {
        perror("Erreur creation socket");
        exit(EXIT_FAILURE);
    }

    printf("Socket Créé\n");

    // Voici la doc des structs utilisées
    /*
    struct sockaddr_in {
        sa_family_t    sin_family;  famille d'adresses : AF_INET     
        uint16_t       sin_port;    port dans l'ordre d'octets réseau
        struct in_addr sin_addr;    adresse Internet                 
    };
    */

    struct sockaddr_in aS;

    aS.sin_family = AF_INET;
    int result = inet_pton(AF_INET,argv[1],&(aS.sin_addr));
    if (result == 0) {
        fprintf(stderr, "Invalid address\n");
        exit(EXIT_FAILURE);
    } else if (result == -1) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    /*
        Adresse Internet
        struct in_addr {
            uint32_t    s_addr;   Adresse dans l'ordre d'octets réseau 
        };
    */
    aS.sin_port = htons(atoi(argv[2])) ;
    socklen_t lgA = sizeof(struct sockaddr_in) ;
    if (connect(dS, (struct sockaddr *) &aS, lgA) == -1) {
        perror("Erreur connect client");
        exit(EXIT_FAILURE);
    }

    printf("Socket Connecté\n\n");

    // Gestion du signal SIGINT (Ctrl+C)
    signal(SIGINT, handle_sigint);

    // Initialisation des threads
    pthread_t readThread;
    pthread_t writeThread;


    // Lancement du thread de lecture
    if (pthread_create(&readThread, NULL, readMessage, &dS) != 0) {
        perror("Erreur lors de la creation du thread de lecture");
        close(dS);
        exit(EXIT_FAILURE);
    }
    // Lancement du thread d'écriture
    if (pthread_create(&writeThread, NULL, writeMessage, &dS) != 0) {
        perror("Erreur lors de la creation du thread d'ecriture");
        close(dS);
        exit(EXIT_FAILURE);
    }




    // Attente de la fin des threads
    if (pthread_join(readThread, NULL) != 0) {
        perror("Erreur lors de la fermeture du thread de lecture");
        close(dS);
        exit(EXIT_FAILURE);
    }
    if (pthread_join(writeThread, NULL) != 0) {
        perror("Erreur lors de la fermeture du thread d'ecriture");
        close(dS);
        exit(EXIT_FAILURE);
    }

    //Efface les 2 dernières lignes
    printf("\033[2K\033[1A\033[2K\r");

    return EXIT_SUCCESS;
}
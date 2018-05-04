#include <api/mictcp_core.h>
#include <mictcp.h>
#include <pthread.h>
// Le nombre de socket maximum
#define MAX_SOCKETS 1024
//#define DEBUG
//#define DEBUG_MESSAGE
// La taille du buffer d'envoi des pdus
#define SEND_PDU_BUFFER_SIZE 1024

// Le timeout en ms
#define TIMEOUT 2
#define TIMEOUT_INITIALISATION_CONNECTION 2000

/* Tableau de socket */
mic_tcp_sock available_sockets[MAX_SOCKETS];

void* listening(void* arg);

/* Tableaux de numéros de séquence */
int pe[MAX_SOCKETS] = {0};
int pa[MAX_SOCKETS] = {0};

/* Compte le nombre de ack perdu */
int ack_perdu = 0;

/* Nombre de pdu envoyés */
int nb_pdu_envoye = 0;

/* Seuil de perte [0-1] */
float threshold = -1.0;
float threshold_min = 0.02;

/* Compteur pour les descripteurs de fichiers */
int file_descriptor_counter = 0;

/* Mets à jour l'état du dernier socket créé */
void set_last_sock_state(protocol_state state) {
	available_sockets[file_descriptor_counter].state = state;
}

/* Renvoi l'état du dernier socket créé */
protocol_state get_last_sock_state() {
	return available_sockets[file_descriptor_counter].state;
}

/* Affiche un pdu pour le débug */
void afficher_pdu(mic_tcp_pdu pdu) {
	printf("Source_port : %d | Seq_num : %d | Ack_num : %d | Syn : %d | Ack : %d | Fin : %d | Data (%d) : ",
	       pdu.header.source_port,
	       pdu.header.seq_num,
	       pdu.header.ack_num,
	       pdu.header.syn,
	       pdu.header.ack,
	       pdu.header.fin,
	       pdu.payload.size);
#ifdef DEBUG_MESSAGE
	for(int i = 0; i < pdu.payload.size; i++) {
		printf("%d ", pdu.payload.data[i]);
	}
#endif
	printf("\n");
}

/* Permet de créer un pdu spécial, c'est à dire un pdu qui contiens des flags particuliers (syn, ack, fin) */
mic_tcp_pdu make_special_pdu(int syn, int ack, int fin, mic_tcp_sock_addr addr) {

	mic_tcp_pdu pdu;

	/* Gestion du header */
	pdu.header.source_port = addr.port;
	pdu.header.dest_port =
	    9999; // Cette valeur va être modifiée derrière par la couche en dessous, du coup on peut mettre n'importe quoi.
	pdu.header.seq_num = 0;
	pdu.header.ack_num = 0;
	pdu.header.syn = syn;
	pdu.header.ack = ack;
	pdu.header.fin = fin;

	pdu.payload.size = 0;
	pdu.payload.data = NULL;
	return pdu;
}

/* Assigne les champs data d'un pdu avec les données fournies en entrée */
void set_pdu_data(mic_tcp_pdu* pdu, char* data, int size) {
	pdu->payload.size = size;
	pdu->payload.data = data;
}

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm) {
	int free_socket_found = 0;
	printf("[MIC-TCP] Appel de la fonction: ");
	printf(__FUNCTION__);
	printf("\n");
	int result = initialize_components(sm); /* Appel obligatoire */
	set_loss_rate(0);

	/* Parcours du tableau à la recherche d'un socket fermé */
	for(int i = 0; i < file_descriptor_counter; i++) {
		if(available_sockets[i].state == CLOSED) {
			available_sockets[i].state = IDLE;
			pe[i] = 0;
			pa[i] = 0;
			result = i;
			free_socket_found = 1;
			break;
		}
	}

	/* Affectation d'un socket si on a pas trouvé de socket à réutiliser */
	if((result != -1) && (free_socket_found == 0)) {
		available_sockets[file_descriptor_counter].fd = file_descriptor_counter;
		available_sockets[file_descriptor_counter].state = CONNECTED;
		result = available_sockets[file_descriptor_counter].fd;
		// pe[file_descriptor_counter] = 0;
		// pa[file_descriptor_counter] = 0;
		file_descriptor_counter++;
	}

	return result;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr) {
	printf("[MIC-TCP] Appel de la fonction: ");
	printf(__FUNCTION__);
	printf("\n");
	if(socket > file_descriptor_counter) {
		return -1;
	} else {
		/* Comme le numéro de descripteur de fichier correspond à l'index dans le tableau, on peut accèder à la
		 * structure directement */
		available_sockets[socket].addr = addr;
		return 0;
	}
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr) {
	sleep(3);
	printf("[MIC-TCP] Appel de la fonction: ");
	printf(__FUNCTION__);
	printf("\n");

	/* Récéption du SYN */
	int rec_syn = 0;
	float* pdu_data = malloc(sizeof(float));
	{
		mic_tcp_pdu pdu_syn;
		pdu_syn.payload.size = sizeof(float);
		pdu_syn.payload.data = malloc(sizeof(float));
		while(1) {
			if(IP_recv(&pdu_syn, addr, TIMEOUT_INITIALISATION_CONNECTION) == -1) {
				printf("Erreur recv \n");
			} else {
				/* Vérification du SYN */
				if(pdu_syn.header.syn == 1 && pdu_syn.header.ack == 0 && pdu_syn.header.fin == 0 &&
				   pdu_syn.payload.size == sizeof(float)) {
					rec_syn = 1;
					break;
				} else {
					printf("Invalid packet : %d\n", pdu_syn.payload.size);
				}
			}
			sleep(0.1);
		}

		float threshold_requested = *((float*)pdu_syn.payload.data);
		printf("Le seuil demandé est de : %f\n", threshold_requested);
		if(threshold_requested < threshold_min) {
			threshold = threshold_min;
			printf("Le seuil demandé est trop faible, envoi du seuil %f\n", threshold_min);
			pdu_data = &threshold_min;
		} else {
			printf("Ok, le seuil est bon \n");
			threshold = threshold_requested;
			*pdu_data = *((float*)pdu_syn.payload.data);
		}
	}

	if(rec_syn) {
		/* Envoi du SYN + ACK */
		mic_tcp_pdu pdu_syn_ack = make_special_pdu(1, 1, 0, *addr);
		set_pdu_data(&pdu_syn_ack, (char*)pdu_data, sizeof(int));
#ifdef DEBUG
		printf("Envoi du PDU de SYN ACK : ");
		afficher_pdu(pdu_syn_ack);
#endif
		if(IP_send(pdu_syn_ack, *addr) == -1) {
			printf("Erreur d'envoi du syn ack \n");
			return -1;
		}
	}

	/* Réception du ACK */
	int rec_ack = 0;
	{
		mic_tcp_pdu pdu_ack;
		pdu_ack.payload.size = 0;
		pdu_ack.payload.data = malloc(pdu_ack.payload.size * sizeof(int));
		unsigned long timestamp_pdu_sent = get_now_time_msec();
		while(get_now_time_msec() - timestamp_pdu_sent < TIMEOUT_INITIALISATION_CONNECTION) {
			if(IP_recv(&pdu_ack, addr, TIMEOUT_INITIALISATION_CONNECTION - (get_now_time_msec() - timestamp_pdu_sent)) == -1) {
				return -1;
			}
			/* Vérification du ACK */
			if(pdu_ack.header.syn == 0 && pdu_ack.header.ack == 1 && pdu_ack.header.fin == 0) {
				rec_ack = 1;
				break;
			}
		}
	}
	if(rec_ack) {
		set_last_sock_state(CONNECTED);
		int* sock = malloc(sizeof(int));
		pthread_t tid;
		*sock = socket;
		pthread_create(&tid, NULL, listening, sock);
		return 0;
	} else {
		return -1;
	}
}


/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr) {
	printf("[MIC-TCP] Appel de la fonction: ");
	printf(__FUNCTION__);
	printf("\n");

	/* Envoi du SYN */
	mic_tcp_pdu pdu_syn = make_special_pdu(1, 0, 0, addr);
	float* pdu_data = malloc(sizeof(float));
	*pdu_data = 0.01;
	set_pdu_data(&pdu_syn, (char*)pdu_data, sizeof(float));
	printf("Le seuil demandé est de %f\n", (float)*pdu_data);
#ifdef DEBUG
	printf("Envoi du SYN\n");
	afficher_pdu(pdu_syn);
#endif
	if(IP_send(pdu_syn, addr) == -1) {
		printf("Erreur d'envoi du syn\n");
		return -1;
	}

	/* Récéption du SYN + ACK */
	mic_tcp_pdu pdu_syn_ack;
	pdu_syn_ack.payload.size = sizeof(float);
	pdu_syn_ack.payload.data = malloc(sizeof(float));
	unsigned long timestamp_pdu_sent = get_now_time_msec();
	int rec_syn_ack = 0;
	while(get_now_time_msec() - timestamp_pdu_sent < TIMEOUT_INITIALISATION_CONNECTION) {
		if(IP_recv(&pdu_syn_ack, &addr, TIMEOUT_INITIALISATION_CONNECTION - (get_now_time_msec() - timestamp_pdu_sent)) == -1) {
			return -1;
		}
#ifdef DEBUG
		printf("Pdu reçu pour le syn ack :\n");
		afficher_pdu(pdu_syn_ack);
#endif
		/* Vérification du SYN + ACK */
		if(pdu_syn_ack.header.syn == 1 && pdu_syn_ack.header.ack == 1 && pdu_syn_ack.header.fin == 0) {
			printf("SYN-ACK reçu !\n");
			rec_syn_ack = 1;
			break;
		} else {
			printf("Erreur : mauvais PDU\n");
		}
	}
	threshold = *((float*)pdu_syn_ack.payload.data);
	printf("Le seuil reçu est de %f\n", threshold);

	if(rec_syn_ack) {
		/* Envoi du ACK */
		mic_tcp_pdu pdu_ack = make_special_pdu(0, 1, 0, addr);

#ifdef DEBUG
		printf("Envoi du ACK \n");
		afficher_pdu(pdu_ack);
#endif
		if(IP_send(pdu_ack, addr) == -1) {
			printf("Erreur d'envoi du ack \n");
			return -1;
		}
		set_last_sock_state(CONNECTED);
		return 0;
	} else {
		return -1;
	}
}

/*
 * Attends la réception d'un acquitement, renvoie 1 si on a reçu l'acquitement pour ack_number avant timeout, sinon
 * renvoie 0.
 */
int wait_for_ack(int ack_number, mic_tcp_sock_addr* addr, int fd) {
	unsigned long timestamp_pdu_sent = get_now_time_msec();
	while(get_now_time_msec() - timestamp_pdu_sent < TIMEOUT) {
		mic_tcp_pdu pdu_ack;
		pdu_ack.payload.size = 0;
		pdu_ack.payload.data = malloc(0);
		if(IP_recv(&pdu_ack, addr, TIMEOUT - (get_now_time_msec() - timestamp_pdu_sent)) != -1) {

#ifdef DEBUG
			afficher_pdu(pdu_ack);
			printf("WAIT_FOR_ACK | pe : %d, pa : %d \n", pe[fd], pa[fd]);
#endif
			if(pdu_ack.header.ack == 1 && pdu_ack.header.ack_num == pe[fd]) {
				pe[fd] = pe[fd] + 1;
				return 1;
			}
		}
	}
	return 0;
}

/* Renvoie 0 si l'envoi a échoué et 1 si cela a réussi */
/* FONCTION BLOQUANTE QUI ATTENDS LA RECEPTION DU ACK */
int send_blocking(int mic_sock, mic_tcp_pdu pdu, mic_tcp_sock_addr addr) {
	/* Envoi du PDU */
	if(IP_send(pdu, addr) != -1) {
#ifdef DEBUG
		printf("Début d'attente du ack\n");
#endif
		while(!wait_for_ack(pa[mic_sock], &addr, mic_sock)) {
#ifdef DEBUG
			printf("Envoi de : ");
			afficher_pdu(pdu);
			printf("Attente du ACK\n");
#endif
			// Renvoi de l'ancien PDU
			if(IP_send(pdu, addr) == -1) {
#ifdef DEBUG
				printf("Echec de IPSEND \n");
#endif
				return -1;
			}
		}
#ifdef DEBUG
		printf("Ack reçu\n");
#endif
		return 1;
	} else {
		return 0;
	}
}

/*
Renvoie:
 *  0 si l'envoi a échoué par non récéption du ACK
 *  1 si cela a réussi
 * -1 si la couche IP à échoué
Cette fonction attend TIMEOUT et passe à la suite si le ACK n'a pas été reçu */
int send_non_blocking(int mic_sock, mic_tcp_pdu pdu, mic_tcp_sock_addr addr) {

	if(IP_send(pdu, addr) != -1) {
		return wait_for_ack(pa[mic_sock], &addr, mic_sock);
	} else {
		return -1;
	}
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send(int mic_sock, char* mesg, int mesg_size) {
#ifdef DEBUG
	printf("[MIC-TCP] Appel de la fonction: ");
	printf(__FUNCTION__);
	printf("\n");
#endif
	mic_tcp_sock_addr addr = available_sockets[mic_sock].addr;
	mic_tcp_pdu pdu;

	/* Gestion du header */
	pdu.header.source_port = addr.port;
	pdu.header.dest_port =
	    9999; // Cette valeur va être modifiée derrière par la couche en dessous, du coup on peut mettre n'importe quoi.
	pdu.header.seq_num = pa[mic_sock];
	pdu.header.ack_num = 0;
	pdu.header.syn = 0;
	pdu.header.ack = 0;
	pdu.header.fin = 0;

	/* Gestion de la payload */
	pdu.payload.data = mesg;
	pdu.payload.size = mesg_size;

#ifdef DEBUG
	printf("pe : %d, pa : %d \n", pe[mic_sock], pa[mic_sock]);
#endif
	/* Incrémentation du numéro de séquence */
	pa[mic_sock] = pa[mic_sock] + 1;
#ifdef DEBUG
	printf("Envoi du PDU : ");
	afficher_pdu(pdu);
	printf("%f > %f\n", (float)ack_perdu / (float)nb_pdu_envoye, threshold);
#endif

	if(nb_pdu_envoye != 0 && (float)ack_perdu / (float)nb_pdu_envoye > threshold) {
		nb_pdu_envoye++;
		if(send_blocking(mic_sock, pdu, addr)) {
			// L'envoi a réussi
			return mesg_size;
		} else {
			// L'envoi a fonctionné
			return -1;
		}
	} else {
		int result = send_non_blocking(mic_sock, pdu, addr);
		if(result) {
			// L'envoi a réussi
			nb_pdu_envoye++;
			return mesg_size;
		} else if(result == 0) {
			// L'envoi est considéré comme réussi bien qu'on ai perdu le paquet
			pa[mic_sock] = pa[mic_sock] - 1;
			ack_perdu++;
			return mesg_size;
		} else {
			// La couche IP est défaillante
			return -1;
		}
	}
}


/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv(int socket, char* mesg, int max_mesg_size) {
#ifdef DEBUG
	printf("[MIC-TCP] Appel de la fonction: ");
	printf(__FUNCTION__);
	printf("\n");
#endif

	/* Initialisation de la payload */
	mic_tcp_payload payload;
	payload.size = max_mesg_size;
	payload.data = mesg;

	int result = app_buffer_get(payload);
	if(result != -1) {
		return result;
	} else {
		return -1;
	}
}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close(int socket) {
	printf("[MIC-TCP] Appel de la fonction :  ");
	printf(__FUNCTION__);
	printf("\n");
	available_sockets[socket].state = CLOSED;
	return 0;
}

/*
 * Construis un PDU de ack
 */
mic_tcp_pdu make_ack(int ack_num, mic_tcp_sock_addr addr) {
	mic_tcp_pdu pdu;

	/* Gestion du header */
	pdu.header.source_port = addr.port;
	pdu.header.dest_port =
	    9999; // Cette valeur va être modifiée derrière par la couche en dessous, du coup on peut mettre n'importe quoi.
	pdu.header.seq_num = 42;
	pdu.header.ack_num = ack_num;
	pdu.header.syn = 0;
	pdu.header.ack = 1;
	pdu.header.fin = 0;

	pdu.payload.size = 0;
	pdu.payload.data = malloc(pdu.payload.size * sizeof(int));
	return pdu;
}

/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_sock_addr addr, int mic_sock) {
#ifdef DEBUG
	printf("[MIC-TCP] Appel de la fonction: ");
	printf(__FUNCTION__);
	printf("\n");
#endif

/* Envoi d'un PDU avec n°seq 'x', renvoi d'un ack avec n°ack 'x'.
   Gestion de l'envoi du PDU avec bon numéro de séquence*/
#ifdef DEBUG
	printf(__FUNCTION__);
	printf(": PDU reçu : ");
	afficher_pdu(pdu);
	printf("\n");
#endif

	/* Vérification de l'état du socket */
	protocol_state state = get_last_sock_state();
	if(state == CONNECTED) {


		/* Vérification du numéro de séquence */
		if(pdu.header.seq_num == pe[mic_sock]) {
#ifdef DEBUG
			printf("PDU reçu\n");
#endif
			app_buffer_put(pdu.payload);
			mic_tcp_pdu pdu_ack = make_ack(pe[mic_sock], addr);
#ifdef DEBUG
			printf("pe : %d,pa : %d \n", pe[mic_sock], pa[mic_sock]);
#endif
			pe[mic_sock] = pe[mic_sock] + 1;
#ifdef DEBUG
			printf("process_received_pdu : Envoi du ack : ");
			afficher_pdu(pdu_ack);
#endif
			if(IP_send(pdu_ack, addr) == -1) {
				printf("Erreur : impossible d'envoyer l'acquitement\n");
				exit(1);
			}
		} else {
			mic_tcp_pdu pdu_ack = make_ack(pe[mic_sock] - 1, addr);
#ifdef DEBUG
			printf("Mauvais numéro de séquence reçu, envoi du ack : ");
			afficher_pdu(pdu_ack);
#endif
			if(IP_send(pdu_ack, addr) == -1) {
				printf("Erreur : impossible d'envoyer l'acquitement\n");
				exit(1);
			}
		}
	} else if(state == IDLE) {
		app_buffer_put(pdu.payload);
	} else {
		printf("Erreur mauvais état pour le socket");
	}
}

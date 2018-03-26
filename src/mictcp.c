#include <mictcp.h>
#include <api/mictcp_core.h>

// Le nomre de socket maximum
#define MAX_SOCKETS 1024

// La taille du buffer d'envoi des pdus
#define SEND_PDU_BUFFER_SIZE 1024

// Le timeout en ms
#define TIMEOUT 2000

/* Tableau de socket */
mic_tcp_sock available_sockets[MAX_SOCKETS];

/* Tableau de pdu envoyés */
mic_tcp_pdu send_pdu[SEND_PDU_BUFFER_SIZE];
int last_pdu_sent = 0;

/* Tableaux de numéros de séquence */
//int pe[MAX_SOCKETS];
//int pa[MAX_SOCKETS];
int pe = 0;
int pa = 0;



/* Compteur pour les descripteurs de fichiers */
int file_descriptor_counter = 0;

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm)
{
	int result = -1;
	int free_socket_found = 0;
	printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
	result = initialize_components(sm); /* Appel obligatoire */
	set_loss_rate(0);

	/* Parcours du tableau à la recherche d'un socket fermé */
	for (int i = 0; i < file_descriptor_counter; i++) {
		if (available_sockets[i].state == CLOSED) {
			available_sockets[i].state = CONNECTED;
			//pe[i] = 0;
			//pa[i] = 0;
			result = i;
			free_socket_found = 1;
			break;
		}
	}

	/* Affectation d'un socket si on a pas trouvé de socket à réutiliser */
	if ((result != -1) && (free_socket_found == 0)) {
		available_sockets[file_descriptor_counter].fd = file_descriptor_counter;
		available_sockets[file_descriptor_counter].state = CONNECTED;
		result = available_sockets[file_descriptor_counter].fd;
		//pe[file_descriptor_counter] = 0;
		//pa[file_descriptor_counter] = 0;
		file_descriptor_counter++;
	}

	return result;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
	printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
	if (socket > file_descriptor_counter) {
		return -1;
	} else {
		/* Comme le numéro de descripteur de fichier correspond à l'index dans le tableau, on peut accèder à la structure directement */
		available_sockets[socket].addr=addr;	
		return 0;
	}
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
	printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
	return 1;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
	printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
	return 1;
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
	printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
	mic_tcp_sock_addr addr = available_sockets[mic_sock].addr;
	mic_tcp_pdu pdu;

	/* Gestion du header */
	pdu.header.source_port = addr.port;
	pdu.header.dest_port = 9999; // Cette valeur va être modifiée derrière par la couche en dessous, du coup on peut mettre n'importe quoi.
	pdu.header.seq_num = pa;
	pdu.header.ack_num = 0;
	pdu.header.syn= 0;
	pdu.header.ack = 0;
	pdu.header.fin = 0;

	/* Gestion de la payload */
	pdu.payload.data = mesg;
	pdu.payload.size = mesg_size;

	/* Sauvegarde du PDU dans le buffer */	
	if (last_pdu_send > 1023) {
		printf("Buffer plein\n");
		last_pdu_sent = 0;
	}

	send_pdu[last_pdu_sent] = pdu;
	last_pdu_sent++;
	pa++;
	
	/* Envoi du PDU */
	if (IP_send(pdu,addr) != -1) {
		return mesg_size;
	} else {
		return -1;
	}
}

/* 
 * Attends la récéption d'un acquitement, renvoie 1 si on a reçu l'acquitement pour ack_number avant timeout, sinon renvoie 0.
 */
int wait_for_ack(int ack_number) {
	unsigned long timestamp_pdu_sent = get_now_time_msec();
	while (get_now_time_msec() - timestamp_pdu_sent < TIMEOUT) {
		mic_tcp_pdu pdu_received = app_buffer_get
	}
}

/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
{
	printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

	/* Initialisation de la payload */
	mic_tcp_payload payload;
	payload.size = max_mesg_size;
	payload.data = mesg;

	int result = app_buffer_get(payload);
	if (result != -1)  {
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
int mic_tcp_close (int socket)
{
	printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
	available_sockets[socket].state=CLOSED;
	return 0;
}

/*
 * Construis un PDU de ack
 */
mic_tcp_pdu make_ack(int ack_num, mic_tcp_addr addr) {
	mic_tcp_pdu pdu;
	/* Gestion du header */
	pdu.header.source_port = addr.port;
	pdu.header.dest_port = 9999; // Cette valeur va être modifiée derrière par la couche en dessous, du coup on peut mettre n'importe quoi.
	pdu.header.seq_num = 42; // TODO : Vérifier que c'est bon.
	pdu.header.ack_num = ack_num;
	pdu.header.syn= 0;
	pdu.header.ack = 1;
	pdu.header.fin = 0;
	return pdu;
}

/*
 * Renvoies 1 si c'est le ack que l'on attends.
 */
int verify_ack(mic_tcp_pdu pdu, int ack_num) {
	return pdu.header.ack == 1 && pdu.header.ack_num == ack_num;
}

/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_sock_addr addr)
{
	printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
	
	/* Envoi d'un PDU avec n°seq 'x', renvoi d'un ack avec n°ack 'x'.
	   Gestion de l'envoi du PDU avec bon numéro de séquence*/	


	/* Vérification du numéro de séquence */
	if (pdu.header.seq_num == pe) {
		app_buffer_put(pdu.payload);
		mic_tcp_pdu pdu_ack = make_ack(pe);
		pe++;
		IP_send(pdu_ack,addr);
	}
	// FIXME : Vérifier qu'il n'y a vraiment rien à faire si jamais le numéro de séquence n'est pas bon.
}

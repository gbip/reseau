---
title : Rapport TP 4 du bureau d'étude de réseau.
authors : Eva SOUSSI,Paul FLORENCE
---

Pour compiler : `make all`.

## Fix des fonction `mictcp_accept` et `mictcp_connect`

Nous avons changé notamment la valeur du timeout des fonctions et elles sont maintenant complètement opérationnelles.


## Changement des numéros de séquences

On change le mode d'incrémentation de pe et pa pour avoir des compteurs classiques. Comme on incrémente pe à chaque envoi de pdu, on doit le décrémenter quand on ne reçois pas le ack.


## Fiabilité partielle

Le mécanisme est assuré par les variables suivantes : `threshold` ainsi que `ack_perdu`et `nb_pdu_envoye` qui sont mis à jour au fur et à mesure des envois.
En comparant le rapport `(float)ack_perdu/(float)nb_pdu_envoye` à `threshold` on décide si le prochain pdu peut être perdu ou non.

Pour cela, on a crée deux fonction d'envoi de pdu, une bloquante `send_blocking` et une non bloquante `send_non_blocking`. 

La fonction `send_blocking` renvoie le pdu tant qu'elle n'a reçu le pdu ACK correspondant.
La fonction `send_non_blocking` envoie le pdu, attend le timeout et s'arrête que le pdu ACK ait été reçu ou non. Selon le code de retour :
  - si le pdu a bien été reçu (réception du pdu ACK correspondant) on incrémente `nb_pdu_envoye`
  - si le pdu a été enovoyé mais qu'on ne reçoit pas le pdu ACK, on incrémente `ack_perdu`
  - si la couche IP est défaillante on sort avec un code de retour égal à -1.


## Négociation des pertes

Il faut trouver un compromis entre le client et le serveur car le serveur veut la qualité maximale alors que le client veut préserver sa bande passante.

Pour cela on dimensionne les données à envoyer dans la `payload` en tant que float dans `mic_tcp_accept` et `mic_tcp_connect`.

On envoie envoie un seuil en même temps que l'envoi du pdu SYN côté `mic_tcp_connect`. Ce float est le seuil demandé.
Dans `mic_tcp_accept`, on déclare un seuil de référence. Si le seuil demandé est plus grand on le gardera et il définira le pourcentage de pertes. Il sera retransmis à travers la payload du pdu SYN-ACK. Dans le cas contraire, le seuil demandé est trop faible et c'est le seuil de référence qui sera choisi, et envoyé dans la payload du pdu SYN-ACK.

C'est le serveur qui a le dernier mot.


## Remplacement de l'attente active par une variable condition

Partie gérée en multithreading. 

Nous avons introduit une fonction `sleep_until_initialization`. Elle est définie dans notre code dans mictcp.c et permet d'attendre une variable condition.

Dans src/api/mictcp_core.c, nous avons introduit cette fonction.
Le thread d'écoute lance la fonction `listening` qui fait appel à `IP_recv`. Par appel à `sleep_until_initialization`, il se retrouve bloqué dans l'attente d'une variable condition. Celle-ci est libérée lorsque la connection est établie. Cela nous permet d'utiliser la fonction `IP_recv` dans la phase d'établissement de connection.


Nos implémentations fonctionnent.

## Modification de `src/apps/gateway.c`
Nous avons rajouté la ligne `exit(1)` à la ligne 226 du fichier `src/apps/gateway.c` pour gérer les erreurs de connection.

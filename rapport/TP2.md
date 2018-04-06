---
title : Rapport TP 2 et 3 du bureau d'étude de réseau.
authors : Eva SOUSSI,Paul FLORENCE
---

Pour compiler : `make all`.

# Transfert de données via le stop & wait

On utilise un numéro de séquence qui vaut 0 ou 1.

## Emetteur

Tant que l'on a pas reçu le ack avec le bon numéro de séquence on incrémente pas `pe`. La fonction `mic_tcp_send()` est bloquante et retourne seulement quand on a reçu le ACK pour le message envoyé.

## Récépteur

Quand on reçois un pdu qui n'étais pas celui qu'on attendait on renvoie un acquitement avec le numéro du précédent.

# Initialisation de la connection (TP5 partie 1, oups...)

Tout d'abord, on met en attente le thread de réception jusqu'à ce que la connexion soit établie, afin d'éviter que les PDU soient traités par `process_received_pdu`. On utilise alors la primitive `IP_recv`.

Cette modification est dans `src/api/mictcp_core.c`.

Pour faciliter le déboguage du programme nous utilisons des macros du préprocesseurs C afin d'afficher conditionnellement des `printf()`.

A la fin des fonctions `mic_tcp_accept()` et `mic_tcp_connect()`, on change l'état du socket de IDLE à CONNECTED.

Autrement, lors de l'implémentation nous avons suivi les machines à états que nous avons réalisé lors des deux premiers TDs.

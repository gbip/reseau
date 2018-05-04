---
title : Rapport TP 4 du bureau d'étude de réseau.
authors : Eva SOUSSI,Paul FLORENCE
---

Pour compiler : `make all`.

Nous avons implémenté toutes les fonctions demandés dans le sujet du TP5.

# Gestion de l'asynchronisme

Lors de l'appel à `initialize_components()` on ne lance plus le thread d'écoute.

On lance le thread d'écoute lorsque un appel à `accept()` réussit. Il faut faire attention aux variables globales qui seront partagés entre les threads. De plus il faut associer à chaque socket les valeurs de pe, pa, le seuil de perte, le nombre de ack perdu et le nombre de pdu envoyés.

Il faut stocker ces donnés dans un tableau et utiliser le numéro de socket comme indice du tableau pour retrouver le socket correspondant. Nous avions mis en place cette structure dans les TPS précédant, mais nous les avons supprimées dans la version finale du code car nous ne gérions pas le multithreading.

Pour l'asynchronisme en réception, nous créons un thread qui execute en boucle des appels à `IP_recv()` et ensuite appel `mic_tcp_process_received_pdu()` avec le pdu reçu. Comme dis précédemment, ce thread est lancé lors de l'appel à `accept()`.

# Comparaison des versions

La version à fiabilité partielle permet une lecture de la vidéo beaucoup plus fluide, cependant des artefacts graphiques apparaissent à l'écran quand il y a beaucoup de pertes. La vidéo est plus longue à lire quand on est sur la version à fiabilité totale.

# Autres usages de ce protocole

On peut imaginer utiliser ce protocole quand on a un flux continu de données à émettre mais que l'intégrité n'est pas nécessaire. Par exemple :
* suivi GPS en temps réel
* diffusion de son
* relevé de capteurs en temps réel sur le réseau

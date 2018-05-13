---
title : Rapport TP 4 du bureau d'étude de réseau.
authors : Eva SOUSSI,Paul FLORENCE
---

Pour compiler : `make all`.

Nous avons implémenté toutes les fonctions demandées dans le sujet du TP5.

# Gestion de l'asynchronisme

Nous avons remplacé la variable condition implémentée précedemment par ce qui suit. 

Lors de l'appel à `initialize_components()` on ne lance plus le thread d'écoute.

On lance le thread d'écoute lorsque un appel à `accept()` réussit. Il faut faire attention aux variables globales qui seront partagées entre les threads. De plus il faut associer à chaque socket les valeurs de pe, pa, le seuil de perte, le nombre de ack perdu et le nombre de pdu envoyé.

Il faut stocker ces données dans un tableau et utiliser le numéro de socket comme indice du tableau pour retrouver le socket correspondant. 

Dans le corps de `listening` nous avons besoin du numéro de socket. On l'alloue donc dynamiquement pour le passer en argument du `pthread_create`. On en a également besoin dans `process_received_PDU` donc nous avons modifié le corps de la fonction pour lui donner un argument de plus.

Pour l'asynchronisme en réception, nous créons un thread qui execute en boucle des appels à `IP_recv()` et ensuite appelle `mic_tcp_process_received_pdu()` avec le pdu reçu. Comme dit précédemment, ce thread est lancé lors de l'appel à `accept()`.


# Comparaison des versions

La version à fiabilité partielle permet une lecture de la vidéo beaucoup plus fluide, cependant des artefacts graphiques apparaissent à l'écran quand il y a beaucoup de pertes. La vidéo est plus longue à lire quand on est sur la version à fiabilité totale.
On remarque les effets de ce nouveau protocole à l'oeil nu.


# Autres usages de ce protocole

On peut imaginer utiliser ce protocole quand on a un flux continu de données à émettre mais que l'intégrité n'est pas nécessaire. Par exemple :
* Suivi GPS en temps réel
* Diffusion de son avec un taux de perte raisonnable
* Envoi et lecture d'un texte (avec des pertes raisonnables, nous pouvons reconnaitre les mots même s'il manque quelques lettres) 
* Relevé de capteurs en temps réel sur le réseau

# Notes

Nous avons fait passer un outil d'analyse statique (cppcheck) afin de détecter d'éventuels beugs et d'améliorer la qualité de notre implémentation.

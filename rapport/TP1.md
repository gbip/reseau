----
Eva SOUSSI
Paul FLORENCE
3MIC-E
----

# Implémentation

Pour permettre l'utilisation de différents nous avons choisi de les stocker dans un tableau global comme ceci :

```c
#define MAX_SOCKET 1024
mic_tcp_sock binded_sockets[MAX_SOCKET]

```

Pour la gestion des descripteurs de fichiers, on utilise un compteur en variable global qui correspond à l'index dans le tableau et à la valeur de `mic_tcp_sock.fd`. Cela permet de retrouver facilement les données qui nous intérressent quand on nous passe un descripteur de fichier.
De plus, pour optimiser le tableau de `mic_tcp_sock` on réutilise les socket qui ont été fermés (état CLOSED).

Nous avons rajouté un état `CONNECTED` dans `protocol_state` qui est l'état assigné à un socket à sa création. Cela sera amené à changer lors des prochains TP (IDLE).

Pour l'envoi de PDU, on ne s'est pas occupé du numéro de port de destination car il est geré par la couche IP lors de l'appel à `IP_send`.






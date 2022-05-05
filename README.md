# Distributed Algorithmic Project

## Exercice 1 

Mise en place d'une table de hachage distribué construite par un simulateur qui envoie aux pairs leur finger table et leur id.
Implementation de l'algorithme de recherche d'une clé dans l'anneau.

## Exercice 2

Calcul des finger table par les pairs.
Utilisation de l'algorithme de Hirschberg & Sinclair pour élire un leader qui calculera les finger table de tous les pairs.

### Description de l'algorithme

Dans un premier temps les pairs sont organisés en anneau bi-directionnel. Ils sont rangés dans l'ordre de leur rang mpi et possèdent dès l'initialisation un identifiant chord qui servira a calculer les finger table. Le calcul des finger table se fera par un processus élu. Pour élire ce processus nous utilisons l'algorithme de Hirschberg & Sinclair qui a une complexité en nombre de messages de N*log(N) oû N est le nombre de pairs dans le système.  
A l'initialisation, le simulateur fournit donc a chaque pair:
- un booleen initiateur qui est vrai si le pair est candidat
- son identifiant chord

Le pair initialise par lui-même :
- son rang MPI
- son etat a NSP
- son voisin droit et son voisin gauche qu'il obtient grâce à son rang MPI

On utilise 4 types de message pour l'election : 
- INIT : message envoyé par le simulateur aux pairs pour leur indiquer si ils sont candidats et pour leur faire passer leur identifiant chord
- OUT : jeton envoyé par un processus candidat a l'election, il est accompagné de l'identifiant du candidat ainsi que de la distance à laquelle il doit se propager avant de revenir
- IN : jeton de retour, lorsqu'un jeton OUT a parcouru la distance à laquelle il devait se propager, les pairs le font revenir
- LEADER : message d'annonce du leader, il y attache son id et le fait passer par les voisins droits des pairs, ce message fait le tour de l'anneau 

Durant l'election le pair peut être dans 4 états.
- NSP : au depart pour tous les pairs
- BATTU : le pair a reçu un message OUT avec un ID superieur au sien ou n'etait pas candidat et a reçu un message OUT
- ELU : le pair a envoyé un message OUT qui a fait le tour de l'anneau et lui est revenu
- ENDELEC : tous les pairs qui ont reçu le message d'annonce du pair elu passent dans cet état


## Exercice 3


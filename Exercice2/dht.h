#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

/* Entiers du système */
#define     NB_SITE 5
#define     M       6

/* Definition des tags d'envoi pour chord */
#define     TAGINIT     0
#define     LOOKUP      1
#define     RESPONSABLE 2
#define     RESPONSE    3
#define     INIT_LOOKUP 4
#define     TERMINAISON 5
#define     CHORDID     6

/* Tag des messages pour l'election */
#define     OUT     1
#define     IN      2
#define     INIT    3
#define     LEADER  4

/* Etats des processus pour l'election */
#define     NSP     -1
#define     BATTU   0
#define     ELU     1
#define     ENDELEC 2

/**
 * @brief   Structure représentant un finger 
 * 
 * @param   chord_id : id obtenu avec la fonction de hachage
 * @param   mpi_rank : rank mpi
 */
struct finger {
    int chord_id;
    int mpi_rank;
};

/**
 * @brief Structure représentant un pair
 * 
 * @param   chord_id : id obtenu avec la fonction de hachage
 * @param   mpi_rank : rank mpi
 * @param   succ     : successeur
 * @param   fingers  : finger table
 */
struct pair {
    int chord_id; // id obtenu avec la fonction de hachage
    int mpi_rank; // rank mpi
    int succ; // successeur
    struct finger fingers[M]; // table des fingers
};

/**
 * @brief Structure représentant un processus participant à l'élection du leader
 * 
 * @param   mpi_rank : rank mpi
 * @param   state    : etat du processus
 * @param   initiateur  : true = candidat, false sinon
 * @param   vg : voisin de gauche
 * @param   vd : voisin de droite
 * @param   nb_in : nombre de tokens qui sont revenus
 * @param   etape : numero de l'etape, utilisé pour calculer distance = 2^etape
 * @param   chord_id : id chord du processus
 */
struct process {
    int mpi_rank; // rank mpi du processus
    int state; // etat du processus
    int initiateur; // true = candidat, false sinon
    int vg; // voisin de gauche
    int vd; // voisin de droite
    int nb_in; // nombre de tokens qui sont revenus
    int etape; // numero de l'etape, utilisé pour calculer distance = 2^etape
    int chord_id; // id chord du processus
};

/*
 * ============================================================================
 *                           FONCTIONS POUR CHORD
 * ============================================================================
 */
void random_hach(int n, int min, int max, int *pairs);
void swap(struct pair *pa,struct pair *pb);
void trie_pairs(struct pair *pairs);
void calcul_finger(struct pair *pairs);
void pair_init(int rang, int chord_id);
void leader_word(void);

/*
 * ============================================================================
 *                    FONCTIONS POUR ELECTION DU LEADER
 * ============================================================================
 */
void recevoir_out(struct process *me, int recv[2], MPI_Status status);
void recevoir_in(struct process *me, int recv[2], MPI_Status status);
void recevoir_election(struct process *me);
void annonce_election(struct process *me);
void initier_etape(struct process *me);
struct process* election(int rang);
void simulateur_election();
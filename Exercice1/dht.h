#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

/* Entiers du système */
#define     NB_SITE     5
#define     M           6

/* Definition des tags d'envoi */
#define     TAGINIT     0
#define     LOOKUP      1
#define     RESPONSABLE 2
#define     RESPONSE    3
#define     INIT_LOOKUP 4
#define     TERMINAISON 5


/**
 * Structure représentant un finger
 */
struct finger{
    int chord_id;
    int mpi_rank;
};

/**
 * Structure représentant un pair
 */
struct pair {
    int chord_id; // id obtenu avec la fonction de hachage
    int mpi_rank; // rank mpi
    int succ; // successeur
    struct finger fingers[M]; // table des fingers
};


void random_hach(int n, int min, int max,struct pair *pairs);
void swap(struct pair *pa,struct pair * pb);
void trie_pairs(struct pair *pairs);
void calcul_finger(struct pair *pairs);
int bool_contains(int a, int b, int key);
int find_next(int key, struct pair *pair);
void lookup(int key, int source, struct pair *me);
int receive(struct pair *me);
void pair_init(int rang);
void simulateur(void);






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
#define     INSERT      6
#define     ASK_RECALCUL    7
#define     RECALCUL    8
#define     FINGER      9




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
    struct finger reverse[NB_SITE-1];// table des inverses
};

void random_hach(int n, int min, int max,struct pair *pairs);
void swap(void *a, void *b, size_t len);
void trie_fingers(struct finger *fingers, int size);
void afficher_fingers(struct pair *pair);
void afficher_reverse(struct pair * pair);
void trie_tab(int *tab, int size);
void trie_pairs(struct pair *pairs,int size);
int remove_duplicates(struct finger *finger, int size);
void recalcul_finger(struct pair *me, int *new_pair);
void calcul_finger(struct pair *pairs, int size);
void calcul_reverse(struct pair *pairs, int size);
int bool_contains(int a, int b, int key);
int find_next(int key, struct pair *pair);
void lookup(int key, int source, struct pair *me);
void wait_insertion();
int receive(struct pair *me);
void pair_init(int rang);
void simulateur(void);






#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#define     NB_SITE     5
#define     M           6
#define     TAGINIT     0
#define     LOOKUP      1
#define     RESPONSABLE 2
#define     RESPONSE    3

/**
 * Structure représentant un doublet pour gérer le chord id et le mpi id
 */
struct duet {
    int a;
    int b;
};

/**
 * Structure représentant un pair
 */
struct pair {
    int chord_id;       // id obtenu avec la fonction de hachage
    int mpi_rank;       // rank mpi
    int finger[M][2];     // table des fingers
    int succ;           // successeur
};


/* =========================CODE POUR SETUP LA SIMULATION========================= */

/**
 * Fonction de hachage random des id
 * @param n     : nombre d'id a calculer
 * @param min   : borne inf des id
 * @param max   : borne sup des id
 */
int* random_hach(int n, int min, int max) 
{
    int *res = (int*) malloc(sizeof(int) * n);
    int value;
    int i;
    int j;
    srand(time(NULL));

    for (i = 0; i < n; i++) {
        rand:
        value = min + (rand() % max);
        for (j = 0; j < n; j++)
            if (res[j] == value)
                goto rand; /* id pas unique, on recalcule */
        res[i] = value;
    }
    return res;
}

/**
 * Fonction utilitaire pour echanger deux valeurs dans un tableau
 * Utilisée dans trie_ids
 */
void swap(int *a, int *b) 
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

/**
 * Fonction triant un array d'ids chord
 * @param pair_ids  : tableau non triée en entrée
 * @return un array de duet ou a = id_chord et b = mpi_rank
 */
struct duet *trie_ids(int *pair_ids)
{
    int i;
    int j;
    int sorted_ids[NB_SITE];
    struct duet *sorted_duet_ids = (struct duet*) malloc(sizeof(struct duet)*NB_SITE);

    for (i = 0; i < NB_SITE; i++) {
        sorted_ids[i] = pair_ids[i];
    }
    for (i = 0; i < NB_SITE; i++) {
        for (j = 0; j < NB_SITE-i-1; j++) {
            if (sorted_ids[j] > sorted_ids[j+1])
                swap(&sorted_ids[j], &sorted_ids[j+1]);
        }
    }

    for (i = 0; i < NB_SITE; i++) {
        for (j = 0; j < NB_SITE; j++) {
            if (pair_ids[j] == sorted_ids[i]) {
                sorted_duet_ids[i].a = sorted_ids[i];
                sorted_duet_ids[i].b = j;
            }
        }
    }
    return sorted_duet_ids;
}

/**
 * Fonction de calcul des fingers table
 */
struct duet **calcul_finger(struct duet *sorted_duet_ids) 
{
    int i;
    int j;
    int k;
    int value;
    int tmp_chord_id;
    int tmp_mpi_id;
    /* Allocation de la finger table */ 
    struct duet **finger_tables = (struct duet**) malloc(sizeof(struct duet) * NB_SITE);
    for (i = 0; i < M; i++) {
        finger_tables[i] = (struct duet*) malloc(sizeof(struct duet));
    }
    
    for (k = 0; k < NB_SITE; k++) {
        for (i = 0; i < M; i++) {
            value = (int) (pow(2, i) + sorted_duet_ids[k].a) % (int) pow(2, M);
            tmp_chord_id = __INT32_MAX__;
            tmp_mpi_id = -1;
            for (j = NB_SITE-1; j >= 0; j--) {
                if (j == NB_SITE-1 && sorted_duet_ids[j].a < value) {
                    tmp_chord_id = sorted_duet_ids[0].a;
                    tmp_mpi_id = sorted_duet_ids[0].b;
                    break;
                }
                if (sorted_duet_ids[j].a >= value) {
                    tmp_chord_id = sorted_duet_ids[j].a;
                    tmp_mpi_id = sorted_duet_ids[j].b;
                }
            }
            finger_tables[k][i].a = tmp_chord_id;
            finger_tables[k][i].b = tmp_mpi_id;
        }
    }
    
    return finger_tables;
}

/**
 * Fonction d'initialisation des pairs 
 */
void simulateur(void)
{
    int i;
    int j;

    /* Calcul des id chord des differents pairs */
    int *pair_ids = random_hach(NB_SITE, 0, pow(2, M)-1);
    struct duet *sorted_duet_ids = trie_ids(pair_ids);

    /* Calcul des finger tables de chaque processus */
    struct duet **finger_tables = calcul_finger(sorted_duet_ids);
    for (i = 0; i < NB_SITE; i++) {
        printf("\n finger table de %d: [", sorted_duet_ids[i].a);
        for (j = 0; j < M; j++) {
            printf("[%d,%d]", finger_tables[i][j].a, finger_tables[i][j].b);
        }
        printf("]\n");
    }

    int final_finger[NB_SITE][M][2];
    for (i = 0; i < NB_SITE; i++) {
        for (j = 0; j < M; j++) {
            final_finger[i][j][0] = finger_tables[i][j].a;
            final_finger[i][j][1] = finger_tables[i][j].b;
        }
    }

    /* Envoi des données aux pairs */
    for (i = 0; i < NB_SITE; i++) {
        MPI_Send(&sorted_duet_ids[i].a, 1, MPI_INT, sorted_duet_ids[i].b+1, TAGINIT, MPI_COMM_WORLD);    
        MPI_Send(&final_finger[i], 2*M, MPI_INT, sorted_duet_ids[i].b+1, TAGINIT, MPI_COMM_WORLD);       
    }
}

/* ============================CODE DES PAIRS================================= */

/**
 * Fonction qui cherche le responsable de la clé grâce à la finger table
 */
int find_next(int key, struct pair *me) 
{
    int i;
    int tmp = -1;
    for (i = M-1; i >= 0; i--) {
        if (me->finger[i][0] < key && i == M-1) {
            return me->finger[0][1];
        }
        if (me->finger[i][0] > key) {
            tmp = me->finger[i][0];
        }
    }
    return tmp;
}

/**
 * Fonction lookup, supervise la recherche du responsable de la clé
 */
void lookup(int key, int source, struct pair *me) 
{
    int next;
    int mess[2] = {key, source};
    if ((next = find_next(key, me)) == -1) {
        MPI_Send(&mess, 2, MPI_INT, me->finger[0][1], RESPONSABLE, MPI_COMM_WORLD); 
    } else {
        MPI_Send(&mess, 2, MPI_INT, next, LOOKUP, MPI_COMM_WORLD);
    }
}

/**
 * Fonction de Reception d'un message
 */
void receive(struct pair *me)
{
    int mess[2];
    MPI_Status status;
    MPI_Recv(mess, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    switch (status.MPI_TAG) {
        case RESPONSABLE:
            MPI_Send(1, 1, MPI_INT, mess[0], RESPONSE, MPI_COMM_WORLD);    
            break;
        case LOOKUP:
            lookup(mess[0], mess[1], me);
            break;
        default:
            printf("Message inconnu\n");
    }
}

/**
 * Fonction gérant la réception des données envoyées par le simulateur
 * @param me    : structure représentant un pair
 */
void receive_init(struct pair *me) 
{
    MPI_Status status;
    MPI_Recv(&(me->chord_id), 1, MPI_INT, 0, TAGINIT, MPI_COMM_WORLD, &status);
    MPI_Recv(&(me->finger), 2*M+1, MPI_INT, 0, TAGINIT, MPI_COMM_WORLD, &status);
    printf("\n finger table de %d id %d: [", me->chord_id, me -> mpi_rank);
    for (int j = 0; j < M; j++) {
        printf("[%d,%d]", me->finger[j][0], me->finger[j][1]);
    }
    printf("]\n");
}

/**
 * Fonction exécutée par les pairs
 * @param rang  : mpi_rank du processus
 */
void pair_init(int rang) 
{
    struct pair *me = (struct pair*) malloc(sizeof(struct pair));
    me->mpi_rank = rang;
    receive_init(me);
}

/**
 * Lancement de la simulation de CHORD
 */
int main(int argc, char* argv[]) 
{
    int nb_proc, rang;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);

    if (nb_proc != NB_SITE+1) {
        printf("Nombre de processus incorrect !\n");
        MPI_Finalize();
        exit(2);
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &rang);
    if (rang == 0) {
        simulateur();
    } else {
        pair_init(rang);
    }
    MPI_Finalize();
    return 0;
}
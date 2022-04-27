#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

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
 * Les id_mpi sont égaux à la case du tableau renvoyé + 1
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
 * @return  un tableau trié par rapport aux ids chords 
 *          le tableau contient pour chaque pair une paire {id_chord, id_mpi}
 */
int **trie_ids(int *pair_ids)
{
    int i;
    int j;
    int sorted_ids[NB_SITE];
    int **sorted_duet_ids = (int**) malloc(sizeof(int)*NB_SITE*2);
    for (i = 0; i < NB_SITE; i++) {
        sorted_duet_ids[i] = (int*) malloc(sizeof(int)*2);
    }

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
                sorted_duet_ids[i][0] = sorted_ids[i];
                sorted_duet_ids[i][1] = j+1; // +1 car on decales les ids a cause du simulateur
            }
        }
    }
    return sorted_duet_ids;
}

/**
 * Fonction de calcul des fingers table
 */
int ***calcul_finger(int **sorted_duet_ids) 
{
    int i;
    int j;
    int k;
    int value;
    int tmp_chord_id;
    int tmp_mpi_id;
    /* Allocation de la finger table */ 
    int ***finger_tables = (int***) malloc(sizeof(int)*NB_SITE*M*2);
    for (i = 0; i < NB_SITE; i++) {
        finger_tables[i] = (int**) malloc(sizeof(int)*M);
        for (j = 0; j < M; j++) {
            finger_tables[i][j] = (int*) malloc(sizeof(int)*2);
        }
    }
    
    for (k = 0; k < NB_SITE; k++) {
        for (i = 0; i < M; i++) {
            value = (int) (pow(2, i) + sorted_duet_ids[k][0]) % (int) pow(2, M);
            tmp_chord_id = __INT32_MAX__;
            tmp_mpi_id = -1;
            for (j = NB_SITE-1; j >= 0; j--) {
                if (j == NB_SITE-1 && sorted_duet_ids[j][0] < value) {
                    tmp_chord_id = sorted_duet_ids[0][0];
                    tmp_mpi_id = sorted_duet_ids[0][1];
                    break;
                }
                if (sorted_duet_ids[j][0] >= value) {
                    tmp_chord_id = sorted_duet_ids[j][0];
                    tmp_mpi_id = sorted_duet_ids[j][1];
                }
            }
            finger_tables[k][i][0] = tmp_chord_id;
            finger_tables[k][i][1] = tmp_mpi_id;
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
    MPI_Status status;

    /* Calcul des id chord des differents pairs */
    int *pair_ids = random_hach(NB_SITE, 0, pow(2, M)-1);
    printf("[");
    for (i = 0; i < NB_SITE; i++) {
        printf("%d,", pair_ids[i]);
    }
    printf("]\n");
    int **sorted_duet_ids = trie_ids(pair_ids);

    /* Calcul des finger tables de chaque processus */
    int ***finger_tables = calcul_finger(sorted_duet_ids);
    for (i = 0; i < NB_SITE; i++) {
        printf("\n finger table de %d: [", sorted_duet_ids[i][0]);
        for (j = 0; j < M; j++) {
            printf("[%d,%d]", finger_tables[i][j][0], finger_tables[i][j][1]);
        }
        printf("]\n");
    }
    
    int final_finger[NB_SITE][M][2];
    for (i = 0; i < NB_SITE; i++) {
        for (j = 0; j < M; j++) {
            final_finger[i][j][0] = finger_tables[i][j][0];
            final_finger[i][j][1] = finger_tables[i][j][1];
        }
    }
    
    /* Envoi des données aux pairs */
    for (i = 0; i < NB_SITE; i++) {
        MPI_Send(&sorted_duet_ids[i][0], 1, MPI_INT, sorted_duet_ids[i][1], TAGINIT, MPI_COMM_WORLD);    
        MPI_Send(&final_finger[i], 2*M, MPI_INT, sorted_duet_ids[i][1], TAGINIT, MPI_COMM_WORLD);       
    }

    srand(time(NULL));
    int mess[2];
    /* Selection d'une clé aléatoire */
    mess[0] = rand() % (int) pow(2, M);
    /* Selection d'un pair aleatoire */
    mess[1] = sorted_duet_ids[rand() % NB_SITE][1];
    /* Envoi de la recherche */
    MPI_Send(mess, 2, MPI_INT, mess[1], INIT_LOOKUP, MPI_COMM_WORLD);
    /* Recv du resultat */
    int recv[2];
    MPI_Recv(mess, 2, MPI_INT, mess[1], TERMINAISON, MPI_COMM_WORLD, &status);
    /* Send TERMINAISON a tout le monde */
    int vide[2] = {0, 0};
    for (i = 1; i < NB_SITE+1; i++) {
        MPI_Send(mess, 2, MPI_INT, i, TERMINAISON, MPI_COMM_WORLD);
    }
}   

/* ============================CODE DES PAIRS================================= */

/**
 * Fonction booléene qui vérifie que key appartient à ]a,b]
 * @return  true    : si key appartient
 *          false   : sinon
 */
int bool_contains(int a, int b, int key)
{
    if (a < b) {
        a += pow(2, M) - 1;
    }
    return b < key && key < a;
}

/**
 * Fonction qui cherche le responsable de la clé grâce à la finger table
 */
int find_next(int key, struct pair *me) 
{
    int i;
    for (i = M-1; i >= 0; i--) {
        if (bool_contains(me->chord_id, me->finger[i][0], key)) {
            return me->finger[i][1];
        }
    }
    return -1;
}

/**
 * Fonction lookup, supervise la recherche du responsable de la clé
 */
void lookup(int key, int source, struct pair *me) 
{
    int next;
    int mess[2] = {key, source};
    int vide[2] = {0, 0};
    if ((next = find_next(key, me)) == -1) {
        printf("Je suis %d et le responsable de %d est %d\n", me->chord_id, key, me->succ);
        MPI_Send(mess, 2, MPI_INT, me->succ, RESPONSABLE, MPI_COMM_WORLD);
    } else {
        printf("next %d, succ %d et je suis %d\n", next, me->succ, me->mpi_rank);
        MPI_Send(mess, 2, MPI_INT, next, LOOKUP, MPI_COMM_WORLD);
    }
}

/**
 * Fonction de Reception d'un message
 */
int receive(struct pair *me)
{
    int mess[2];
    int vide[2] = {0, 0};
    MPI_Status status;
    MPI_Recv(mess, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    switch (status.MPI_TAG) {
        case RESPONSABLE:
            MPI_Send(vide, 2, MPI_INT, mess[1], RESPONSE, MPI_COMM_WORLD);    
            break;
        case RESPONSE:
            MPI_Send(vide, 2, MPI_INT, 0, TERMINAISON, MPI_COMM_WORLD); 
            break;
        case LOOKUP:
            lookup(mess[0], mess[1], me);
            break;
        case INIT_LOOKUP:
            printf("Je cherche %d, je suis %d\n", mess[0], me->chord_id);
            lookup(mess[0], mess[1], me);
            break;
        case TERMINAISON:
            return -1;
        default:
            printf("Message inconnu\n");
    }
    return 0;
}

/**
 * Fonction gérant la réception des données envoyées par le simulateur
 * @param me    : structure représentant un pair
 */
void receive_init(struct pair *me) 
{
    MPI_Status status;
    MPI_Recv(&(me->chord_id), 1, MPI_INT, 0, TAGINIT, MPI_COMM_WORLD, &status);
    MPI_Recv(&(me->finger), 2*M, MPI_INT, 0, TAGINIT, MPI_COMM_WORLD, &status);
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
    me->succ = me->finger[0][1];

    while (receive(me) != -1) {}
    printf("%d se termine\n", me->mpi_rank);
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
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define     NB_SITE     5
#define     M           6

/**
 * Structure représentant un pair
 */
struct pair {
    int id;         // id obtenu avec la fonction de hachage
    int mpi_rank;   // rank mpi
    int *finger;    // table des fingers
    int succ;       // successeur
};



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
 * Trie d'un tableau de pairs dans l'ordre croissant
 */
void swap(int *a, int *b) 
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}
int **trie_ids(int *pair_ids)
{
    int i;
    int j;
    int tmp[NB_SITE];
    int sorted_pair_ids[NB_SITE];
    int **result = (int**) malloc(sizeof(int) * NB_SITE);
    for (i = 0; i < NB_SITE; i++) {
        result[i] = (int*) malloc(sizeof(int) * 2);
    }
    for (i = 0; i < NB_SITE; i++) {
        sorted_pair_ids[i] = pair_ids[i];
    }
    for (i = 0; i < NB_SITE; i++) {
        for (j = 0; j < NB_SITE-i-1; j++) {
            if (sorted_pair_ids[j] > sorted_pair_ids[j+1])
                swap(&sorted_pair_ids[j], &sorted_pair_ids[j+1]);
        }
    }

    for (i = 0; i < NB_SITE; i++) {
        for (j = 0; j < NB_SITE; j++) {
            if (pair_ids[j] == sorted_pair_ids[i]) {
                result[i][0] = sorted_pair_ids[i];
                result[i][1] = j;
            }
        }
    }
    return result;
}

/**
 * Fonction de calcul des fingers table
 */
int ***calcul_finger(int **sorted_pair_ids) 
{
    int i;
    int j;
    int k;
    int value;
    int tmp_chord_id;
    int tmp_mpi_id;
    /* Allocation de la finger table */ 
    int ***finger_tables = (int***) malloc(sizeof(int) * NB_SITE);
    for (i = 0; i < NB_SITE; i++) {
        finger_tables[i] = (int**) malloc(sizeof(int) * M);
        for (j = 0; j < M; j++) {
            finger_tables[i][j] = (int*) malloc(sizeof(int) * 2);
        }
    }
    printf("max : %d\n", sorted_pair_ids[NB_SITE-2][0]);
    for (k = 0; k < NB_SITE; k++) {
        for (i = 0; i < M; i++) {
            value = (int) (pow(2, i) + sorted_pair_ids[k][0]) % (int) pow(2, M);
            tmp_chord_id = __INT32_MAX__;
            tmp_mpi_id = -1;
            for (j = NB_SITE-1; j >= 0; j--) {
                printf("tmp : %d, value : %d\n", sorted_pair_ids[j][0], value);
                if (j == NB_SITE-1 && sorted_pair_ids[j][0] < value) {
                    printf("1ere cond\n");
                    tmp_chord_id = sorted_pair_ids[0][0];
                    tmp_mpi_id = sorted_pair_ids[0][1];
                    break;
                }
                if (sorted_pair_ids[j][0] > value) {
                    printf("2eme cond\n");
                    tmp_chord_id = sorted_pair_ids[j][0];
                    tmp_mpi_id = sorted_pair_ids[j][1];
                }
            }
            printf("value = %d, chord id = %d, mpi_id = %d\n", value, tmp_chord_id, tmp_mpi_id);
            finger_tables[k][i-1][0] = tmp_chord_id;
            finger_tables[k][i-1][1] = tmp_mpi_id;
        }
        printf("tour\n");
    }
    printf("fin\n");
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
    int *pair_ids = random_hach(NB_SITE, 0, pow(2, 6)-1);

    int **sorted_pairs = trie_ids(pair_ids);
    printf("[");
    for (i = 0; i < NB_SITE; i++)
        printf("[%d-%d]", sorted_pairs[i][0], sorted_pairs[i][1]);
    printf("]\n");

    /* Calcul des finger tables de chaque processus */

    int ***finger_tables = calcul_finger(sorted_pairs);
    for (i = 0; i < NB_SITE; i++) {
        printf("\n finger table de %d : [", i);
        for (j = 0; j < M; j++) {
            printf("[%d,%d]", finger_tables[i][j][0], finger_tables[i][j][1]);
        }
        printf("]\n");
    }
    /* Ajout des successeurs */

    /* Envoi des données aux pairs */
}

int main(int argc, char* argv[]) 
{
    /*
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
        // A FAIRE
    }
    */
   simulateur();
    return 0;
}
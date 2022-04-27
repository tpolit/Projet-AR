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

struct finger{
    int chord_id;
    int mpi_rank;
};

/**
 * Structure représentant un pair
 */
struct pair {
    int chord_id;       // id obtenu avec la fonction de hachage
    int mpi_rank;       // rank mpi
    int succ;           // successeur
    struct finger new_fingers[M];
};



/**
 * Fonction de hachage random des id
 * Les id_mpi sont égaux à la case du tableau renvoyé + 1
 * @param n     : nombre d'id a calculer
 * @param min   : borne inf des id
 * @param max   : borne sup des id
 */

void random_hach(int n, int min, int max,struct pair *pairs) 

{
  
    int value;
    int i;
    int j;
    srand(time(NULL));

    for (i = 0; i < n; i++) {
        rand:
        value = min + (rand() % max);
        for (j = 0; j < n; j++)
            if (pairs[j].chord_id == value)
                goto rand; /* id pas unique, on recalcule */
        pairs[i].chord_id = value;
    }
}


void swap(struct pair *pa,struct pair * pb){
    struct pair  ptmp = *pa;
    *pa=*pb;
    *pb=ptmp;

}

void trie_pairs(struct pair *pairs){
    int i,j;
    for(i=0; i< NB_SITE;i++){
        for(j=0 ; j<NB_SITE-i-1;j++){
            if(pairs[j].chord_id > pairs[j+1].chord_id){
                swap(&pairs[j],&pairs[j+1]);
                
            }
        }
    }

}


void calcul_finger(struct pair * pairs)
{

    int i;
    int j;
    int k;
    int value;
    int tmp_mpi_id;


    //k : indice pour les pairs à initialiser
    for(k=0; k<NB_SITE; k++){
        struct finger tmp_finger;

        //i :ndice pour les fingers
        for(i=0; i<M ;i++){
            value=(int )( (int)(pow(2,i)+pairs[k].chord_id) %(int)pow(2,M));
            for(j =NB_SITE -1 ; j>=0 ; j--){
                if( j == NB_SITE -1 && pairs[j].chord_id < value){
                    tmp_finger.chord_id=pairs[0].chord_id;
                    tmp_finger.mpi_rank=pairs[0].mpi_rank;
                    pairs[k].new_fingers[i]=(tmp_finger);
                    break;
                }
                if(pairs[j].chord_id>=value){
                    tmp_finger.chord_id=pairs[j].chord_id;
                    tmp_finger.mpi_rank=pairs[j].mpi_rank;
                    pairs[k].new_fingers[i]=tmp_finger;
                    
                }
            }
        }
    }
}

int bool_contains(int a, int b, int key)
{
    printf("b=%d, k=%d, a=%d \n",b,key,a);
    if (a < b) {
        a += pow(2, M) - 1;
        if(key<a)
            key+=pow(2,M)-1;
    }
    return b < key && key < a;
}

/**
 * Fonction qui cherche le responsable de la clé grâce à la finger table
 */
int find_next(int key, struct pair *pair) 
{
    int i;
    for (i = M-1; i >= 0; i--) {
        if (bool_contains(pair->chord_id, pair->new_fingers[i].chord_id, key)) {
            return pair->new_fingers[i].mpi_rank;
        }
    }
    return -1;
}


void lookup(int key, int source, struct pair *me) 
{
    int next;
    int mess[2] = {key, source};
    int vide[2] = {0, 0};
    if (((next = find_next(key, me)) == -1)) {
        if( (key > me->new_fingers[0].chord_id)){
            printf("Je suis %d et le responsable de %d est %d\n", me->chord_id, key, me->succ);
            MPI_Send(mess, 2, MPI_INT, me->succ, RESPONSABLE, MPI_COMM_WORLD);
        }else{
            MPI_Send(mess, 2, MPI_INT, me->succ, LOOKUP, MPI_COMM_WORLD);
        }
    } else {
        printf("next %d, succ %d et je suis %d\n", next, me->succ, me->mpi_rank);
        MPI_Send(mess, 2, MPI_INT, next, LOOKUP, MPI_COMM_WORLD);
    }
}


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
            MPI_Send(vide, 2, MPI_INT, NB_SITE, TERMINAISON, MPI_COMM_WORLD); 
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


void pair_init(int rang)
{
    struct pair* pair=(struct pair*)malloc(sizeof(struct pair));
    int recv;
    MPI_Status status;
    pair->mpi_rank=rang;
    MPI_Recv(&recv,1,MPI_INT,MPI_ANY_SOURCE,TAGINIT,MPI_COMM_WORLD,NULL);
    MPI_Recv(&(pair->new_fingers),M*sizeof(struct finger),MPI_CHAR,MPI_ANY_SOURCE,TAGINIT,MPI_COMM_WORLD,&status);
    pair->chord_id=recv;
    pair->succ=pair->new_fingers[0].mpi_rank;

    printf("%d Finger Table \n",pair->chord_id);
    printf("[");
    for(int i=0 ; i< M;i ++){
        printf("%d ",pair->new_fingers[i].chord_id);
    }
    printf("]\n");

    while(receive(pair)!=-1){}
    
    

}


void simulateur(void)
{

    struct pair *pairs= malloc(NB_SITE *sizeof(struct pair));  
    MPI_Status status;
    int i;
    random_hach(NB_SITE,0,pow(2,M)-1,pairs);    
    for(int i=0; i< NB_SITE ; i++){
        pairs[i].mpi_rank=i;
    }

    trie_pairs(pairs);
    printf("[");
    for(int i=0 ; i<NB_SITE; i++){
        printf(" ID %d Rank %d \n",pairs[i].chord_id,pairs[i].mpi_rank);
    }
    printf("]\n");


    //Calcul de la finger table
    calcul_finger(pairs);

    for(int i=0 ; i<NB_SITE ;i ++){
        MPI_Send(&(pairs[i].chord_id),1,MPI_INT,pairs[i].mpi_rank,TAGINIT,MPI_COMM_WORLD);
        MPI_Send(pairs[i].new_fingers,M*sizeof(struct finger),MPI_CHAR,pairs[i].mpi_rank,TAGINIT,MPI_COMM_WORLD);
    }


    srand(time(NULL));
    int mess[2];
    /* Selection d'une clé aléatoire */
    mess[0] = rand() % (int) pow(2, M);
    /* Selection d'un pair aleatoire */
    mess[1] = pairs[rand() % NB_SITE].mpi_rank;
    /* Envoi de la recherche */
    MPI_Send(mess, 2, MPI_INT, mess[1], INIT_LOOKUP, MPI_COMM_WORLD);
    /* Recv du resultat */
    int recv[2];
    MPI_Recv(mess, 2, MPI_INT, mess[1], TERMINAISON, MPI_COMM_WORLD, &status);
    /* Send TERMINAISON a tout le monde */
    int vide[2] = {0, 0};
    for (i = 0; i < NB_SITE; i++) {
        MPI_Send(mess, 2, MPI_INT, i, TERMINAISON, MPI_COMM_WORLD);
    }

}


int main(int argc, char* argv[]){

    int nb_proc, rang;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);

    if (nb_proc != NB_SITE+1) {
        printf("Nombre de processus incorrect !\n");
        MPI_Finalize();
        exit(2);
    }
    MPI_Comm_rank(MPI_COMM_WORLD,&rang);

    if(rang == NB_SITE){
        simulateur();
    }else{
        pair_init(rang);

    }
    MPI_Finalize();


}


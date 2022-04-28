/**
 * @file Exercice1.c
 * @author Firas Jebari & Titouan Polit
 * @brief Recherche d'une clé en distribué avec calcul des finger table par un simulateur
 * @version 1
 * @date 2022-04-28
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "dht.h"

/**
 * @brief   Fonction de hachage random des id
 *          Les id_mpi seront liés à la case du tableau renvoyé
 * @param n nombre d'id a calculer
 * @param min borne inf des id
 * @param max borne sup des id
 * @param pairs tableau de structure pair que l'on va remplir avec les id_chord générés
 */
void random_hach(int n, int min, int max,struct pair *pairs) 
{
    int value;
    int i;
    int j;
    srand(time(NULL));

    printf("ID_CHORDS -> [");
    for (i = 0; i < n; i++) {
        rand:
        value = min + (rand() % max);
        for (j = 0; j < n; j++)
            if (pairs[j].chord_id == value)
                goto rand; /* id pas unique, on recalcule */
        pairs[i].chord_id = value;
        printf("(%d)", value);
    }
    printf("]\n");
}

/**
 * @brief   Fonction d'echange de deux valeurs pour le tri a bulle
 * 
 * @param pa a mettre a la position pb
 * @param pb a mettre a la position pa
 */
void swap(struct pair *pa,struct pair * pb)
{
    struct pair ptmp = *pa;
    *pa=*pb;
    *pb=ptmp;
}

/**
 * @brief : Fonction triant un array de pairs en fonction de leur chord id
 * 
 * @param pairs array de pairs que l'on modifie par adresse
 */
void trie_pairs(struct pair *pairs)
{
    int i,j;
    for (i = 0; i < NB_SITE; i++){
        for (j = 0; j < NB_SITE-i-1; j++){
            if (pairs[j].chord_id > pairs[j+1].chord_id){
                swap(&pairs[j],&pairs[j+1]);
            }
        }
    }
}

/**
 * @brief Fonction de calcul des fingers tables pour chaque pair
 * 
 * @param pairs On calcul la table des fingers et on l'assigne a pair->finger
 *                pour chaque pair
 */
void calcul_finger(struct pair *pairs)
{
    int i;
    int j;
    int k;
    int value;
    struct finger tmp_finger;

    //k : indice du pair à initialiser
    for (k = 0; k < NB_SITE; k++) {
        //i : pour le calcul de la puissance de 2
        for (i = 0; i < M; i++) {
            value = (int) (pow(2,i)+pairs[k].chord_id % (int) pow(2,M));
            //j : indice de l'entrée dans la table des fingers de k
            for (j = NB_SITE-1; j >= 0; j--) {
                if (j == NB_SITE-1 && pairs[j].chord_id < value){
                    tmp_finger.chord_id=pairs[0].chord_id;
                    tmp_finger.mpi_rank=pairs[0].mpi_rank;
                    pairs[k].fingers[i]=(tmp_finger);
                    break;
                }
                if (pairs[j].chord_id>=value){
                    tmp_finger.chord_id=pairs[j].chord_id;
                    tmp_finger.mpi_rank=pairs[j].mpi_rank;
                    pairs[k].fingers[i]=tmp_finger;
                }
            }
        }
    }
}

/**
 * @brief   Fonction permettant de savoir si b < key < a 
 *          On augmente de 2^M-1 a si a < b
 *          On augmente de 2^M-1 key si key < a et a < b
 *          Cela permet d'avoir une relation simple sur a, b et key
 * 
 * @param a id chord du pair executant lookup
 * @param b id chord du finger[i]
 * @param key clé recherchée
 * @return  true si b < key < a
 *          false sinon
 */
int bool_contains(int a, int b, int key)
{
    if (a < b) {
        if(key<a)
            key+=pow(2,M)-1;
        a += pow(2, M) - 1;
    }
    return b < key && key < a;
}

/**
 * @brief   Fonction parcourant la table des fingers du pair executant lookup a la 
 *          recherche du plus grand predecesseur de key
 * 
 * @param key clé recherchée
 * @param pair pair executant lookup
 * @return  -1 si aucun predecesseur de key n'a été trouvé,
 *          id_chord du plus grand predecesseur de key sinon
 */
int find_next(int key, struct pair *pair) 
{
    int i;
    for (i = M-1; i >= 0; i--)
        if (bool_contains(pair->chord_id, pair->fingers[i].chord_id, key))
            return pair->fingers[i].mpi_rank;
    return -1;
}

/**
 * @brief   Fonction gérant le retour de la fonction find_next:
 *              - si aucun prédecesseur n'est trouvé, alors le responsable
 *          notre successeur.
 *              - si un prédecesseur est trouvé, on lui demande d'executer lookup
 *          a son tour
 * 
 * @param key clé recherchée
 * @param source mpi_rank du pair ayant initié la recherche de key
 * @param me pair executant lookup
 */
void lookup(int key, int source, struct pair *me) 
{
    int next;
    int mess[2] = {key, source};
    int vide[2] = {0, 0};
    if (((next = find_next(key, me)) == -1)) {
        printf("RECH -> Je suis %d et le responsable de %d est mpi_rank=%d\n", me->chord_id, key, me->succ);
        MPI_Send(mess, 2, MPI_INT, me->succ, RESPONSABLE, MPI_COMM_WORLD);
    } else {
        printf("RECH -> Le pair %d demande a mpi_rank=%d d'executer lookup\n", me->chord_id, next);
        MPI_Send(mess, 2, MPI_INT, next, LOOKUP, MPI_COMM_WORLD);
    }
}

/**
 * @brief   Fonction gerant les receive pour les pairs, on se bloque
 *          en MPI_Recv et apres on agit en fonction du TAG du message
 * 
 * @param me pair recevant le message
 * @return  -1 si on a reçu le message de terminaison, le pair sort de la
 *          boucle de recv de la fonction pair_init et se termine
 *          0 sinon, on continue a recevoir des messages
 */
int receive(struct pair *me)
{
    int mess[2];
    int vide[2] = {0, 0};
    MPI_Status status;
    MPI_Recv(mess, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    switch (status.MPI_TAG) {
        case RESPONSABLE:
            printf("RECH -> Je suis %d et je suis responsable de %d\n", me->chord_id, mess[0]);
            MPI_Send(vide, 2, MPI_INT, mess[1], RESPONSE, MPI_COMM_WORLD);    
            break;
        case RESPONSE:
            MPI_Send(vide, 2, MPI_INT, NB_SITE, TERMINAISON, MPI_COMM_WORLD); 
            break;
        case LOOKUP:
            printf("RECH -> Je cherche %d, je suis %d\n", mess[0], me->chord_id);
            lookup(mess[0], mess[1], me);
            break;
        case INIT_LOOKUP:
            printf("RECH -> Je cherche %d, je suis %d\n", mess[0], me->chord_id);
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
 * @brief   Fonction commune executée par chaque pair, on initialise la struct
 *          pair en recevant les données du simulateur puis on boucle sur 
 *          des recv tant que l'on a pas reçu de message de terminaison
 * 
 * @param rang rang mpi du pair qui s'initialise
 */
void pair_init(int rang)
{
    struct pair* pair=(struct pair*)malloc(sizeof(struct pair));
    int recv;
    MPI_Status status;
    pair->mpi_rank = rang;
    MPI_Recv(&recv, 1, MPI_INT, MPI_ANY_SOURCE, TAGINIT, MPI_COMM_WORLD, NULL);
    MPI_Recv(&(pair->fingers), M*sizeof(struct finger), MPI_CHAR ,MPI_ANY_SOURCE, TAGINIT, MPI_COMM_WORLD, &status);
    pair->chord_id = recv;
    pair->succ = pair->fingers[0].mpi_rank;

    printf("FINGER TABLE -> (mpi_rank=%d; id_chord=%d) : ", pair->mpi_rank, pair->chord_id);
    printf("[");
    for(int i=0 ; i< M;i ++){
        printf("%d ",pair->fingers[i].chord_id);
    }
    printf("]\n");

    while(receive(pair)!=-1){}
}

/**
 * @brief   Fonction initiant les id chord et calculant les finger tables
 * 
 */
void simulateur(void)
{
    MPI_Status status;
    int i;
    int mess[2];
    int recv[2];
    int vide[2] = {0, 0};
    struct pair *pairs = malloc(NB_SITE * sizeof(struct pair));  

    random_hach(NB_SITE, 0, pow(2,M)-1, pairs);    
    for(int i=0; i< NB_SITE ; i++){
        pairs[i].mpi_rank=i;
    }

    /* Triage de l'array */
    trie_pairs(pairs);

    /* Calcul des finger table */
    calcul_finger(pairs);

    /* Envoi de la finger table de chaque pair */
    for (int i=0 ; i<NB_SITE ;i ++) {
        MPI_Send(&(pairs[i].chord_id),1,MPI_INT,pairs[i].mpi_rank,TAGINIT,MPI_COMM_WORLD);
        MPI_Send(pairs[i].fingers,M*sizeof(struct finger),MPI_CHAR,pairs[i].mpi_rank,TAGINIT,MPI_COMM_WORLD);
    }

    srand(time(NULL));
    /* Selection d'une clé aléatoire */
    mess[0] = rand() % (int) pow(2, M);
    /* Selection d'un pair aleatoire */
    mess[1] = pairs[rand() % NB_SITE].mpi_rank;
    /* Envoi de la recherche */
    MPI_Send(mess, 2, MPI_INT, mess[1], INIT_LOOKUP, MPI_COMM_WORLD);
    /* Recv du resultat */
    MPI_Recv(recv, 2, MPI_INT, mess[1], TERMINAISON, MPI_COMM_WORLD, &status);
    /* Send TERMINAISON a tout le monde */
    for (i = 0; i < NB_SITE; i++) {
        MPI_Send(vide, 2, MPI_INT, i, TERMINAISON, MPI_COMM_WORLD);
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

    if (rang == NB_SITE) {
        simulateur();
    } else {
        pair_init(rang);
    }
    MPI_Finalize();
    return 0;
}


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
    printf("]\n\n");
}

void swap(void * a, void * b, size_t len)
{
    unsigned char * p = a, * q = b, tmp;
    for (size_t i = 0; i != len; ++i){
        tmp = p[i];
        p[i] = q[i];
        q[i] = tmp;
    }
}



void trie_fingers(struct finger *fingers,int size)
{
    int i,j;
    for (i = 0; i < size; i++){
        for (j = 0; j < size-i-1; j++){
            if (fingers[j].chord_id > fingers[j+1].chord_id){
                swap(&fingers[j],&fingers[j+1],sizeof(struct finger));
            }
        }
    }
}

void afficher_fingers(struct pair *pair)
{
        printf(" FINGER TABLE -> (mpi_rank=%d; id_chord=%d) : [", pair->mpi_rank, pair->chord_id);
        for(int j=0 ; j< M; j++){
            printf("%d ", pair->fingers[j].chord_id);
        }
    printf("]\n");
}

void afficher_reverse(struct pair * pair)
{
    int i=0;
    printf(" REVRSE TABLE -> (mpi_rank=%d; id_chord=%d) : [", pair->mpi_rank, pair->chord_id);
    while(pair->reverse[i].chord_id!= -1){
        printf("%d ",pair->reverse[i].chord_id);
        i++;
    }
    printf("]\n");
}

void trie_tab(int *tab,int size)
{
    int i,j;
    for (i = 0; i < size; i++){
        for (j = 0; j < size-i-1; j++){
            if (tab[j] > tab[j+1]){
                swap(&tab[j],&tab[j+1],sizeof(int));
            }
        }
    }
}



/**
 * @brief : Fonction triant un array de pairs en fonction de leur chord id
 * 
 * @param pairs array de pairs que l'on modifie par adresse
 */
void trie_pairs(struct pair *pairs,int size)
{
    int i,j;
    for (i = 0; i < size; i++){
        for (j = 0; j < size-i-1; j++){
            if (pairs[j].chord_id > pairs[j+1].chord_id){
                swap(&pairs[j],&pairs[j+1],sizeof(struct pair));
            }
        }
    }
}


/**
 * @brief Enelever les doublons d'un tableau de finger 
 *  
 * @param finger Le tableau de finger
 * @param size Taille du tableau    
 * @return int Nouvelle taille
 */

int remove_duplicates(struct finger *finger,int size)
{
    int i,j,k;
    for(int i=0 ; i< size;i ++){
        for(int j= i+1; j<size ; j++){
           if(finger[i].chord_id==finger[j].chord_id){
               for(k=j ;k < size-1;k++)
               {
                   finger[k]=finger[k+1];
               }
               size--;
               j--;
           }
        }
    }
    return size;


}

void recalcul_finger(struct pair *me,int *new_pair)
{
    int i;
    int j;
    int k;
    int value;
    int size;
    int cpt=0;
    struct finger *tmp_fingers= malloc(sizeof(struct finger)*M);
    int new_finger= new_pair[0];
    int new_mpi_rank=new_pair[1];
    for(i=0 ; i<M ; i++){
        tmp_fingers[i].chord_id=me->fingers[i].chord_id;
        tmp_fingers[i].mpi_rank=me->fingers[i].mpi_rank;

    }
    //On rajoute le nouveau pair dans la finger table temporaire
    size=remove_duplicates(tmp_fingers,M);
    tmp_fingers[size].chord_id=new_finger;
    tmp_fingers[size].mpi_rank=NB_SITE-1;
    trie_fingers(tmp_fingers,size+1);

    

    for(i= 0 ; i< M ;i++){
        value = (int) (pow(2,i)+me->chord_id) % (int) pow(2,M);
        for(j=size ; j>=0; j--){
            //printf("TMP_finger %d\n",tmp_fingers[j].chord_id);
            if(j== size && value>tmp_fingers[j].chord_id){
                me->fingers[i].chord_id=tmp_fingers[0].chord_id;
                me->fingers[i].mpi_rank=tmp_fingers[0].mpi_rank;

            }
            if(tmp_fingers[j].chord_id>= value){
                me->fingers[i].chord_id=tmp_fingers[j].chord_id;
                me->fingers[i].mpi_rank=tmp_fingers[j].mpi_rank;
            }
        }
    }
    me->succ=me->fingers[0].mpi_rank;
    
}

/**
 * @brief Fonction de calcul des fingers tables pour chaque pair
 * 
 * @param pairs On calcul la table des fingers et on l'assigne a pair->finger
 *                pour chaque pair
 */
void calcul_finger(struct pair *pairs,int size)
{
    int i;
    int j;
    int k;
    int value;
    struct finger tmp_finger;

    //k : indice du pair à initialiser
    for (k = 0; k < size; k++) {
        //i : indice dans la table des fingers
        for (i = 0; i < M; i++) {
            value = (int) (pow(2,i)+pairs[k].chord_id) % (int) pow(2,M);
            //j : indice tous les pairs dans l'ordre croissant
            for (j = size-1; j >= 0; j--) {
                if (j == size -1 && pairs[j].chord_id < value){
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
 * @brief   Fonction de calcul des tables reverse pour chaque pair
 * 
 * @param pairs On calcul la table des reverse et on l'assigne a pair->reverse
 *                pour chaque pair
 */
void calcul_reverse(struct pair *pairs,int size) 
{
    int i;
    int j;
    int k;
    int cpt;
    //i : indice du pair à initialiser
    for (i = 0; i < size; i++) {
        cpt = 0;
        //j : indice de la table des fingers que l'on parcourt
        for (j = 0; j < size; j++) {
            // si la table des fingers est celle du pair que l'on initialise on continue
            if (j == i)
                continue;
            // k : indice dans la finger table que l'on parcourt
            for (k = 0; k < M; k++) {
                if (pairs[j].fingers[k].chord_id != pairs[i].chord_id)
                    continue;
                pairs[i].reverse[cpt].chord_id = pairs[j].chord_id;
                pairs[i].reverse[cpt].mpi_rank = pairs[j].mpi_rank;
                cpt++;
                // on break car on ne veut pas ajouter plusieurs fois le même pair dans la table reverse
                break;
            }
        }
        // remplissage du reste de la table avec des -1
        while (cpt < size) {
            pairs[i].reverse[cpt].chord_id = -1;
            pairs[i].reverse[cpt].mpi_rank = -1;
            cpt++;
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
 * @brief Fonction qu'exécute le nouveau pair qui veut s'insérer dans la DHT
 *        il attend la réception de son ID et du pair à contacter
 * 
 *  
 * 
 */

void wait_insertion()
{
    struct pair *pair= malloc(sizeof(struct pair));
    int vide[2];
    int mess[2];
    int recv[2];
    int ids[NB_SITE-1];
    int indice=0;
    int value;
    int contact;
    MPI_Status status;
    MPI_Recv(mess, 2, MPI_INT, NB_SITE, INSERT, MPI_COMM_WORLD,&status);
    pair->mpi_rank=NB_SITE-1;
    pair->chord_id=mess[0];
    contact=mess[1];
    sleep(2);

    printf("Le nouveau pair %d contacte %d \n",mess[0],mess[1]);

    //Le nouveau pair envoie un msg avec son id à son CONTACT pour  le lookup
    mess[0]= pair->chord_id;
    mess[1]= pair->mpi_rank;
    MPI_Send(mess,2,MPI_INT,contact,INSERT,MPI_COMM_WORLD);
    //Il attend la reponse qui contient son successeur
    MPI_Recv(recv,2,MPI_INT,MPI_ANY_SOURCE,RESPONSE,MPI_COMM_WORLD,&status);
    //Le nouveau pair informe son responsable de dire aux pairs de sa table inverse de recalculer leur finger table avec son id 
    pair->succ=recv[1];
    MPI_Send(mess,2,MPI_INT,pair->succ,ASK_RECALCUL,MPI_COMM_WORLD);
    //Il envoie un message qui fait le tour de l'anneau ou chaque pair met son id
    MPI_Send(ids,NB_SITE-1,MPI_INT,pair->succ,FINGER,MPI_COMM_WORLD);
    MPI_Send(&indice,1,MPI_INT,pair->succ,FINGER,MPI_COMM_WORLD);
    MPI_Recv(ids,6,MPI_INT,MPI_ANY_SOURCE,FINGER,MPI_COMM_WORLD,&status);
    
    trie_tab(ids,M);

    for(int i=0 ;i< M; i++){
            value = (int) (pow(2,i)+pair->chord_id) % (int) pow(2,M);
            for(int j= NB_SITE-2; j>= 0 ;j--){
                if (j == NB_SITE-2 && ids[j] < value){
                    pair->fingers[i].chord_id=ids[0];
                    break;
                }
                if (ids[j]>=value){
                    pair->fingers[i].chord_id=ids[j];
                }
                
            }   
    }

    //Envoie d'un message de terminaison au simulateur
    MPI_Send(vide,2,MPI_INT,NB_SITE,TERMINAISON,MPI_COMM_WORLD);
    MPI_Recv(vide,2,MPI_INT,MPI_ANY_SOURCE,TERMINAISON,MPI_COMM_WORLD,&status);
    afficher_fingers(pair);


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
    int mess[NB_SITE-1];
    int send[2];
    int i;
    int indice;
    int new_pair;
    int vide[2] = {0, 0};
    MPI_Status status;
    MPI_Recv(mess, NB_SITE-1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    switch (status.MPI_TAG) {
        case RESPONSABLE:
            printf("RECH -> Je suis %d et je suis responsable de %d, j'envoie à : %d \n", me->chord_id, mess[0],mess[1]);
            send[0]=me->chord_id;
            send[1]=me->mpi_rank;
            MPI_Send(send, 2, MPI_INT, mess[1], RESPONSE, MPI_COMM_WORLD);    
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
        case INSERT:
            printf("INSERT  -> le processus %d veut s'insérer\n",mess[0]);
            lookup(mess[0],mess[1],me);
            break;
        case TERMINAISON:
            return -1;
        case ASK_RECALCUL:
            //Le pair demande à tout les pairs de sa table inverse de recalculer leur finger table avec le nouvel id
            i=0;
            new_pair=status.MPI_SOURCE;
            while( (me->reverse[i].chord_id)!=-1){
                MPI_Send(mess,2,MPI_INT,me->reverse[i].mpi_rank,RECALCUL,MPI_COMM_WORLD);
                i++;
            }
            break;
        case RECALCUL:
            recalcul_finger(me,mess);
            break;
        case FINGER:
            MPI_Recv(&indice,1,MPI_INT,MPI_ANY_SOURCE,FINGER,MPI_COMM_WORLD,&status);
            mess[indice]=me->chord_id;
            indice++;
            printf("\n");
            MPI_Send(mess,NB_SITE-1,MPI_INT,me->succ,FINGER,MPI_COMM_WORLD);
            MPI_Send(&indice,1,MPI_INT,me->succ,FINGER,MPI_COMM_WORLD);
            break;

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
    int i;
    MPI_Status status;
    pair->mpi_rank = rang;
    MPI_Recv(&recv, 1, MPI_INT, MPI_ANY_SOURCE, TAGINIT, MPI_COMM_WORLD, NULL);
    MPI_Recv(&(pair->fingers), M*sizeof(struct finger), MPI_CHAR, MPI_ANY_SOURCE, TAGINIT, MPI_COMM_WORLD, &status);
    MPI_Recv(&(pair->reverse), (NB_SITE-1)*sizeof(struct finger), MPI_CHAR, MPI_ANY_SOURCE, TAGINIT, MPI_COMM_WORLD, &status);
    // receive de la table reverse
    pair->chord_id = recv;      
    pair->succ = pair->fingers[0].mpi_rank;
    afficher_fingers(pair);
    sleep(1);
    afficher_reverse(pair);
    while(receive(pair)!=-1) {}
    afficher_fingers(pair);
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
    int id_insert;
    int unique= 0;
    struct pair *pairs = malloc(NB_SITE * sizeof(struct pair));  

    random_hach(NB_SITE-1, 0, pow(2,M)-1, pairs);    
    for(int i=0; i< NB_SITE-1; i++){
        pairs[i].mpi_rank=i;
    }

    /* Triage de l'array */
    trie_pairs(pairs,NB_SITE-1);

    /* Calcul des finger table */
    calcul_finger(pairs,NB_SITE-1);

    /* Calcul des tables reverse */
    calcul_reverse(pairs,NB_SITE-1);
   
    /* Envoi de la finger table de chaque pair */
    for (int i=0 ; i<NB_SITE-1 ;i ++) {
        MPI_Send(&(pairs[i].chord_id),1,MPI_INT,pairs[i].mpi_rank,TAGINIT,MPI_COMM_WORLD);
        MPI_Send(pairs[i].fingers, M*sizeof(struct finger), MPI_CHAR,pairs[i].mpi_rank, TAGINIT, MPI_COMM_WORLD);
        MPI_Send(pairs[i].reverse, (NB_SITE-1)*sizeof(struct finger), MPI_CHAR,pairs[i].mpi_rank, TAGINIT, MPI_COMM_WORLD);
    }

    //Tirage de l'id du nouveau proc à insérer et du pair qu'il peut contacter
    srand(time(NULL));
    while(!(unique)){
            rand2:
            id_insert= rand() % (int) pow(2, M);
            for(int i=0 ; i< NB_SITE-1 ;i++){
                if(pairs[i].chord_id == id_insert){
                    unique=0;
                    goto rand2;

                }
            }
            unique=1;
    }
    mess[0]= id_insert;
    mess[1]= rand()% (NB_SITE -1);
   
    //Envoie de son id au nouveau pair à insérer
    MPI_Send(mess, 2, MPI_INT, NB_SITE-1, INSERT, MPI_COMM_WORLD);
    //Attente de la terminaison
    MPI_Recv(recv, 2, MPI_INT, MPI_ANY_SOURCE, TERMINAISON, MPI_COMM_WORLD, &status);
    pairs[NB_SITE-1].chord_id = id_insert;
    pairs[NB_SITE-1].mpi_rank=NB_SITE-1;
   
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
        //Par convention le pair qui s'insère a le rang NB_SITE -1
        if(rang == NB_SITE- 1){ 
            wait_insertion();
        }else{
            pair_init(rang);
        }
    }
    MPI_Finalize();
    return 0;
}


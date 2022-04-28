/**
 * @file Exercice2.c
 * @author Firas Jebari & Titouan Polit
 * @brief Calcul des finger tables en distribué
 * @version 1
 * @date 2022-04-28
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "dht.h"

/*
 * ============================================================================
 *                           FONCTIONS POUR CHORD
 * ============================================================================
 */

/**
 * @brief   Fonction de hachage random des id
 *          Les id_mpi seront liés à la case du tableau renvoyé
 * 
 * @param n nombre d'id a calculer
 * @param min borne inf des id
 * @param max borne sup des id
 * @param pairs tableau d'entiers que l'on va remplir avec les chord id
 */
void random_hach(int n, int min, int max, int *pairs) 
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
            if (pairs[j] == value)
                goto rand; /* id pas unique, on recalcule */
        pairs[i] = value;
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
 * @brief   Fonction triant un array de pairs en fonction de leur chord id
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

    /* k : indice du pair à initialiser */
    for (k = 0; k < NB_SITE; k++) {
        /* i : pour le calcul de la puissance de 2 */
        for (i = 0; i < M; i++) {
            value = (int) (pow(2,i)+pairs[k].chord_id % (int) pow(2,M));
            /* j : indice de l'entrée dans la table des fingers de k */
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
 * @brief   Fonction executee par le leader pour calculer les finger table de
 *          tous les pairs. Il commence par recevoir tous les couple {mpi_rank, id_chord}
 *          de tous les pairs. Une fois reçus il calcule la table de fingers
 *          de chacun des pairs puis leur envoie.
 * 
 * @param rang mpi_rank du leader
 * @param chord_id chord_id du leader
 */
void leader_work(int rang, int chord_id)
{
    int i;
    int index = 1;
    MPI_Status status;
    /* Initialisation de l'array contenant tout les pairs */
    struct pair *pairs = malloc(NB_SITE * sizeof(struct pair));  
    pairs[0].chord_id = chord_id;
    pairs[0].mpi_rank = rang;
    for (i = 0; i < NB_SITE; i++) {
        if (i == rang)
            continue;
        MPI_Recv(&(pairs[index].chord_id), 1, MPI_INT, i, CHORDID, MPI_COMM_WORLD, &status);
        pairs[index].mpi_rank = status.MPI_SOURCE;
        index++;
    }
    /* Triage de l'array */
    trie_pairs(pairs);
    /* Calcul des finger table */
    calcul_finger(pairs);
    /* Envoi de la finger table de chaque pair */
    for (int i=0; i < NB_SITE; i++) {
        MPI_Send(pairs[i].fingers, M*sizeof(struct finger), MPI_CHAR, pairs[i].mpi_rank, TAGINIT, MPI_COMM_WORLD);
    }
}

/**
 * @brief   Fonction commune executée par chaque pair, on initialise la struct
 *          pair en recevant les données du leader puis on affiche notre finger
 *          table
 * 
 * @param rang rang mpi du pair qui s'initialise
 * @param chord_id id chord du pair qui s'initialise
 */
void pair_init(int rang, int chord_id)
{
    struct pair *pair = (struct pair*) malloc(sizeof(struct pair));
    int recv;
    MPI_Status status;
    pair->mpi_rank = rang;
    pair->chord_id = chord_id;
    MPI_Recv(&(pair->fingers), M*sizeof(struct finger), MPI_CHAR ,MPI_ANY_SOURCE, TAGINIT, MPI_COMM_WORLD, &status);
    pair->succ = pair->fingers[0].mpi_rank;

    printf("FINGER TABLE -> (mpi_rank=%d; id_chord=%d) : ", pair->mpi_rank, pair->chord_id);
    printf("[");
    for(int i=0 ; i< M;i ++){
        printf("%d ",pair->fingers[i].chord_id);
    }
    printf("]\n");
}


/*
 * ============================================================================
 *                    FONCTIONS POUR ELECTION DU LEADER
 * ============================================================================
 */

/**
 * @brief   Reception d'un message OUT, si la distance est a 0 alors on renvoie 
 *          un message IN a l'emetteur, sinon on envoie a l'autre voisin un 
 *          message OUT avec distance - 1
 * 
 * @param me pair executant la fonction
 * @param recv 0 : id de l'initiateur, 1 : distance
 * @param status contenu du message recu
 */
void recevoir_out(struct process *me, int recv[2], MPI_Status status)
{
    int mess[2];
    if (!me->initiateur || recv[0] > me->mpi_rank) {
        me->state = BATTU;
        if (recv[1] > 0) {
            mess[0] = recv[0];
            mess[1] = recv[1]-1;
            if (status.MPI_SOURCE == me->vg) {
                MPI_Send(mess, 2, MPI_INT, me->vd, OUT, MPI_COMM_WORLD);
            }
            if (status.MPI_SOURCE == me->vd) {
                MPI_Send(mess, 2, MPI_INT, me->vg, OUT, MPI_COMM_WORLD);
            }
        } else {
            mess[0] = recv[0];
            mess[1] = -1;
            if (status.MPI_SOURCE == me->vg) {
                MPI_Send(mess, 2, MPI_INT, me->vg, IN, MPI_COMM_WORLD);
            }
            if (status.MPI_SOURCE == me->vd) {
                MPI_Send(mess, 2, MPI_INT, me->vd, IN, MPI_COMM_WORLD);
            }
        }
    } else {
        if (recv[0] == me->mpi_rank) {
            printf("Je suis (mpi_rank=%d) et je suis elu\n", me->mpi_rank);
            me->state = ELU;
            annonce_election(me);
        } /* Sinon on ne fait rien, on supprime le jeton */
    }
}

/**
 * @brief   Reception d'un message IN, si l'initiateur du token est nous-meme
 *          alors on augmente le nombre de token qui sont revenus. Si on a 
 *          reçu nos deux token alors on passe a l'etape suivante. Si nous ne
 *          sommes pas l'initiateur du token, on fait suivre le token a notre 
 *          autre voisin.
 * 
 * @param me pair executant la fonction
 * @param recv 0 : initiateur, 1 : la distance,  NULL ici puisque le token revient
 * @param status contenu du message
 */
void recevoir_in(struct process *me, int recv[2], MPI_Status status)
{
    int mess[2] = {recv[0], -1}; // distance = NULL
    if (recv[0] != me->mpi_rank) {
        if (status.MPI_SOURCE == me->vg) {
            MPI_Send(mess, 2, MPI_INT, me->vd, IN, MPI_COMM_WORLD);
        } 
        if (status.MPI_SOURCE == me->vd) {
            MPI_Send(mess, 2, MPI_INT, me->vg, IN, MPI_COMM_WORLD);
        }
    } else {
        (me->nb_in)++;
        if (me->nb_in == 2) {
            (me->etape)++;
            initier_etape(me);
        }
    }
}

/**
 * @brief   Fonction bloquant sur un Recv puis executant un code spécifique 
 *          en fonction du TAG du message reçu
 * 
 * @param me pair executant la fonction
 */
void recevoir_election(struct process *me) 
{
    int recv[2];
    int mess[2];
    MPI_Status status;
    MPI_Recv(recv, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    switch (status.MPI_TAG) {
        case OUT:
            recevoir_out(me, recv, status);
            break;
        case IN:
            recevoir_in(me, recv, status);
            break;
        case LEADER:
            /* On fait passer le message d'annonce du leader */
            MPI_Send(recv, 2, MPI_INT, me->vd, LEADER, MPI_COMM_WORLD);
            printf("Pour (mpi_rank=%d) l'election est finie %d est le chef\n", me->mpi_rank, recv[0]);
            /* Envoi au leader de mon chord_id */
            MPI_Send(&(me->chord_id), 1, MPI_INT, recv[0], CHORDID, MPI_COMM_WORLD);
            me->state = ENDELEC;
            break;
        default:
            printf("Tag de message inconnu\n");
    }
}

/**
 * @brief   Fonction permettant au pair elu d'annoncer son election aux autres
 *          pairs en faisant circuler un message LEADER dans l'anneau.
 * 
 * @param me pair annoncant son election
 */
void annonce_election(struct process *me)
{
    MPI_Status status;
    int recv[2];
    int mess[2] = {me->mpi_rank, 0};
    MPI_Send(mess, 2, MPI_INT, me->vd, LEADER, MPI_COMM_WORLD);
    MPI_Recv(recv, 2, MPI_INT, me->vg, LEADER, MPI_COMM_WORLD, &status);
}

/**
 * @brief   Fonction permettant d'initier une etape de l'election en envoyant deux
 *          tokens a une distance 2^me->etape
 * 
 * @param me pair initiant l'etape me->etape
 */
void initier_etape(struct process *me)
{
    int mess[2] = {me->mpi_rank, pow(2, me->etape)};
    me->nb_in = 0;
    MPI_Send(mess, 2, MPI_INT, me->vd, OUT, MPI_COMM_WORLD);
    MPI_Send(mess, 2, MPI_INT, me->vg, OUT, MPI_COMM_WORLD);
}


/**
 * @brief   Fonction executée par les pairs pour l'election du leader
 * 
 * @param rang rang mpi du pair executant la fonction d'election
 */
struct process* election(int rang) 
{
    MPI_Status status;
    struct process *me = malloc(sizeof(struct process));
    me->mpi_rank = rang;
    me->vd = (rang-1+NB_SITE) % NB_SITE;
    me->vg = (rang+1) % NB_SITE;
    me->nb_in = 0;
    me->etape = 0;
    me->state = NSP;
    MPI_Recv(&(me->initiateur), 1, MPI_INT, NB_SITE, INIT, MPI_COMM_WORLD, &status);
    MPI_Recv(&(me->chord_id), 1, MPI_INT, NB_SITE, INIT, MPI_COMM_WORLD, &status);
    if (me->initiateur) {
        printf("(mpi_rank=%d) est CANDIDAT\n", me->mpi_rank);
        initier_etape(me);
        while (me->state != ENDELEC && me->state != ELU) {
            recevoir_election(me);
        }
    } else {
        while (me->state != ENDELEC) {
            recevoir_election(me);
        } 
    }
    return me;
}

/**
 * @brief   Fonction choississant aléatoirement au moins 1 initiateur dans les pairs 
 *          qui sera candidat à l'election.
 */
void simulateur_election()
{
    int i;
    int cpt = 0;
    int boolean;
    int isCandidat[NB_SITE];
    int *pairs = malloc(sizeof(int)*NB_SITE);
    srand(time(NULL));
    /* Calcul des identifiants CHORD */
    random_hach(NB_SITE, 0, pow(2,M)-1, pairs);
    /* Choix de au max nb_initiateurs >= 1*/
    init:
    for (i = 0; i < NB_SITE; i++) {
        boolean = rand() % 2;
        if (boolean) {
            isCandidat[i] = 1; // true
            cpt++;
        } else {
            isCandidat[i] = 0; // false
        }
    }
    if (cpt < 1)
        goto init;

    /* Envoi aux processus si ils sont candidats ou pas */
    for (i = 0; i < NB_SITE; i++) {
        MPI_Send(&isCandidat[i], 1, MPI_INT, i, INIT, MPI_COMM_WORLD);
        MPI_Send(&pairs[i], 1, MPI_INT, i, INIT, MPI_COMM_WORLD);
    }
}

/**
 * @brief   Main Lançant les processus
 */
int main(int argc, char* argv[]) {
    int nb_proc,rang;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);
    struct process *tmp; // utile apres l'election

    if (nb_proc != NB_SITE+1) {
        printf("Nombre de processus incorrect !\n");
        MPI_Finalize();
        exit(2);
    }
    MPI_Comm_rank(MPI_COMM_WORLD,&rang);

    if (rang == NB_SITE) {
        simulateur_election();
    } else {
        tmp = election(rang);
        if (tmp->state == ELU) {
            leader_work(rang, tmp->chord_id);
        }
        pair_init(rang, tmp->chord_id);
    }
    MPI_Finalize();
    return 0;
}
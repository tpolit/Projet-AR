#!/bin/bash

# Script créé par Mazigh Saoudi

cd Exercice1
mpicc -o exercice1 Exercice1.c -lm
echo -e "\n+=============================================================================+"
echo -e "|                     Running exercice1: M = 5, N = 6                         |"
echo -e "+=============================================================================+\n"
mpirun -np 6 --oversubscribe exercice1

echo -e "\n+=============================================================================+"
echo -e "|                     Running exercice2: M = 5, N = 6                         |"
echo -e "+=============================================================================+\n"
cd ../Exercice2
mpicc -o exercice2 Exercice2.c -lm
mpirun -np 6 --oversubscribe exercice2 

echo -e "\n+=============================================================================+"
echo -e "|                     Running exercice3 : M = 5, N = 6                         |"
echo -e "+=============================================================================+\n"
cd ../Exercice3
mpicc -o exercice3 Exercice3.c -lm
mpirun -np 6 --oversubscribe exercice3

Dijkstra
========
16.7.2012

Sequencial and parallel Dijkstar Implementaiton using OpenMP and OpenMPI

Compilation and execution
-------------------------
The programs can be compiled using gcc by running

gcc -fopenmp dijkstra_omp.c Dijkstra_tools.c -o dijkstra_omp

and

dijkstra.c Dijkstra_tools.c -o dijkstra

and

mpicc -fopenmp dijkstra_ompMPI.c Dijkstra_tools.c -o dijkstra_ompMPI


There are two valid program arguments. The first specifies the number of Nodes
of the graph that will be generated at runtime to test the Dijkstra algoritm,
and the second optional argument is the value passed to the srand() function.
Rerunning the application with the same both arguments ensures that the same

Matrix is generated!

./dijkstra 1000 19

or

./dijkstra_omp 1000 19

or

mpirun -n 8 ./dijkstra_ompMPI 1000 19


Blame
-----

Author: Pasieka Manuel , manuel.pasieka@protonmail.ch

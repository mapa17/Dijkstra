/*
 * dijkstra.c
 *
 *  Created on: Jul 16, 2012
 *      Author: Manuel Pasieka , mapa17@posgrado.upv.es
 *
 *  Simple Dijkstra algorithm implementation
 *  http://en.wikipedia.org/wiki/Dijkstra%27s_algorithm
 *
 *  Documentation of OpenMP
 *  https://computing.llnl.gov/tutorials/openMP/
 */

#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <omp.h>
#include "mpi.h"

#include "Dijkstra_tools.h"

long N;
int randInit;

int mpi_size, mpi_id;

void inputCheck(int argc, char** argv);
void printUsage(int argc, char** argv);
void testScheduler(int nThreads, graph* G, char debug);
void dijkstra(graph* G, long initial_node, char debug);
void par_dijkstra(graph* G, long initial_node, long nStart, long nEnd, char debug);

int main(int argc, char **argv)
{
	long i;
	graph G;
	char debugFlag = 0;

	debugFlag=0;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank (MPI_COMM_WORLD, &mpi_id);        /* get current process id */
	MPI_Comm_size (MPI_COMM_WORLD, &mpi_size);        /* get number of processes */

	inputCheck(argc, argv);

	if(mpi_id == 0)
	{
		if(N == 1){
			generateTestGraph(&G);
		} else {
			generateGraph(N, randInit, &G, debugFlag);
		}
	} else {
		if(N == 1)
			N = 6;
		generateEmptyGraph(N, &G);
	}
	N = G.N;

	if(debugFlag){
		enableDebug(N);
	}

	if((debugFlag == 1) && (mpi_id == 0) ){
		printf("Using graph\n");
		printGraph(&G);
	}

	if(mpi_id == 0) printf("\nTesting with max 2 Threads\n");
	testScheduler(2,&G, debugFlag);

	if(mpi_id == 0) printf("\nTesting with max 4 Threads\n");
	testScheduler(4,&G, debugFlag);

	if(mpi_id == 0) printf("\nTesting with max 8 Threads\n");
	testScheduler(8,&G, debugFlag);

//	omp_set_num_threads(4);
//	omp_set_schedule(omp_sched_static, 2);
//	dijkstra(&G, 0, 0);
//	char *b;
//	b = malloc(G.N * 5);
//	if(b == NULL) {perror("malloc"); exit(EXIT_FAILURE); }
//	sprintf(b,"\nLowest distances!\nD=[");
//	for(i = 0; i<G.N; i++){
//		sprintf(&b[strlen(b)], "%d,", G.D[i]);
//	}
//	printf("%s]\n", b);

	MPI_Finalize();
	return EXIT_SUCCESS;
}

void testScheduler(int nThreads, graph* G, char debug)
{
	double runtime;

	//Set max nThreads
	omp_set_num_threads(nThreads);

	if(mpi_id == 0) printf("Scheduler (Static, %d)", G->N/100 );
	resetGraph(G);
	omp_set_schedule(omp_sched_static, G->N/100);
	if(mpi_id == 0) tick();
		dijkstra(G, 0, debug);
	if(mpi_id == 0){
		runtime = tack();
		printf("working for [%f] sec.\n",runtime);
	}


	if(mpi_id == 0) printf("Scheduler (dynamic, %d)", G->N/100 );
	resetGraph(G);
	omp_set_schedule(omp_sched_dynamic, G->N/100);
	if(mpi_id == 0) tick();
		dijkstra(G, 0, debug);
	if(mpi_id == 0){
		runtime = tack();
		printf("working for [%f] sec.\n",runtime);
	}

	if(mpi_id == 0) printf("Scheduler (guided, %d)", G->N/100 );
	resetGraph(G);
	omp_set_schedule(omp_sched_guided, G->N/100);
	if(mpi_id == 0) tick();
		dijkstra(G, 0, debug);
	if(mpi_id == 0){
		runtime = tack();
		printf("working for [%f] sec.\n",runtime);
	}
}

void dijkstra(graph* G, long initial_node, char debug)
{
	long nStart, nEnd;
	long nNodes, nOffset;
	int i;

	MPI_Bcast( G->node[0], G->N*G->N, MPI_CHAR, 0, MPI_COMM_WORLD);

	nNodes = G->N/mpi_size; //Number of nodes for each process
	nOffset = G->N % nNodes; //The first gets more
	if(mpi_id == 0){
		nStart = 0;
		nEnd = nNodes + nOffset;
	} else {
		nStart = (mpi_id * nNodes) + nOffset;
		nEnd = nStart + nNodes;
	}


	if( debug == 1){
		//Print local to process data
		printf("[%d] Processing node [%d-%d]\n",mpi_id, nStart, nEnd);
	}

//	int i,j;
//	printf("[%d] [", mpi_id);
//	for(i = nStart; i < nEnd; i++){
//		for(j = 0; j < G->N; j++){
//			printf("%03d,",G->node[i][j]);
//		}
//		printf("]\n[%d] [", mpi_id);
//	}

	par_dijkstra(G, 0, nStart, nEnd, debug);

	if( debug == 1)
	{
		printf("[%d] Partial D result\n",mpi_id);
		printf("[%d] [", mpi_id);
		for(i = nStart; i < nEnd; i++){
			printf("%03d,",G->D[i]);
		}
		printf("]");
	}

	if(mpi_id == 0){
		printf(" Complete D result [");
		if(G->N > 20)
			nEnd = 20;
		else
			nEnd = G->N;
		for(i = 0; i < nEnd; i++){
			printf("%d,",G->D[i]);
		}
		if(G->N > 20)
			printf(" ... ]");
		else
			printf("]");
	}
}

void par_dijkstra(graph* G, long initial_node, long nStart, long nEnd, char debug)
{
	long i,j,k;
	int aN; //actualNode
	int nextAN[2];

	//G->D[initial_node] = 0;
	//aN = initial_node;
	nextAN[1] = initial_node;
	nextAN[0] = 0;

	if(debug){
		printf("Calculating nodes [%d - %d]\n", nStart, nEnd);
		printf("Running dijkstra on graph\n");
		printGraph(G);
	}


	for(i = 0; i < G->N; i++)
	{
		aN = nextAN[1];
		G->D[aN] = nextAN[0];
		G->visited[aN] = VISITED;

		if(debug){
			printf("[%d] It[%d] aN [%d]\n",mpi_id, i, aN); printStatus(G); printf("\n");
		}

		//Find all nodes connected to aN
		#pragma omp parallel for schedule(runtime)
		for(j=nStart;j<nEnd;j++){
			if( (G->node[aN][j] != NO_CONN) ){
				if( (G->D[aN] + G->node[aN][j]) < G->D[j] ){
					G->D[j] = (G->D[aN] + G->node[aN][j]);
				}
			}
		}

		nextAN[1] = par_getNextNode(G, nStart, nEnd); //Which node
		if(nextAN[1] == -1){
			nextAN[0] = INF;
			nextAN[1] = 0; //-1 cant bet set as index for MPI_MINLOC operation, so set INF as value so it wont be chosen
		} else {
			nextAN[0] = G->D[nextAN[1]];				  //Distance of this node
		}

		//printf("[%d] nextAn[ %d , %d]\n", mpi_id, nextAN[1], nextAN[0] );
		MPI_Allreduce(nextAN,nextAN,1,MPI_2INT,MPI_MINLOC,MPI_COMM_WORLD);

		//printf("[%d] new aN [%d,%d]\n", mpi_id, aN , G->D[aN]);
	}
}

void printUsage(int argc, char** argv){
	printf("Usage: %s NUMER_OF_POINTS [SRAND_INIT_VALUE]\n",argv[0]);
}

void inputCheck(int argc, char** argv)
{
	if ( argc < 2){
		printUsage(argc, argv);
		exit(EXIT_FAILURE);
	}

	errno = 0; // To distinguish success/failure after call
	N = atol(argv[1]);

	/* Check for various possible errors */

	if ((errno == ERANGE && (N == LONG_MAX || N == LONG_MIN)) || (errno != 0 && N == 0)) {
	   perror("atol");
	   exit(EXIT_FAILURE);
	}

	if(N <= 0){
		printf("Invalid number of points! Number of points must be bigger than zero.\n");
		exit(EXIT_FAILURE);
	}

	//Check for second argument being the random init value
	if(argc == 3)
	{
		errno = 0; // To distinguish success/failure after call
		randInit = atol(argv[2]);

		/* Check for various possible errors */

		if ((errno == ERANGE && (randInit == LONG_MAX || randInit == LONG_MIN)) || (errno != 0 && randInit == 0)) {
		   perror("atol");
		   exit(EXIT_FAILURE);
		}
	} else {
		randInit = -1;
	}

	if(randInit < 0){
		time_t t1;
		struct tm* t2 = NULL;
		int sec;
		t1 = time( NULL );
		t2 = gmtime(&t1);
		sec = t2->tm_sec;
		randInit = sec;
	}
}

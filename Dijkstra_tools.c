/*
 * Dijkstra_tools.c
 *
 *  Created on: Jul 16, 2012
 *      Author: Manuel Pasieka , mapa17@posgrado.upv.es
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <string.h>
#include <assert.h>

#include "Dijkstra_tools.h"


struct timeval start;
char *b = NULL;

void tick(void){
	gettimeofday(&start, NULL);
}

double tack(void){
	struct timeval end;
	if( gettimeofday(&end, NULL) != 0){
		printf("Getting time failed!\n");
	}
//	printf("S %d:%d , E %d:%d\n", start.tv_sec, start.tv_usec, end.tv_sec, end.tv_usec);
	return (double) end.tv_sec-start.tv_sec + ( end.tv_usec - start.tv_usec ) / 1000000.0;
}

void generateTestGraph(graph* G)
{
	char* p;

	generateEmptyGraph(6,G);
	//generateGraph(6,0,G,1);
	p = &G->node[0][0];
	*p = 0; p++; *p=40; p++; *p=15; p++; *p=-1; p++; *p=-1; p++; *p=-1;

	p = &G->node[1][0];
	*p = 40; p++; *p=0; p++; *p=20; p++; *p=10; p++; *p=25; p++; *p=6;

	p = &G->node[2][0];
	*p = 15; p++; *p=20; p++; *p=0; p++; *p=100; p++; *p=-1; p++; *p=-1;

	p = &G->node[3][0];
	*p = -1; p++; *p=10; p++; *p=100; p++; *p=0; p++; *p=-1; p++; *p=-1;

	p = &G->node[4][0];
	*p = -1; p++; *p=25; p++; *p=-1; p++; *p=-1; p++; *p=0; p++; *p=8;

	p = &G->node[5][0];
	*p = -1; p++; *p=6; p++; *p=-1; p++; *p=-1; p++; *p=8; p++; *p=0;
}

#define MAX_EDGE_WEIGTH 100

void generateEmptyGraph(long N, graph *G)
{
	int i;

	assert((N > 0) && "N has to be bigger than zero!" );

	G->N = N;
	G->D = (int*)malloc(sizeof(int) * G->N );
	if(G->D == NULL) { perror("malloc"); exit(EXIT_FAILURE); }
	G->visited = (char*)malloc(sizeof(char) * G->N);
	if(G->visited == NULL) { perror("malloc"); exit(EXIT_FAILURE); }

	//Ensure continues data array (for OpenMPI send)
	char* temp;
	temp = (char*) malloc(G->N*G->N);
	if(temp == NULL) { perror("malloc"); exit(EXIT_FAILURE); }
	G->node = (char**)malloc(sizeof(char*) * G->N);
	if(G->node == NULL) { perror("malloc"); exit(EXIT_FAILURE); }
	for(i = 0; i < G->N; i++){
		G->node[i] = &temp[i*G->N];
	}
}

void enableDebug(long N)
{
	b = malloc(10 * (N*N) + 400);
	if(b == NULL){
		printf("No memory for debug messages!");
		return;
	}
}

void generateGraph(long N, int randInit, graph *G, char debug)
{
	long linkCnt;
	long i,j;
	long t;
	printf("Generating Graph of size %dx%d with init [%d]!\n", N,N, randInit);

	assert((N > 0) && "N has to be bigger than zero!" );
	assert((randInit >= 0) && "randInit has to be bigger than zero!" );

	srand(randInit);

	generateEmptyGraph(N, G);
	resetGraph(G);

	//Initialize Matrix
	for(i = 0; i< G->N; i++)
	{
		linkCnt=0; //Keep track of # of outgoing edges
		for(j = 0; j < G->N; j++)
		{
			if(i == j){
				t = NO_CONN;
			} else {
				t = (rand() % ((MAX_EDGE_WEIGTH-1) * 2)+1); //50% of having no connection
				if(t > MAX_EDGE_WEIGTH){
					//t = INF; //Like no connection
					t = NO_CONN; //Like no connection
				} else {
					linkCnt++;
					G->visited[j] = VISITED; //Do this to find isolated nods that have no incomming edge
				}
			}

			G->node[i][j] = t;
		}

		//Be sure to only generate fully connected graphs by each node having at least one edge to someone else!
		if(linkCnt == 0)
		{
			printf("Adding outgoing link for [%d]\n", i);
			t = rand() % (G->N);

			if(t == i) //NO self loops
				t = (t*t)%G->N;

			G->node[i][t] = rand() % (MAX_EDGE_WEIGTH);
		}
	}

	//To nods that have no incoming edge, add one randomly
	for(i=0;i < G->N; i++){
		if(G->visited[i] != VISITED){
			t = rand() % (G->N);

			if(t == i) //NO self loops
				t = (t+1)%G->N;

			printf("Adding incomming link for %d -> %d\n", t, i);
			G->node[t][i] = rand() % (MAX_EDGE_WEIGTH);
		}

		//No set it to unvisited for initialization
		G->visited[i] = NOT_VISITED;
	}
}

long getNextNode(graph* G)
{
	long i;
	long minD;
	long nextNode;

	nextNode = -1;
	minD = INF;

	//Find unvisited node with lowest Distance to the intial node
	for(i = 0; i<G->N; i++){
		if( (G->visited[i] == NOT_VISITED) && (G->D[i] < minD) ){
			minD = G->D[i];
			nextNode = i;
		}
	}

	return nextNode;
}

long par_getNextNode(graph* G, long nStart, long nEnd)
{
	long i;
	long minD;
	long nextNode;

	nextNode = -1;
	minD = INF;

	//Find unvisited node with lowest Distance to the initial node
	for(i = nStart; i<nEnd; i++){
		if( (G->visited[i] == NOT_VISITED) && (G->D[i] < minD) ){
			minD = G->D[i];
			nextNode = i;
		}
	}

	return nextNode;
}


void printGraph(graph* G)
{
	long i,j;
	//Debug Print

	if(b == NULL)
		return;

	b[0] = 0;
	printf("       ");
	for(i = 0; i<G->N; i++){
		sprintf(&b[strlen(b)], " [%03d] ", i);
	}
	sprintf(&b[strlen(b)], "\n\n");
	for(i = 0; i<G->N; i++)
	{
		sprintf(&b[strlen(b)], "[%03d]  ",i);
		for(j = 0; j < G->N; j++)
		{
			sprintf(&b[strlen(b)], "  %03d  ", G->node[i][j]);
		}
		sprintf(&b[strlen(b)], "  [%03d]\n",i);
	}
	sprintf(&b[strlen(b)], "\n       ");
	for(i = 0; i<G->N; i++){
		sprintf(&b[strlen(b)], " [%03d] ", i);
	}
	sprintf(&b[strlen(b)], "\n");
	printf(b);
}

void printStatus(graph* G)
{
	long i,k;
	//Debug Print

	if(b == NULL)
		return;

	b[0] = 0;
	sprintf(b,"D=[");
	for(k = 0; k<G->N; k++){
		sprintf(&b[strlen(b)], "%ld,", G->D[k]);
	}
	sprintf(&b[strlen(b)],"] V=[");
	for(k = 0; k<G->N; k++){
				sprintf(&b[strlen(b)], "%s,", (G->visited[k]==VISITED)?"X":" ");
			}
	printf("%s]", b);
}

void resetGraph(graph* G)
{
	long i;
	for(i=0; i < G->N; i++){
		G->D[i] = INF;
		G->visited[i] = NOT_VISITED;
	}
}

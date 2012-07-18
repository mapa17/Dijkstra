/*
 * Dijkstra_tools.h
 *
 *  Created on: Jul 16, 2012
 *      Author: Manuel Pasieka , mapa17@posgrado.upv.es
 */

#ifndef DIJKSTRA_TOOLS_H_
#define DIJKSTRA_TOOLS_H_

typedef struct {
	long N; //Number of nodes
	char** node; //Matrix of Nodes and connections

	long* D; //Result of Dijkstra Algorithm. Shortest path length for each node.
	long* visited; //Used to flag which nodes have been visted yet or not.
} graph;

#define INF LONG_MAX
#define NO_CONN -1
#define NOT_VISITED 1
#define VISITED 2

extern struct timeval start;

void tick(void);
double tack(void);

void generateGraph(long N, int randInit, graph *G, char debug);
void generateTestGraph(graph* G);
long getNextNode(graph* G);

void printGraph(graph* G);
void printStatus(graph* G);
void resetGraph(graph* G);

#endif /* DIJKSTRA_TOOLS_H_ */

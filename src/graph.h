/* Based on https://www.cs.yale.edu/homes/aspnes/pinewiki/C(2f)Graphs.html
*/
#include <stdio.h>

typedef struct graph *Graph;

/* create a new graph with n vertices labeled 0..n-1 and no edges */
Graph graph_create(int n);

/* import a graph from a file in adjacency list format */
Graph graph_import(char* inputpath);

/* copy a graph*/
Graph graph_copy(Graph g);

/* free all space used by graph */
void graph_destroy(Graph);

/* add a directed edge to an existing graph */
/* doing this more than once may have unpredictable results */
void graph_add_directed_edge(Graph, int source, int sink);

/* add an undirected edge to an existing graph */
void graph_add_edge(Graph, int vertex1, int vertex2);

/* sort the neighbours array of each vertex */
void graph_sort(Graph);

/* eliminate a vertex from the graph in the context
   of elimination orderings (delete vertex and
   connect its neighbours)*/


/* return the number of vertices/edges in the graph */
int graph_vertex_count(Graph);
int graph_edge_count(Graph);

/* return the out-degree of a vertex */
int graph_out_degree(Graph, int source);

/* return 1 if edge (source, sink) exists), 0 otherwise */
int graph_has_edge(Graph, int source, int sink);

/* Print the graph in adjacency list format*/
void graph_print(Graph g, FILE *stream);

/* invoke f on all edges (u,v) with source u */
/* supplying data as final parameter to f */
/* no particular order is guaranteed */
void graph_foreach(Graph g, int source,
        void (*f)(Graph g, int source, int sink, void *data),
        void *data);
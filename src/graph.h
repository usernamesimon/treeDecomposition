/* Based on https://www.cs.yale.edu/homes/aspnes/pinewiki/C(2f)Graphs.html
*/
#include <stdio.h>

typedef enum strategy { unspecified, degree, fillin, mcs} strategy;

typedef struct graph *Graph;

/* create a new graph with n vertices labeled 0..n-1 and no edges */
Graph graph_create(int n);

/* import a graph from a file in adjacency list format */
Graph graph_import(FILE *fstream);

/* import pre existing ordering from a file
    return 1 on success, 0 otherwise */
char graph_import_ordering(FILE *fstream);

/* copy a graph*/
Graph graph_copy(Graph g);

/* free all space used by graph */
void graph_destroy(Graph);


/* return the number of vertices/edges in the graph */
int graph_vertex_count(Graph);
int graph_edge_count(Graph);

/* return 1 if edge (source, sink) exists), 0 otherwise */
int graph_has_edge(Graph, int source, int sink);

/* calculate an elimination ordering for g
    according to the min degree heuristic.
    Writes the ordering into set, returns
    the width of the ordering.
    ATTENTION: this function alters the graph.
*/
int graph_order_degree (Graph g);

/* Same as graph_order_degree but use 
    min fill-in heuristic
*/
int graph_order_fillin (Graph g);

/* Again same usage, but use 
    maximum cardinality heuristic.
*/
int graph_order_mcs (Graph g);

/* Convert an elimination ordering to a 
    tree decomposition.
    Assumes the elimination ordering was
    calculated prior.
*/
void graph_eo_to_treedecomp (Graph g);

/* return 1 if every node appears exactly
   once in the ordering of g
*/
char graph_ordering_plausible (Graph g);

/* Print the graph in adjacency list format.
    A "d" for the adjacency list of a vertex
    signals that this vertex is deleted
*/
void graph_print(Graph g, FILE *stream);

/* Print the elimination ordering of a graph
    if it has been calculated already
*/
void graph_print_ordering(Graph g, FILE *stream);
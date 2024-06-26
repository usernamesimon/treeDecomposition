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

/* add an undirected edge to an existing graph */
void graph_add_edge(Graph, int vertex1, int vertex2);

/* delete an undirected edge from a graph */
void graph_delete_edge(Graph g, int vertex1, int vertex2);

/* delete node vertex from the graph.
    Note: this does not free its memory.
*/
void graph_delete_vertex(Graph g, int vertex);

/* eliminate a vertex from the graph in the context
   of elimination orderings (delete vertex and
   connect its neighbours)*/
int graph_eliminate_vertex(Graph g, int vertex);

/* return the number of vertices/edges in the graph */
int graph_vertex_count(Graph);
int graph_edge_count(Graph);

/* return the out-degree of a vertex */
int graph_vertex_degree(Graph, int source);

/* return the amount of fill-in edges to be added
    if this vertex was eliminated. 
    Returns max int value for deleted vertex.
    Note: This function only works with undirected graphs.
*/
int graph_vertex_fillin(Graph g, int vertex);

/* return number of neighbours of vertex that
    are in the provided set 
*/
int graph_vertex_cardinality(Graph g, int vertex);

/* return the index of the vertex with minimal degree
*/
int graph_min_degree_index(Graph g);

/* return the index of vertex that results in
    minimal fill-in edges upon elimination */
int graph_min_fillin_index(Graph g);

/* return index of the vertex that has 
    most neighbours in a provided set
*/
int graph_max_cardinality_index(Graph g);

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

/* return 1 if every node appears exactly
   once in the ordering of g
*/
char graph_ordering_plausible (Graph g);

/* Print the graph in adjacency list format.
    A "d" for the adjacency list of a vertex
    signals that this vertex is deleted
*/
void graph_print(Graph g, FILE *stream);
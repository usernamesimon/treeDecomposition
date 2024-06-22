/* Based on https://www.cs.yale.edu/homes/aspnes/pinewiki/C(2f)Graphs.html
 */
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "graph.h"

struct node_t
    {
        int d;          /* number of successors */
        char is_deleted; /* deletion flag */
        char in_set;    /* true if this node is included in a mcs set*/
        int* neighbours; /* list of neighbours for iterating purposes*/
        int len; /* size of neigbour_list */
        
    };

struct graph
{
    int n; /* number of vertices */
    int m; /* number of edges */
    struct node_t **nodes; /* holds meta information of each vertex*/
    /* if a vertex is deleted, n decreases
        but nodes_len stays the same. Since
        deleting a node is simply setting a
        flag, the valid entries in nodes are
        not necessarily consecutive. Therefore
        iterations over nodes require nodes_len
    */
    int nodes_len; /* size of nodes array */
   /* We use an adjacency matrix to keep track
        of the edges. If an edge between u and v
        exists, this specific bit is set to 1
     */
    char** adjacency_matrix;
};

char node_invalid(Graph g, int node) {
    if (node < 0 || 
        node >= g->nodes_len ||
        g->nodes[node]->is_deleted) return 1;
    return 0;
}

/* create a new graph with n vertices labeled 0..n-1 and no edges */
Graph graph_create(int n)
{
    Graph g = malloc(sizeof(struct graph));
    assert(g);
    g->n = n;
    g->m = 0;
    g->nodes = malloc(sizeof(struct node_t *) * n);
    g->nodes_len = n;
    g->adjacency_matrix = malloc(sizeof(char *) *n);
    assert(g->nodes);
    /* calculate the size of the adjacency matrix.
        We need one more byte if the number of 
        vertices is not a multiple of 8.
    */
    int size = n/8;
    if (n % 8 != 0) size++;

    for (int i = 0; i < n; i++)
    {
        g->adjacency_matrix[i] = malloc(size);
        memset(g->adjacency_matrix[i], 0, size);
        g->nodes[i] = malloc(sizeof(struct node_t));
        assert(g->nodes[i]);
        g->nodes[i]->d = 0;
        g->nodes[i]->is_deleted = 0;
        g->nodes[i]->in_set = 0;
        g->nodes[i]->neighbours = malloc(sizeof(int));
        g->nodes[i]->len = 1;
    }

    return g;
}

/* import a graph from a file in adjacency list format
    The file must start with <# nodes n> where n is the
    number of vertices in the graph */
Graph graph_import(char *inputpath)
{

    /* get the size of the graph */
    int n;
    char *line = NULL;
    size_t linelen = 0;

    // open the file
    FILE *fptr;
    fptr = fopen(inputpath, "r");
    if (fptr == NULL)
    {
        fprintf(stderr, "Not able to open file %s\n", inputpath);
        return NULL;
    }
    if (getline(&line, &linelen, fptr) < 0)
    {
        // no point in continuing if we could not read
        free(line);
        return NULL;
    }
    char *str = line;
    char *tok = strtok(str, " ");
    if (tok == NULL)
        return NULL;
    if (strcmp(tok, "#") != 0)
        return NULL;
    tok = strtok(NULL, " ");
    if (tok == NULL)
        return NULL;
    if (strcmp(tok, "nodes") != 0)
        return NULL;
    tok = strtok(NULL, " ");
    if (tok == NULL)
        return NULL;
    // tok should now be the number of vertices as string
    if (sscanf(tok, "%d", &n) != 1)
    {
        fprintf(stderr, "Conversion error\n");
        return NULL;
    }
    //fprintf(stdout, "Number of vertices: %d\n", n);

    Graph g = graph_create(n);
    assert(g);

    // populate the adjacency lists of each node
    for (int i = 0; i < g->n; i++)
    {
        // get the next line from file
        if (getline(&line, &linelen, fptr) < 0)
        {
            // unexpected EOF etc.
            fprintf(stderr, "Error parsing the adjacency list for node %d\n", i);
            graph_destroy(g);
            free(line);
            if (fptr != NULL) fclose(fptr);
            return NULL;
        }
        // split line into tokens and convert to int
        int neighbour_count = 0;
        int node = 0;
        char* str = line;
        tok = strtok(str, " ");
        // the first entry in a line is the node itself
        if (tok != NULL)
        {
            if (sscanf(tok, "%d", &node) != 1)
            {
                fprintf(stderr, "Conversion error\n");
                graph_destroy(g);
                if (fptr != NULL) fclose(fptr);
                return NULL;
            }
            assert(node == i);
            tok = strtok(NULL, " ");
        }
        while (tok != NULL)
        {
            assert(neighbour_count < n);
            int neighbour;
            if (sscanf(tok, "%d", &neighbour) != 1)
            {
                fprintf(stderr, "Conversion error\n");
                graph_destroy(g);
                if (fptr != NULL) fclose(fptr);
                return NULL;
            }
            neighbour_count++;
            graph_add_edge(g, node, neighbour);
            
            tok = strtok(NULL, " ");
        }
    }
    if (fptr != NULL) fclose(fptr);
    free(line);
    return g;
}

/* copy a graph*/
Graph graph_copy(Graph g){
    assert(g);
    assert(g->nodes);

    Graph copy = malloc(sizeof(struct graph));
    assert(copy);
    copy->n = g->n;
    copy->m = g->m;
    copy->nodes = malloc(sizeof(struct node_t *) * g->n);
    int size = g->n * g->n/8;
    if (g->n % 8 != 0) size++;
    copy->adjacency_matrix = malloc(size);
    memcpy(copy->adjacency_matrix, g->adjacency_matrix, size);
    copy->nodes_len = g->nodes_len;
    assert(copy->nodes);
    for (int i = 0; i < copy->nodes_len; i++)
    {
        copy->nodes[i] = malloc(sizeof(struct node_t));
        assert(copy->nodes[i]);
        copy->nodes[i]->d = g->nodes[i]->d;
        copy->nodes[i]->is_deleted = g->nodes[i]->is_deleted;
        copy->nodes[i]->in_set = g->nodes[i]->in_set;
        copy->nodes[i]->len = copy->nodes[i]->d;
        copy->nodes[i]->neighbours = (int*)malloc(sizeof(int)*copy->nodes[i]->len);
        assert(copy->nodes[i]->neighbours);
        for (int j = 0; j < copy->nodes[i]->d; j++)
        {
            copy->nodes[i]->neighbours[j] = g->nodes[i]->neighbours[j];
        }
        
    }
    return copy;
}

/* free all space used by graph */
void graph_destroy(Graph g)
{
    assert(g);
    int i;

    for (i = 0; i < g->nodes_len; i++)
    {
        free(g->nodes[i]->neighbours);
        free(g->nodes[i]);
        //free(g->adjacency_matrix[i]);
    }
    free(g->adjacency_matrix);
    free(g->nodes);
    free(g);
}

/* add an undirected edge to an existing graph */
void graph_add_edge(Graph g, int u, int v)
{
    if(node_invalid(g, u)) return;
    if(node_invalid(g, v)) return;

    if (graph_has_edge(g, u, v)) return;
     
    g->adjacency_matrix[u][v/8] |= 1<<(7-v%8);
    g->adjacency_matrix[v][u/8] |= 1<<(7-u%8);
    struct node_t* source = g->nodes[u];
    /* do we need to grow the list? */
    while (source->d >= source->len)
    {
        source->len *= 2;
        source->neighbours =
            realloc(source->neighbours,
                    sizeof(int) * (source->len));
    }

    /* now add the new sink */
    source->neighbours[source->d++] = v;
    struct node_t* sink = g->nodes[v];
    /* do we need to grow the list? */
    while (sink->d >= sink->len)
    {
        sink->len *= 2;
        sink->neighbours =
            realloc(sink->neighbours,
                    sizeof(int) * (sink->len));
    }

    /* now add the new source */
    sink->neighbours[sink->d++] = u;
    /* bump edge count */
    g->m++;
}

/* eliminate a vertex from the graph
   and return its degree upon elimination
*/
int graph_eliminate_vertex(Graph g, int vertex) {
    if (node_invalid(g, vertex)) return -1; //TODO: what to return?

    struct node_t* current = g->nodes[vertex];
    int degree = current->d;
    /* for each neighbour of vertex but the last one
       (since there are no other neighbours to connect
       to) */
    for (int i = 0; i < (current->d - 1); i++)
    {
        /* add an edge to all other neighbours
        if the edge is already present, the call
        returns immediately, thus no need to check
        */
        for (int j = i+1; j < current->d; j++)
        {
            graph_add_edge(g, current->neighbours[i],
             current->neighbours[j]);
        }        
    }
    graph_delete_vertex(g, vertex);
    return degree;
}

/* mark this vertex as included in a
    mcs set
*/
void graph_include_vertex(Graph g, int vertex) {
    if (node_invalid(g, vertex)) return;
    g->nodes[vertex]->in_set = 1;
}

/* delete node vertex from the graph.
    Note: this does not free its memory.
*/
void graph_delete_vertex(Graph g, int vertex) {
    if (node_invalid(g, vertex)) return;
    struct node_t* current = g->nodes[vertex];
    int decrease = 0;
    /* decrease the degree of all neighbours */
    for (int i = 0; i < current->d ; i++)
    {
        if (node_invalid(g, current->neighbours[i])) continue;
        g->nodes[current->neighbours[i]]->d--;
        decrease++;
    }
    current->is_deleted = 1;
    g->n--;
    g->m -= decrease;
}

/* return the number of vertices in the graph */
int graph_vertex_count(Graph g)
{
    return g->n;
}

/* return the number of vertices in the graph */
int graph_edge_count(Graph g)
{
    return g->m;
}

/* return the out-degree of a vertex
    if the supplied node is deleted,
    returns max int value
*/
int graph_vertex_degree(Graph g, int source)
{
    if (node_invalid(g, source)) return __INT_MAX__;
    return g->nodes[source]->d;
}

/* return the amount of fill-in edges to be added
    if this vertex was eliminated. 
    Returns max int value for deleted vertex.
    Note: This function only works with undirected graphs.
*/
int graph_vertex_fillin(Graph g, int vertex) {
    if (node_invalid(g, vertex)) return __INT_MAX__;
    int result = 0;
    struct node_t* current = g->nodes[vertex];
    /* for each neighbour of vertex but the last one
       (since there are no other neighbours to check
       against) */
    for (int i = 0; i < (current->d - 1); i++)
    {
        // check if all other neighbour are connected to it
        for (int j = i+1; j < current->d; j++)
        {
            if (!graph_has_edge(g, g->nodes[i]->neighbours[i], g->nodes[i]->neighbours[j])) result++;
        }        
    }
    return result;
}

/* return number of neighbours of vertex that
    are in the provided set 
*/
int graph_vertex_cardinality(Graph g, int vertex, int* set, int set_len) {
    if (node_invalid(g, vertex) || g->nodes[vertex]->in_set) return -1;
    int result = 0;
    for (int i = 0; i < set_len; i++)
    {
        if (graph_has_edge(g, vertex, set[i])) result++;
    }
    return result;
}

/* return 1 if edge (source, sink) exists), 0 otherwise */
int graph_has_edge(Graph g, int source, int sink)
{
    if (node_invalid(g, source) ||
        node_invalid(g, sink)) return 0;

    return g->adjacency_matrix[source][sink/8] & 1<<(7-sink%8);
}

/* return index of vertex with minimal value for
    function f*/
int graph_min_vertex(Graph g,
                     int (*f)(Graph g, int vertex)) {
    int min_value = __INT_MAX__;
    int min_index = 0;
    for (int i = 0; i < g->nodes_len; i++)
    {
        int current = f(g, i);
        if (current < min_value)
        {
            min_index = i;
            min_value = current;
        }        
    }
    return min_index;
}

/* return the index of the vertex with minimal degree
*/
int graph_min_degree_index(Graph g) {
    return graph_min_vertex(g, graph_vertex_degree);
}

/* return the index of vertex that results in
    minimal fill-in edges upon elimination */
int graph_min_fillin_index(Graph g) {
    return graph_min_vertex(g, graph_vertex_fillin);
}

/* return index of the vertex that has 
    most neighbours in a provided set
*/
int graph_max_cardinality_index(Graph g, int* set, int set_len) {
    int max_value = -1;
    int max_index = 0;
    for (int i = g->nodes_len-1 ; i >= 0; i--)
    {
        int current = graph_vertex_cardinality(g, i, set, set_len);
        if (current > max_value) {
            max_index = i;
            max_value = current;
        }
    }
    return max_index;
}

/* Print the graph in adjacency list format.
    A "d" for the adjacency list of a vertex
    signals that this vertex is deleted
*/
void graph_print(Graph g, FILE *stream){
    if (g == NULL) return;
    if (stream == NULL) return;

    fprintf(stream, "# nodes %d\n", g->n);
    for (int i = 0; i < g->nodes_len; i++)
    {
        fprintf(stream, "%d", i);
        if (g->nodes[i]->is_deleted) fprintf(stream, " d");
        for (int j = 0; j < g->nodes[i]->d; j++)
        {
            fprintf(stream, " %d", g->nodes[i]->neighbours[j]);
        }
        fprintf(stream, "\n");
    }    
}
/* Based on https://www.cs.yale.edu/homes/aspnes/pinewiki/C(2f)Graphs.html
 */
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "graph.h"

/* basic directed graph type */
/* the implementation uses adjacency lists
 * represented as variable-length arrays */

/* these arrays may or may not be sorted: if one gets long enough
 * and you call graph_has_edge on its source, it will be */

#define SMALL_BUFFER 64
/* when we are willing to call bsearch */
#define BSEARCH_THRESHOLD (10)

static int
intcmp(const void *a, const void *b)
{
    return *((const int *)a) - *((const int *)b);
}


struct node_t
    {
        int d;          /* number of successors */
        int len;        /* number of slots in array */
        char is_sorted; /* true if  neighbour array is already sorted */
        char is_deleted; /* deletion flag */
        char in_set;    /* true if this node is included in a mcs set*/
        int *neighbours;     /* actual arry of successors */
    };

struct graph
{
    int n; /* number of vertices */
    int m; /* number of edges */
    struct node_t **nodes;   
    int nodes_len; /* size of nodes array */
    /* if a vertex is deleted, n decreases
        but nodes_len stays the same. Since
        deleting a node is simply setting a
        flag, the valid entries in nodes are
        not necessarily consecutive. Therefore
        iterations over nodes require nodes_len
    */
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
    assert(g->nodes);

    for (int i = 0; i < n; i++)
    {
        g->nodes[i] = malloc(sizeof(struct node_t));
        assert(g->nodes[i]);

        g->nodes[i]->d = 0;
        g->nodes[i]->len = 1;
        g->nodes[i]->is_sorted = 1;
        g->nodes[i]->is_deleted = 0;
        g->nodes[i]->in_set = 0;
        g->nodes[i]->neighbours = (int *)malloc(sizeof(int));
        assert(g->nodes[i]->neighbours);
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
    char buffer[SMALL_BUFFER];
    char *str = buffer;

    // open the file
    FILE *fptr;
    fptr = fopen(inputpath, "r");
    if (fptr == NULL)
    {
        fprintf(stderr, "Not able to open file %s\n", inputpath);
        return NULL;
    }
    if (fgets(str, SMALL_BUFFER, fptr) == NULL)
    {
        // no point in continuing if we could not read
        return NULL;
    }
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

    Graph g = malloc(sizeof(struct graph));
    assert(g);
    g->n = n;
    g->m = 0;
    g->nodes = malloc(sizeof(struct node_t *) * n);
    g->nodes_len = n;
    assert(g->nodes);

    // populate the adjacency lists of each node
    for (int i = 0; i < g->n; i++)
    {
        g->nodes[i] = malloc(sizeof(struct node_t));
        assert(g->nodes[i]);
        g->nodes[i]->is_sorted = 1;
        g->nodes[i]->is_deleted = 0;
        g->nodes[i]->in_set = 0;
        // reserve space for a fully connected node
        int *work = (int *)malloc(sizeof(int) * g->n);
        assert(work);

        // get the next line from file
        if (fgets(str, SMALL_BUFFER, fptr) == NULL)
        {
            // unexpected EOF etc.
            fprintf(stderr, "Error parsing the adjacency list for node %d\n", i);
            graph_destroy(g);
            if (fptr != NULL) fclose(fptr);
            return NULL;
        }
        // split line into tokens and convert to int
        int neighbour_count = 0;
        int node = 0;
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
            if (sscanf(tok, "%d", &work[neighbour_count]) != 1)
            {
                fprintf(stderr, "Conversion error\n");
                graph_destroy(g);
                if (fptr != NULL) fclose(fptr);
                return NULL;
            }
            neighbour_count++;
            if (neighbour_count>0 && 
                work[neighbour_count] < work[neighbour_count-1])
            {
                g->nodes[i]->is_sorted = 0;
            }
            
            tok = strtok(NULL, " ");
        }
        g->nodes[i]->d = neighbour_count;
        g->m += neighbour_count;
        g->nodes[i]->len = neighbour_count + 1;
        work = (int *)realloc(work, (neighbour_count + 1) * sizeof(int));
        g->nodes[i]->neighbours = work;
        assert(g->nodes[i]->neighbours);
    }
    if (fptr != NULL) fclose(fptr);

    /* By reading the adjacency list file, 
    we added the edge only in one node so far,
    now add the edges to the other nodes as well*/
    Graph half = graph_copy(g);
    for (int i = 0; i < g->n; i++) {
        for (int j = 0; j < half->nodes[i]->d; j++)
        {
            graph_add_directed_edge(g, half->nodes[i]->neighbours[j], i);
        }
        
    }
    graph_sort(g);
    graph_destroy(half);
    return g;
}

/* copy a graph*/
Graph graph_copy(Graph g){
    /* TODO: implementing this with memcpy would be an
        alternative to look into 
    */
    Graph copy = malloc(sizeof(struct graph));
    assert(copy);
    copy->n = g->n;
    copy->m = g->m;
    copy->nodes = malloc(sizeof(struct node_t *) * copy->n);
    copy->nodes_len = g->nodes_len;
    assert(copy->nodes);
    for (int i = 0; i < copy->nodes_len; i++)
    {
        copy->nodes[i] = malloc(sizeof(struct node_t));
        assert(copy->nodes[i]);
        copy->nodes[i]->d = g->nodes[i]->d;
        copy->nodes[i]->is_sorted = g->nodes[i]->is_sorted;
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
    }
    free(g);
}

/* add a directed edge to an existing graph */
void graph_add_directed_edge(Graph g, int u, int v)
{
    if(node_invalid(g, u)) return;
    if(node_invalid(g, v)) return;

    if (graph_has_edge(g, u, v)) return;
     
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
    source->is_sorted = 0;

    /* bump edge count */
    g->m++;
}

/* add an undirected edge to an existing graph */
void graph_add_edge(Graph g, int vertex1, int vertex2) {
    graph_add_directed_edge(g, vertex1, vertex2);
    graph_add_directed_edge(g, vertex2, vertex1);
    //correct edge count because it was counted twice
    g->m--;
}

/* delete a directed edge from a graph */
void graph_delete_directed_edge(Graph g, int source, int sink) {
    if (node_invalid(g, source)) return;
    if (node_invalid(g, sink)) return;

    if (!graph_has_edge(g, source, sink)) return;
    
    // find index of sink in source neighbourhood
    int index;
    if (graph_vertex_degree(g, source) >= BSEARCH_THRESHOLD)
    {
        /* make sure it is sorted */
        if (!g->nodes[source]->is_sorted)
        {
            qsort(g->nodes[source]->neighbours,
                  g->nodes[source]->d,
                  sizeof(int),
                  intcmp);
            g->nodes[source]->is_sorted = 1;
        }

        /* call bsearch to do binary search for us */
        int *ptr = (int*)bsearch(&sink,
                       g->nodes[source]->neighbours,
                       g->nodes[source]->d,
                       sizeof(int),
                       intcmp);
        index = (ptr - g->nodes[source]->neighbours)/sizeof(int);
    }
    else
    {
        /* just do a simple linear search */
        /* we could call lfind for this, but why bother? */
        for (int i = 0; i < g->nodes[source]->d; i++)
        {
            if (g->nodes[source]->neighbours[i] == sink) {
                index = i;
                break;
            }
        }
    }
    
    /* delete sink by shifting following neighbours
       to the left (overwriting it) and adjusting degree
    */
   int remaining = g->nodes[source]->d - 1 - index;
   g->nodes[source]->d--;
   g->m--;
   // if sink was the last entry, nothing has to be moved
   if (!remaining) return;
   memcpy(&g->nodes[source]->neighbours[index],
             &g->nodes[source]->neighbours[index + 1],
             remaining * sizeof(int));
    
}

/* delete an undirected edge from a graph */
void graph_delete_edge(Graph g, int vertex1, int vertex2) {
    graph_delete_directed_edge(g, vertex1, vertex2);
    graph_delete_directed_edge(g, vertex2, vertex1);
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
    /* don't bother removing the edges of vertex as it is deleted
        anyway, but remove vertex from its neighbours */
    for (int i = 0; i < current->d ; i++)
    {
        graph_delete_directed_edge(g, current->neighbours[i], vertex);
    }
    current->is_deleted = 1;
    current->d = 0;
    g->n--;
    // deleting the edges already corrected g->n
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


/* sort the neighbours array of each vertex */
void graph_sort(Graph g) {
    for (int i = 0; i < g->nodes_len; i++)
    {
        // skip deleted nodes
        if (g->nodes[i]->is_deleted) continue;
        if (!g->nodes[i]->is_sorted)
        {
            qsort(g->nodes[i]->neighbours,
                  g->nodes[i]->d,
                  sizeof(int),
                  intcmp);
            g->nodes[i]->is_sorted = 1;
        }
    }
    
}

/* return 1 if edge (source, sink) exists), 0 otherwise */
int graph_has_edge(Graph g, int source, int sink)
{
    int i;

    if (node_invalid(g, source) ||
        node_invalid(g, sink)) return 0;

    if (graph_vertex_degree(g, source) >= BSEARCH_THRESHOLD)
    {
        /* make sure it is sorted */
        if (!g->nodes[source]->is_sorted)
        {
            qsort(g->nodes[source]->neighbours,
                  g->nodes[source]->d,
                  sizeof(int),
                  intcmp);
        }

        /* call bsearch to do binary search for us */
        return bsearch(&sink,
                       g->nodes[source]->neighbours,
                       g->nodes[source]->d,
                       sizeof(int),
                       intcmp) != 0;
    }
    else
    {
        /* just do a simple linear search */
        /* we could call lfind for this, but why bother? */
        for (i = 0; i < g->nodes[source]->d; i++)
        {
            if (g->nodes[source]->neighbours[i] == sink)
                return 1;
        }
        /* else */
        return 0;
    }
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
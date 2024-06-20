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

struct successors
    {
        int d;          /* number of successors */
        int len;        /* number of slots in array */
        char is_sorted; /* true if array is already sorted */
        int *neighbours;     /* actual arry of successors */
    };

struct graph
{
    int n; /* number of vertices */
    int m; /* number of edges */
    struct successors **nodes;
};

/* create a new graph with n vertices labeled 0..n-1 and no edges */
Graph graph_create(int n)
{
    Graph g = malloc(sizeof(struct graph));
    assert(g);
    g->n = n;
    g->m = 0;
    g->nodes = malloc(sizeof(struct successors *) * n);
    assert(g->nodes);

    for (int i = 0; i < n; i++)
    {
        g->nodes[i] = malloc(sizeof(struct successors));
        assert(g->nodes[i]);

        g->nodes[i]->d = 0;
        g->nodes[i]->len = 1;
        g->nodes[i]->is_sorted = 1;
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
    g->nodes = malloc(sizeof(struct successors *) * n);
    assert(g->nodes);

    // populate the adjacency lists of each node
    for (int i = 0; i < g->n; i++)
    {
        g->nodes[i] = malloc(sizeof(struct successors));
        assert(g->nodes[i]);
        g->nodes[i]->is_sorted = 1;
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
    Graph copy = malloc(sizeof(struct graph));
    assert(copy);
    copy->n = g->n;
    copy->m = g->m;
    copy->nodes = malloc(sizeof(struct successors *) * copy->n);
    assert(copy->nodes);
    for (int i = 0; i < copy->n; i++)
    {
        copy->nodes[i] = malloc(sizeof(struct successors));
        assert(copy->nodes[i]);
        copy->nodes[i]->d = g->nodes[i]->d;
        copy->nodes[i]->is_sorted = g->nodes[i]->is_sorted;
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

    for (i = 0; i < g->n; i++)
    {
        free(g->nodes[i]->neighbours);
        free(g->nodes[i]);
    }
    free(g);
}

/* add a directed edge to an existing graph */
void graph_add_directed_edge(Graph g, int u, int v)
{
    assert(u >= 0);
    assert(u < g->n);
    assert(v >= 0);
    assert(v < g->n);

    if (graph_has_edge(g, u, v)) return;
    
    struct successors* source = g->nodes[u];
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

/* return the out-degree of a vertex */
int graph_out_degree(Graph g, int source)
{
    assert(source >= 0);
    assert(source < g->n);

    return g->nodes[source]->d;
}

/* when we are willing to call bsearch */
#define BSEARCH_THRESHOLD (10)

static int
intcmp(const void *a, const void *b)
{
    return *((const int *)a) - *((const int *)b);
}

/* sort the neighbours array of each vertex */
void graph_sort(Graph g) {
    for (int i = 0; i < g->n; i++)
    {
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

    assert(source >= 0);
    assert(source < g->n);
    assert(sink >= 0);
    assert(sink < g->n);

    if (graph_out_degree(g, source) >= BSEARCH_THRESHOLD)
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

/* invoke f on all edges (u,v) with source u */
/* supplying data as final parameter to f */
void graph_foreach(Graph g, int source,
                   void (*f)(Graph g, int source, int sink, void *data),
                   void *data)
{
    int i;

    assert(source >= 0);
    assert(source < g->n);

    for (i = 0; i < g->nodes[source]->d; i++)
    {
        f(g, source, g->nodes[source]->neighbours[i], data);
    }
}

/* Print the graph in adjacency list format*/
void graph_print(Graph g, FILE *stream){
    if (g == NULL) return;
    if (stream == NULL) return;

    fprintf(stream, "# nodes %d\n", g->n);
    for (int i = 0; i < g->n; i++)
    {
        fprintf(stream, "%d", i);
        for (int j = 0; j < g->nodes[i]->d; j++)
        {
            fprintf(stream, " %d", g->nodes[i]->neighbours[j]);
        }
        fprintf(stream, "\n");
    }    
}
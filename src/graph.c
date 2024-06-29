/* Based on https://www.cs.yale.edu/homes/aspnes/pinewiki/C(2f)Graphs.html
 */
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "graph.h"

struct node_t
    {
        int id;         /* id of the node */
        int d;          /* number of successors */
        char is_deleted; /* deletion flag */
        char in_set; /* true if the node is already in ordering */
        int* neighbours; /* list of neighbours for iterating purposes*/
        int len; /* size of neigbour_list */
        int MCS_index; /* this node is contained in <g->MCS->heads[MCS_index]>*/
        struct node_t *next; /* Next member of the MCS linked list */
        struct node_t *prev; /* Previous member of MCS linked list */
        
    };

/* For maximum cardinality search we keep
    an array of sets MCS_t where MCS_t->heads[j] is
    a list of pointers to nodes which
    have exactly j neighbours in the 
    current <g->ordering>. Look at it
    as an array of linked lists
    of pointers to nodes in <g->nodes> */
struct MCS_t {
    struct node_t **heads;
    struct node_t **tails;
    /* max_S is the biggest index i where
        <MCS_t->heads[i]> is not empty */
    int max_S;
    /* len is the size of the heads and tails
        arrays */
    int len;
};

struct graph
{
    int n; /* number of vertices */
    int m; /* number of edges */
    struct node_t **nodes; /* holds meta information of each vertex*/

    /* if a vertex is deleted, <n> decreases
        but <nodes_len> stays the same. Since
        deleting a node is simply setting a
        flag, the valid entries in nodes are
        not necessarily consecutive. Therefore
        iterations over nodes require <nodes_len>
        and checking for the deletion flag.
    */
    int nodes_len; /* size of nodes array */

   /* We use an adjacency matrix to keep track
        of the edges. Note: This is a bit field.
        A graph has n "rows" of n bits, so approx.
        n/8 chars per row.
        If an edge between u and v exists, 
        the v'th bit in the u'th row is set.
     */
    char** adjacency_matrix;

    int* ordering; /* holds the ordering produced by an elimination ordering*/

    struct MCS_t *MCS; /* structure for maximum cardinality search */
};

/* check if a node exists and is not deleted */
char node_invalid(Graph g, int node) {
    if (node < 0 || 
        node >= g->nodes_len ||
        g->nodes[node]->is_deleted) return 1;
    return 0;
}

/* add node g->nodes[node_index] to the MCS
    lists with set number index
*/
void MCS_add_node(Graph g, int node_index, int index) {
    if (node_invalid(g, node_index)) return;
    struct node_t *node = g->nodes[node_index];
    if (g->MCS->tails[index])
    {
        g->MCS->tails[index]->next = node;
        node->prev = g->MCS->tails[index];       
    } else
    {
        g->MCS->heads[index] = node;
    }    
    g->MCS->tails[index] = node;
    if (index > g->MCS->max_S)
        g->MCS->max_S = index;
}

/* remove node g->nodes[node_index] from the 
    MCS lists
*/
void MCS_delete_node(Graph g, int node_index) {
    if (node_invalid(g, node_index)) return;
    struct node_t *node = g->nodes[node_index];
    if (node->prev)
        node->prev->next = node->next;

    if (node->next)
        node->next->prev = node->prev;

    if (node == g->MCS->heads[node->MCS_index]) {
        g->MCS->heads[node->MCS_index] = node->next;
        /* if this was the last node in the list
            we need to decrease max_S
        */
        if (!node->next)
        {
            while (!g->MCS->heads[(g->MCS->max_S)]) g->MCS->max_S--;
        }
    }

    if (node == g->MCS->tails[node->MCS_index]) {
        g->MCS->tails[node->MCS_index] = node->prev;
        
        // max_S should have been corrected already
        assert(g->MCS->tails[g->MCS->max_S]);
        
    }
}

Graph graph_create(int n)
{
    Graph g = malloc(sizeof(struct graph));
    assert(g);
    g->n = n;
    g->m = 0;
    g->nodes = malloc(sizeof(struct node_t *) * n);
    assert(g->nodes);
    g->nodes_len = n;
    g->adjacency_matrix = malloc(sizeof(char *) *n);
    g->ordering = malloc(sizeof(int)*n);
    assert(g->ordering);
    /* At most a node can be connected to all
        other nodes which could be in the ordering
        potentially, therefore n-1 is the maximum for MCS */
    g->MCS = malloc(sizeof(struct MCS_t));
    assert(g->MCS);
    g->MCS->len = n-1;
    g->MCS->max_S = 0;
    g->MCS->heads = malloc(sizeof(struct node_t*)*g->MCS->len);
    assert(g->MCS->heads);
    g->MCS->tails = malloc(sizeof(struct node_t*)*g->MCS->len);
    assert(g->MCS->tails);
    
    
    
    /* calculate the size of the adjacency matrix.
        We need one more byte if the number of 
        vertices is not a multiple of 8.
    */
    int size = n/8;
    if (n % 8 != 0) size++;

    for (int i = 0; i < n; i++)
    {
        g->adjacency_matrix[i] = malloc(size);
        assert(g->adjacency_matrix[i]);
        memset(g->adjacency_matrix[i], 0, size);
        
        g->nodes[i] = malloc(sizeof(struct node_t));
        assert(g->nodes[i]);
        g->nodes[i]->id = i;
        g->nodes[i]->d = 0;
        g->nodes[i]->is_deleted = 0;
        g->nodes[i]->in_set = 0;
        g->nodes[i]->neighbours = malloc(sizeof(int));
        assert(g->nodes[i]->neighbours);
        g->nodes[i]->len = 1;
        g->nodes[i]->MCS_index = 0;
        g->nodes[i]->next = NULL;
        g->nodes[i]->prev = NULL;
        
        g->ordering[i] = -1;        
    }

    for (int i = 0; i < g->MCS->len; i++)
    {
        g->MCS->heads[i] = NULL;
        g->MCS->tails[i] = NULL;
    }
    
    /* There is no ordering yet, so every
        node is in the 0 set 
    */
    for (int i = 0; i < g->n; i++)
    {
        MCS_add_node(g, i, 0);
    }
    

    return g;
}

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

Graph graph_copy(Graph g){
    assert(g);
    assert(g->nodes);

    Graph copy = malloc(sizeof(struct graph));
    assert(copy);
    int n = g->nodes_len;
    copy->n = g->n;
    copy->m = g->m;
    copy->nodes = malloc(sizeof(struct node_t *) * n);
    assert(copy->nodes);
    copy->nodes_len = g->nodes_len;
    copy->adjacency_matrix = malloc(sizeof(char *) * g->n);
    copy->ordering = malloc(sizeof(int)*n);
    assert(copy->ordering);
    
    copy->MCS = malloc(sizeof(struct MCS_t));
    assert(copy->MCS);
    copy->MCS->len = g->MCS->len;
    copy->MCS->max_S = g->MCS->max_S;
    copy->MCS->heads = malloc(sizeof(struct node_t*)*g->MCS->len);
    assert(copy->MCS->heads);
    copy->MCS->tails = malloc(sizeof(struct node_t*)*g->MCS->len);
    assert(copy->MCS->tails);
    
    /* calculate the size of the adjacency matrix.
        We need one more byte if the number of 
        vertices is not a multiple of 8.
    */
    int size = n * n/8;
    if (n % 8 != 0) size++;
    
    for (int i = 0; i < n; i++)
    {
        copy->adjacency_matrix[i] = malloc(size);
        assert(copy->adjacency_matrix[i]);
        memcpy(copy->adjacency_matrix[i], g->adjacency_matrix[i], size);
        
        copy->nodes[i] = malloc(sizeof(struct node_t));
        assert(copy->nodes[i]);
        copy->nodes[i]->id = g->nodes[i]->id;
        copy->nodes[i]->d = g->nodes[i]->d;
        copy->nodes[i]->is_deleted = g->nodes[i]->is_deleted;
        copy->nodes[i]->in_set = g->nodes[i]->in_set;
        copy->nodes[i]->len = g->nodes[i]->len;
        copy->nodes[i]->neighbours = (int*)malloc(sizeof(int)*copy->nodes[i]->len);
        assert(copy->nodes[i]->neighbours);
        copy->nodes[i]->neighbours = memcpy(copy->nodes[i]->neighbours,
                                        g->nodes[i]->neighbours,
                                        sizeof(int)*g->nodes[i]->len);
        copy->nodes[i]->MCS_index = g->nodes[i]->MCS_index;
        copy->nodes[i]->next = g->nodes[i]->next;
        copy->nodes[i]->prev = g->nodes[i]->prev;

        copy->ordering[i] = g->ordering[i];
        
    }

    for (int i = 0; i < g->MCS->len; i++)
    {
        copy->MCS->heads[i] = g->MCS->heads[i];
        g->MCS->tails[i] = g->MCS->tails[i];
    }
    
    return copy;
}

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
    free(g->ordering);
    free(g->MCS->heads);
    free(g->MCS->tails);
    free(g->MCS);
    free(g);
}

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
        /* add an edge to all other neighbours.
        If the edge is already present, the call
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

/* Use this function instead of the elimination function
    when doing MCS strategy
*/
void graph_include_vertex(Graph g, int vertex) {
    /* If a vertex is deleted, all its neighbours
        move up in the MCS lists by one index 
    */
    for (int i = 0; i < g->nodes[vertex]->len; i++)
    {
        int neighbour = g->nodes[vertex]->neighbours[i];
        if (node_invalid(g, neighbour)) continue;
        int current = g->nodes[neighbour]->MCS_index;
        MCS_delete_node(g, neighbour);
        MCS_add_node(g, neighbour, current+1);
    }    
}

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

int graph_vertex_count(Graph g)
{
    return g->n;
}

int graph_edge_count(Graph g)
{
    return g->m;
}

int graph_vertex_degree(Graph g, int source)
{
    if (node_invalid(g, source)) return INT_MAX;
    return g->nodes[source]->d;
}

int graph_vertex_fillin(Graph g, int vertex) {
    if (node_invalid(g, vertex)) return INT_MAX;
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

int graph_vertex_cardinality(Graph g, int vertex) {
    if (node_invalid(g, vertex) || g->nodes[vertex]->in_set) return -1;
    return g->nodes[vertex]->MCS_index;
}

int graph_has_edge(Graph g, int source, int sink)
{
    if (node_invalid(g, source) ||
        node_invalid(g, sink)) return 0;

    return g->adjacency_matrix[source][sink/8] & 1<<(7-sink%8);
}

int graph_min_vertex(Graph g,
                     int (*f)(Graph g, int vertex)) {
    int min_value = INT_MAX;
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

int graph_min_degree_index(Graph g) {
    return graph_min_vertex(g, graph_vertex_degree);
}

int graph_min_fillin_index(Graph g) {
    return graph_min_vertex(g, graph_vertex_fillin);
}

int graph_max_cardinality_index(Graph g) {
    return g->MCS->heads[g->MCS->max_S]->id;
}

int graph_order_degree (Graph g) {
  int size = graph_vertex_count(g);
  int width = 0;
  for (int i = 0; i < size; i++)
  {
    int best_node = graph_min_degree_index(g);
    int current_width = graph_eliminate_vertex(g, best_node);
    if (current_width > width) width = current_width;
    g->ordering[i] = best_node;
  }
  return width;
}

int graph_order_fillin (Graph g) {
  int size = graph_vertex_count(g);
  int width = 0;
  for (int i = 0; i < size; i++)
  {
    int best_node = graph_min_fillin_index(g);
    int current_width = graph_eliminate_vertex(g, best_node);
    if (current_width > width) width = current_width;
    g->ordering[i] = best_node;
  }
  return width;  
}

int graph_order_mcs (Graph g) {
  int size = graph_vertex_count(g);
  int width = 0;
  for (int i = size-1; i > 0; i--) {
    int best_node = graph_max_cardinality_index(g);
    g->ordering[i] = best_node;
  }
  /* calculate treewidth */
  for (int i = 0; i < size; i++)
  {
    int current_width = graph_eliminate_vertex(g, g->ordering[i]);
    if (current_width > width) width = current_width;
  }  
  return width;
}

char graph_ordering_plausible (Graph g) {
    char* used = (char*)malloc(g->nodes_len);
    memset(used, 0, g->nodes_len);
    for (int i = 0; i < g->nodes_len; i++)
    {
        int index = g->ordering[i];
        if (index == -1) return 0;
        if (used[index]) return 0;
        used[index] = 1;
    }
    return 1;
}

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
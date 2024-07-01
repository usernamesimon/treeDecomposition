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
        struct node_t **neighbours; /* array of neighbour pointers for iterating purposes*/
        int len; /* size of neigbour array */
        int priority_index; /* this node is contained in <g->priority->heads[priority_index]>*/
        struct node_t *next; /* Next member of the priority linked list */
        struct node_t *prev; /* Previous member of priority linked list */
        
    };

/*  To greatly improve performance, we keep track
    of which nodes are to be selected next by updating
    this structure, instead of calculating the best
    node in each step.

    
    For the vertex degree method, a node with
    degree j is in the list heads[j].

    For the min-fill-in method, a node with j
    fill-in edges produced is in the list heads[j].

    For maximum cardinality search a nodes
    which has exactly j neighbours in the 
    list heads[j].
    */
struct Priority_t {
    struct node_t **heads;
    struct node_t **tails;
    /* max_ptr is the biggest index i where
        <Priority_t->heads[i]> is not empty */
    int max_ptr;
    /* min_ptr is the smallest index i where
        <Priority_t->heads[i]> is not empty */
    int min_ptr;
    /* len is the size of the heads and tails
        arrays */
    int len;
};

enum strategy {unspecified, degree, fillin, mcs};
struct graph
{
    int n; /* number of vertices */
    int m; /* number of edges */
    struct node_t **nodes; /* actual array of vertex pointers */

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

    struct Priority_t *priority; /* structure for determining the next node
                                    to eliminate */
    enum strategy_t *strategy;
};

/* check if a node exists and is not deleted */
char node_invalid(Graph g, int node) {
    if (node < 0 || 
        node >= g->nodes_len ||
        g->nodes[node]->is_deleted) return 1;
    return 0;
}

/* add node g->nodes[node_index] to the priority
    lists with set number index
*/
void priority_add_node(Graph g, int node_index, int index) {
    if (node_invalid(g, node_index)) return;
    /*  Grow array if necessary
        This can happen if a node leads to a lot of 
        fill-in edges when using that heuristic.
    */
    while (index > g->priority->len)
    {
        g->priority->len *= 2;
        g->priority->heads = (struct node_t **)realloc(g->priority->heads,
                                sizeof(struct node_t*)* g->priority->len);
        g->priority->tails = (struct node_t **)realloc(g->priority->tails,
                                sizeof(struct node_t*)* g->priority->len);
    }
    
    struct node_t *node = g->nodes[node_index];
    /* if list with index <index> is not empty */
    if (g->priority->tails[index])
    {
        /* link in new node*/
        node->prev = g->priority->tails[index];
        //assert(!g->priority->tails[index]->next);
        g->priority->tails[index]->next = node;
        g->priority->tails[index] = node;
    } else
    {
        /* otherwise make this node the new head */
        g->priority->tails[index] = g->priority->heads[index] = node;
    }
    
    /* check if we have to update min and max pointers */
    if (index > g->priority->max_ptr)
        g->priority->max_ptr = index;
    if (index < g->priority->min_ptr)
        g->priority->min_ptr = index;
    node->priority_index = index;
    assert(node->next!=node);
}

/* remove node g->nodes[node_index] from the 
    priority lists
*/
void priority_delete_node(Graph g, int node_index) {
    if (node_invalid(g, node_index)) return;

    /* unlink the node */
    struct node_t *node = g->nodes[node_index];
    if (node->prev)
        node->prev->next = node->next;

    if (node->next)
        node->next->prev = node->prev;

    /*  if this node was the head, we need to set
        a new one */
    if (node == g->priority->heads[node->priority_index]) {
        g->priority->heads[node->priority_index] = node->next;
        
        /*  if this was the last node in the list
            we may need to decrease max_ptr and
            increase min_ptr
        */
        if (!node->next)
        {
            while (!g->priority->heads[(g->priority->max_ptr)]) g->priority->max_ptr--;
            while (!g->priority->heads[(g->priority->min_ptr)]) g->priority->min_ptr++;
        }
    }

    /* if this was the tail we need to set a new one */
    if (node == g->priority->tails[node->priority_index]) {
        g->priority->tails[node->priority_index] = node->prev;
        
        // max_ptr and min_ptr should have been corrected already
        assert(g->priority->tails[g->priority->max_ptr]);
        assert(g->priority->tails[g->priority->min_ptr]);
        
    }
    node->next = node->prev = NULL;
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
        potentially, therefore n-1 is the maximum for priority */
    g->priority = malloc(sizeof(struct Priority_t));
    assert(g->priority);
    g->priority->len = n-1;
    g->priority->max_ptr = 0;
    g->priority->min_ptr = g->priority->len-1;
    g->priority->heads = malloc(sizeof(struct node_t*)*g->priority->len);
    assert(g->priority->heads);
    g->priority->tails = malloc(sizeof(struct node_t*)*g->priority->len);
    assert(g->priority->tails);

    g->strategy = unspecified;
    
    
    
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
        g->nodes[i]->neighbours = malloc(sizeof(struct node_t*));
        assert(g->nodes[i]->neighbours);
        g->nodes[i]->len = 1;
        g->nodes[i]->priority_index = 0;
        g->nodes[i]->next = NULL;
        g->nodes[i]->prev = NULL;
        
        g->ordering[i] = -1;        
    }

    for (int i = 0; i < g->priority->len; i++)
    {
        g->priority->heads[i] = NULL;
        g->priority->tails[i] = NULL;
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
    
    copy->priority = malloc(sizeof(struct Priority_t));
    assert(copy->priority);
    copy->priority->len = g->priority->len;
    copy->priority->max_ptr = g->priority->max_ptr;
    copy->priority->min_ptr = g->priority->min_ptr;
    copy->priority->heads = malloc(sizeof(struct node_t*)*g->priority->len);
    assert(copy->priority->heads);
    copy->priority->tails = malloc(sizeof(struct node_t*)*g->priority->len);
    assert(copy->priority->tails);

    copy->strategy = g->strategy;
    
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
        copy->nodes[i]->neighbours = (struct node_t**)malloc(sizeof(struct node_t*)*copy->nodes[i]->len);
        assert(copy->nodes[i]->neighbours);
        copy->nodes[i]->neighbours = memcpy(copy->nodes[i]->neighbours,
                                        g->nodes[i]->neighbours,
                                        sizeof(int)*g->nodes[i]->len);
        copy->nodes[i]->priority_index = g->nodes[i]->priority_index;
        
        /* If the original has next/prev link to the node
            with the same id in the copy, otherwise
            to NULL pointer
        */
        if (g->nodes[i]->next) {
            copy->nodes[i]->next = copy->nodes[g->nodes[i]->next->id];
        } else copy->nodes[i]->next = NULL;
        if (g->nodes[i]->prev) {
            copy->nodes[i]->prev = copy->nodes[g->nodes[i]->prev->id];
        } else copy->nodes[i]->prev = NULL;
        copy->ordering[i] = g->ordering[i];
        
    }

    /* Link to new heads and tails if they were present in original*/
    for (int i = 0; i < g->priority->len; i++)
    {
        if (g->priority->heads[i]) {
            copy->priority->heads[i] = copy->nodes[g->priority->heads[i]->id];
        } else copy->priority->heads[i] = NULL;
        if (g->priority->tails[i]) {
            copy->priority->tails[i] = copy->nodes[g->priority->tails[i]->id];
        } else copy->priority->tails[i] = NULL;
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
        free(g->adjacency_matrix[i]);
    }
    free(g->adjacency_matrix);
    free(g->nodes);
    free(g->ordering);
    free(g->priority->heads);
    free(g->priority->tails);
    free(g->priority);
    free(g);
}

/* resize the int array and fill with INT_MIN */
void* realloc_neighbours(void* pBuffer, size_t oldSize, size_t newSize) {
  void* pNew = realloc(pBuffer, newSize);
  if ( newSize > oldSize && pNew ) {
    size_t diff = newSize - oldSize;
    void* pStart = ((char*)pNew) + oldSize;
    memset(pStart, 0, diff);
  }
  return pNew;
}

void graph_add_edge(Graph g, int u, int v)
{
    if (graph_has_edge(g, u, v)) return;
     
    g->adjacency_matrix[u][v/8] |= 1<<(7-v%8);
    g->adjacency_matrix[v][u/8] |= 1<<(7-u%8);
    struct node_t* source = g->nodes[u];
    struct node_t* sink = g->nodes[v];
    /* do we need to grow the list? */
    while (source->d >= source->len)
    {
        int oldsize = source->len;
        source->len *= 2;
        source->neighbours =
            realloc_neighbours(source->neighbours,
                    sizeof(struct node_t*) * oldsize,
                    sizeof(struct node_t*) * (source->len));
    }

    /* now add the new sink */
    source->neighbours[source->d++] = sink;
    
    /* do we need to grow the list? */
    while (sink->d >= sink->len)
    {
        int oldsize = sink->len;
        sink->len *= 2;
        sink->neighbours =
            realloc_neighbours(sink->neighbours,
                    sizeof(struct node_t*) * oldsize,
                    sizeof(struct node_t*) * sink->len);
    }

    /* now add the new source */
    sink->neighbours[sink->d++] = source;
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
    int found_outer = 0;
    /* for each neighbour of vertex but the last one
       (since there are no other neighbours to connect
       to) */
    for (int i = 0; i < current->len && found_outer < degree - 1; i++)
    {
        /* add an edge to all other neighbours
        */
        int found_inner = i+1;
        if (!current->neighbours[i]) continue;
        if (node_invalid(g,current->neighbours[i]->id)) continue;
        for (int j = i+1; j < current->len && found_inner < degree; j++)
        {
            if (!current->neighbours[j]) continue;
            if (node_invalid(g,current->neighbours[j]->id)) continue;
            graph_add_edge(g, current->neighbours[i]->id,
             current->neighbours[j]->id);
            found_inner++;
        }
        found_outer++;        
    }
    graph_delete_vertex(g, vertex);
    return degree;
}

/* Use this function instead of the elimination function
    when doing MCS strategy
*/
void graph_include_vertex(Graph g, int vertex) {
    /* If a vertex is included, all its neighbours
        move up in the priority lists by one index 
    */
    int found = 0;
    for (int i = 0; i < g->nodes[vertex]->len && found < g->nodes[vertex]->d; i++)
    {
        int neighbour = g->nodes[vertex]->neighbours[i]->id;
        if (node_invalid(g, neighbour)) continue;
        found++;
        int current = g->nodes[neighbour]->priority_index;
        priority_delete_node(g, neighbour);
        priority_add_node(g, neighbour, current+1);
    }    
}

void graph_delete_vertex(Graph g, int vertex) {
    if (node_invalid(g, vertex)) return;
    struct node_t* current = g->nodes[vertex];
    /* decrease the degree of all neighbours */
    int found = 0;
    for (int i = 0; i < current->len && found < current->d; i++)
    {
        if (!current->neighbours[i]) continue;
        if (node_invalid(g, current->neighbours[i]->id)) continue;
        current->neighbours[i]->d--;
        found++;
    }
    assert(found == current->d);
    current->is_deleted = 1;
    g->n--;
    g->m -= found;
}

/* void graph_delete_edge(Graph g, int vertex1, int vertex2) {
    if(node_invalid(g, vertex1) ||
        node_invalid(g, vertex2)) return;
    
    struct node_t *v1 = g->nodes[vertex1];
    struct node_t *v2 = g->nodes[vertex2];


} */
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
    int found_outer = 0;
    for (int i = 0; i < current->len && found_outer < current->d; i++)
    {
        if (node_invalid(g, current->neighbours[i]->id)) continue;
        // check if all other neighbour are connected to it
        found_outer++;
        int found_inner = 0;
        for (int j = i+1; j < current->len && found_inner < current->d; j++)
        {
            if (node_invalid(g, current->neighbours[j]->id)) continue;
            found_inner++;
            if (!graph_has_edge(g, g->nodes[i]->neighbours[i]->id, g->nodes[i]->neighbours[j]->id)) result++;
        }        
    }
    return result;
}

int graph_vertex_cardinality(Graph g, int vertex) {
    if (node_invalid(g, vertex) || g->nodes[vertex]->in_set) return -1;
    return g->nodes[vertex]->priority_index;
}

int graph_has_edge(Graph g, int source, int sink)
{
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

/*  Initialise the priority lists with the
    degrees of the nodes */
void calc_initial_degrees(Graph g) {
    g->strategy = degree;
    for (int i = 0; i < g->nodes_len; i++)
    {
        if (node_invalid(g, i)) continue;
        g->nodes[i]->priority_index = g->nodes[i]->d;
        priority_add_node(g, i, g->nodes[i]->priority_index);
    }    
}

/*  Initialise the priority lists with 0 for every node 
    for later use with the mcs method */
void calc_initial_mcs(Graph g) {
    g->strategy = mcs;
    /* There is no ordering yet, so every
        node is in the 0 set 
    */
    for (int i = 0; i < g->n; i++)
    {
        priority_add_node(g, i, 0);
    }
}

/*  Update the priority lists for min-degree method
    upon eliminating the node g->nodes[node] 
*/
void update_degree(Graph g, int node) {
    struct node_t *current = g->nodes[node];
    int found = 0;
    for (int i = 0; i < current->len && found < current->d; i++)
    {
        if(!current->neighbours[i]) continue;            
        struct node_t *neighbour_ptr = current->neighbours[i];
        int newdegree = neighbour_ptr->d - 1;
        neighbour_ptr->d = newdegree;
        found++;
        priority_delete_node(g, neighbour_ptr->id);
        priority_add_node(g, neighbour_ptr->id, newdegree);
    }
    assert(found == current->d);
    
}

int graph_min_fillin_index(Graph g) {
    return graph_min_vertex(g, graph_vertex_fillin);
}

int graph_max_cardinality_index(Graph g) {
    return g->priority->heads[g->priority->max_ptr]->id;
}

int graph_order_degree (Graph g) {
  int size = graph_vertex_count(g);
  int width = 0;
  calc_initial_degrees(g);
  for (int i = 0; i < size; i++)
  {
    struct node_t* best_node = g->priority->heads[g->priority->min_ptr];
    /* decrease index of the neighbours in priority lists */
    update_degree(g, best_node->id);
    priority_delete_node(g, best_node->id);
    int current_width = graph_eliminate_vertex(g, best_node->id);
    update_degree(g, best_node->id);
    if (current_width > width) width = current_width;
    g->ordering[i] = best_node->id;
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
  calc_initial_mcs(g);
  for (int i = size-1; i > 0; i--) {
    int best_node = g->priority->heads[g->priority->max_ptr]->id;
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
            fprintf(stream, " %d", g->nodes[i]->neighbours[j]->id);
        }
        fprintf(stream, "\n");
    }    
}
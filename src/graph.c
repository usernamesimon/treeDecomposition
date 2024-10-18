/* Based on https://www.cs.yale.edu/homes/aspnes/pinewiki/C(2f)Graphs.html
 */
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

#include "graph.h"

#define ALIGNMENT 16
//#define VALIDATE_FILLIN 1

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
int graph_eliminate_vertex(Graph g, int vertex, int *neighbourhood);

struct node_t
{
    int id;              /* id of the node */
    int degree;          /* number of successors */
    char is_deleted;     /* deletion flag */
    char in_set;         /* true if the node is already in ordering */
    int priority_index;  /* this node is contained in <g->priority->heads[priority_index]>*/
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
struct Priority_t
{
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

struct graph
{
    int n;                 /* number of vertices */
    int m;                 /* number of edges */
    struct node_t **nodes; /* actual array of vertex pointers */

    /* if a vertex is deleted, <n> decreases
        but <nodes_len> stays the same. Since
        deleting a node is simply setting a
        flag, the valid entries in <nodes> are
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
    char **adjacency_matrix;
    int adjacency_size; /* size of adjacency matrix rows */

    int *ordering; /* holds the ordering produced by an elimination ordering*/

    struct Priority_t *priority; /* structure for determining the next node
                                    to eliminate */
    // enum strategy_t *strategy;
};

/* resize memory and initialize new space to 0 */
void *realloc_zero(void *pBuffer, size_t oldSize, size_t newSize)
{
    void *pNew = realloc(pBuffer, newSize);
    if (newSize > oldSize && pNew)
    {
        size_t diff = newSize - oldSize;
        void *pStart = ((char *)pNew) + oldSize;
        memset(pStart, 0, diff);
    }
    return pNew;
}

void bitwise_or(char *result, char *a, char *b, int size)
{
#ifdef AVX

#else
    for (int i = 0; i < size; i++)
    {
        *(result + i) = *(a + i) | *(b + i);
    }
#endif
}

void bitwise_and(char *result, char *a, char *b, int size)
{
#ifdef AVX

#else
    for (int i = 0; i < size; i++)
    {
        *(result + i) = *(a + i) & *(b + i);
    }
#endif
}

char bit_vectors_equal (char *a, char *b, int size) {
#ifdef AVX

#else
    for (int i = 0; i < size; i++)
    {
        if ((*(a + i) ^ *(b + i))>0) return 0;
    }
    return 1;
#endif
}
/* Given 2 adjacency lists calculates
    - a_not_b: vertices that are neighbours of a but not b
    - b_not_a: vertices that are neighbours of b but not a
*/
void calculate_uncommon_neigbours(char *a_not_b, char *b_not_a, char *a, char *b, int size)
{
#ifdef AVX

#else
    if (!a_not_b) {
        for (int i = 0; i < size; i++)
        {
            *(b_not_a + i) = ~*(a + i) & *(b + i);
        }
        return;
    }
    else if (!b_not_a) {
        for (int i = 0; i < size; i++)
        {
            *(a_not_b + i) = *(a + i) & ~*(b + i);
        }
        return;
    }
    for (int i = 0; i < size; i++)
    {

        *(a_not_b + i) = *(a + i) & ~*(b + i);
        *(b_not_a + i) = ~*(a + i) & *(b + i);
    }
#endif
}

/* Given 2 adjacency lists calculates
    - common: vertices that are neigbours to both vertices
    - a_not_b: vertices that are neighbours of a but not b
    - b_not_a: vertices that are neighbours of b but not a
*/
void calculate_common_uncommon_neighbours(char *common, char *a_not_b, char *b_not_a, char *a, char *b, int size)
{
#ifdef AVX

#else
    for (int i = 0; i < size; i++)
    {
        *(common + i) = *(a + i) & *(b + i);
        *(a_not_b + i) = *(a + i) & ~*(b + i);
        *(b_not_a + i) = ~*(a + i) & *(b + i);
    }
#endif
}

int number_of_set_bits(char *ch_pointer, int size)
{
    int result = 0;
    /* Since we are only counting bits, we don't care
        about Endianness here
    */
    uint32_t* pointer = (uint32_t*)ch_pointer;
    int j;
    for (j = 0; j < size / 4; j++)
    {
        uint32_t i = pointer[j];
        i = i - ((i >> 1) & 0x55555555);                // add pairs of bits
        i = (i & 0x33333333) + ((i >> 2) & 0x33333333); // quads
        i = (i + (i >> 4)) & 0x0F0F0F0F;                // groups of 8
        i *= 0x01010101;                                // horizontal sum of bytes
        result += i >> 24;                                 // return just that top byte (after truncating to 32-bit even when int is wider than uint32_t)
    }
    if (size%4==0) return result;

    static const uint8_t NIBBLE_LOOKUP [16] =
        {
            0, 1, 1, 2, 1, 2, 2, 3, 
            1, 2, 2, 3, 2, 3, 3, 4
        };

    for (int k = 0; k < size%4; k++)
    {
        uint8_t byte = ch_pointer[4*j+k];
        result += NIBBLE_LOOKUP[byte & 0x0F];
        byte = byte >> 4;
        result += NIBBLE_LOOKUP[byte];
    }
    return result;
}

/* Get the index of the next set bit in
    <adjacency_list> starting from <start_index>.
    If the bit at <start_index> is set, <start_index>
    is returned.
    If no set bit is found the first <size> bytes
    from <adjacency_list>, -1 is returned
*/
int get_next_bit_index(char *adjacency_list, int start_index, int size)
{
    int work = start_index / 8;
    // get next not empty byte
    while (work < size && adjacency_list[work] == 0)
        work++;
    // no next neighbour found
    if (work >= size)
        return -1;
    for (int i = start_index % 8; i < 8; i++)
    {
        if (adjacency_list[work] & 1 << (7 - i))
            return work * 8 + i;
    }
    /* if only bits before start_index in this byte
        are set, we have to jump to the next byte
    */
    work++;
    // get next not empty byte
    while (work < size && adjacency_list[work] == 0)
        work++;
    // no next neighbour found
    if (work >= size)
        return -1;
    for (int i = 0; i < 8; i++)
    {
        if (adjacency_list[work] & 1 << (7 - i))
            return work * 8 + i;
    }
    // should not be reached
    assert(0);
}

/* check if a node exists and is not deleted */
char node_invalid(Graph g, int node)
{
    if (node < 0 ||
        node >= g->nodes_len ||
        g->nodes[node]->is_deleted)
        return 1;
    return 0;
}

/* add node g->nodes[node_index] to the priority
    lists with set number index
*/
void priority_add_node(Graph g, int node_index, int index)
{
    if (node_invalid(g, node_index))
        return;
    /*  Grow array if necessary
        This can happen if a node leads to a lot of
        fill-in edges when using that heuristic.
    */
    while (index >= g->priority->len)
    {
        size_t newsize = g->priority->len * 2;
        g->priority->heads = (struct node_t **)realloc_zero(
            g->priority->heads, sizeof(struct node_t *) * g->priority->len, sizeof(struct node_t *) * newsize);
        g->priority->tails = (struct node_t **)realloc_zero(
            g->priority->tails, sizeof(struct node_t *) * g->priority->len, sizeof(struct node_t *) * newsize);
        g->priority->len = newsize;
    }

    struct node_t *node = g->nodes[node_index];
    /* if list with index <index> is not empty */
    if (g->priority->tails[index])
    {
        /* link in new node*/
        node->prev = g->priority->tails[index];
        // assert(!g->priority->tails[index]->next);
        g->priority->tails[index]->next = node;
        g->priority->tails[index] = node;
    }
    else
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
    //assert(node->next != node);
}

/* remove node g->nodes[node_index] from the
    priority lists
*/
void priority_delete_node(Graph g, int node_index)
{
    if (node_invalid(g, node_index))
        return;
    /* if this is the last node we won't need to do anything*/
    if(g->n <= 1) return;

    /* unlink the node */
    struct node_t *node = g->nodes[node_index];
    if (node->prev)
        node->prev->next = node->next;

    if (node->next)
        node->next->prev = node->prev;

    /*  if this node was the head, we need to set
        a new one */
    if (node == g->priority->heads[node->priority_index])
    {
        g->priority->heads[node->priority_index] = node->next;

        /*  if this was the last node in the list
            we may need to decrease max_ptr and
            increase min_ptr
        */
        if (!node->next)
        { 
            while (g->priority->max_ptr >= 0 && !g->priority->heads[(g->priority->max_ptr)])
                g->priority->max_ptr--;
            while (g->priority->min_ptr < g->priority->len && !g->priority->heads[(g->priority->min_ptr)])
                g->priority->min_ptr++;
        }
    }

    /* if this was the tail we need to set a new one */
    if (node == g->priority->tails[node->priority_index])
    {
        g->priority->tails[node->priority_index] = node->prev;

        // max_ptr and min_ptr should have been corrected already
    }
    node->next = node->prev = NULL;
}

Graph graph_create(int n)
{
    Graph g = malloc(sizeof(struct graph));
    if(!g) return NULL;
    g->n = n;
    g->m = 0;
    g->nodes = malloc(sizeof(struct node_t *) * n);
    if(!g->nodes) {
        free(g); return NULL;}
    g->nodes_len = n;
    g->adjacency_matrix = malloc(sizeof(char *) * n);
    g->ordering = malloc(sizeof(int) * n);
    if(!g->ordering) {
        free(g->nodes); free(g); return NULL; }
    /* At most a node can be connected to all
        other nodes which could be in the ordering
        potentially, therefore n-1 is the maximum for priority */
    g->priority = malloc(sizeof(struct Priority_t));
    if(!g->priority) {
        free(g->ordering); free(g->nodes); free(g); return NULL; }
    g->priority->len = n;
    g->priority->max_ptr = 0;
    g->priority->min_ptr = INT_MAX;
    g->priority->heads = malloc(sizeof(struct node_t *) * g->priority->len);
    if(!g->priority->heads) {
        free(g->priority); free(g->ordering); free(g->nodes); free(g); return NULL; }
    g->priority->tails = malloc(sizeof(struct node_t *) * g->priority->len);
    if(!g->priority->tails) {
        free(g->priority->heads); free(g->priority); free(g->ordering); free(g->nodes); free(g); return NULL; }

    // g->strategy = unspecified;

    /* calculate the size of the adjacency matrix.
        We need one more byte if the number of
        vertices is not a multiple of 8.
    */
    int size = n / 8;
    if (n % 8 != 0)
        size++;
    /* Resize to next multiple of alignment */
    while (size % ALIGNMENT != 0)
    {
        size++;
    }
    
    g->adjacency_size = size;

    for (int i = 0; i < n; i++)
    {
        g->adjacency_matrix[i] = (char *)aligned_alloc(ALIGNMENT, size);
        if(!g->adjacency_matrix[i]) return NULL;
        memset(g->adjacency_matrix[i], 0, size);

        g->nodes[i] = malloc(sizeof(struct node_t));
        if(!g->nodes[i]) return NULL;
        g->nodes[i]->id = i;
        g->nodes[i]->degree = 0;
        g->nodes[i]->is_deleted = 0;
        g->nodes[i]->in_set = 0;
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

Graph graph_import(FILE *fstream)
{

    /* get the size of the graph */
    int n;
    char *line = NULL;
    size_t linelen = 0;

    if (fstream == NULL)
    {
        return NULL;
    }
    if (getline(&line, &linelen, fstream) < 0)
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
    // fprintf(stdout, "Number of vertices: %d\n", n);

    Graph g = graph_create(n);
    if(!g) return NULL;

    // populate the adjacency lists of each node
    for (int i = 0; i < g->n; i++)
    {
        // get the next line from file
        if (getline(&line, &linelen, fstream) < 0)
        {
            // unexpected EOF etc.
            fprintf(stderr, "Error parsing the adjacency list for node %d\n", i);
            graph_destroy(g);
            free(line);
            return NULL;
        }
        // split line into tokens and convert to int
        int neighbour_count = 0;
        int node = 0;
        char *str = line;
        tok = strtok(str, " ");
        // the first entry in a line is the node itself
        if (tok != NULL)
        {
            if (sscanf(tok, "%d", &node) != 1)
            {
                fprintf(stderr, "Conversion error\n");
                graph_destroy(g);
                return NULL;
            }
            //assert(node == i);
            tok = strtok(NULL, " ");
        }
        while (tok != NULL)
        {
            if(neighbour_count > n) {
                fprintf(stderr, "Error importing: too many neighbours\n");
                graph_destroy(g);
                return NULL;
            };
            int neighbour;
            if (sscanf(tok, "%d", &neighbour) != 1)
            {
                fprintf(stderr, "Conversion error\n");
                graph_destroy(g);
                return NULL;
            }
            neighbour_count++;
            graph_add_edge(g, node, neighbour);

            tok = strtok(NULL, " ");
        }
    }
    free(line);
    return g;
}

Graph graph_copy(Graph g)
{
    if(!g) return NULL;
    if(!g->nodes) return NULL;

    Graph copy = malloc(sizeof(struct graph));
    if(!copy) return NULL;
    int n = g->nodes_len;
    copy->n = g->n;
    copy->m = g->m;
    copy->nodes = malloc(sizeof(struct node_t *) * n);
    if(!copy->nodes) return NULL;
    copy->nodes_len = g->nodes_len;
    copy->adjacency_matrix = malloc(sizeof(char *) * g->n);
    copy->adjacency_size = g->adjacency_size;
    copy->ordering = malloc(sizeof(int) * n);
    if(!copy->ordering) return NULL;

    copy->priority = malloc(sizeof(struct Priority_t));
    if(!copy->priority) return NULL;
    copy->priority->len = g->priority->len;
    copy->priority->max_ptr = g->priority->max_ptr;
    copy->priority->min_ptr = g->priority->min_ptr;
    copy->priority->heads = malloc(sizeof(struct node_t *) * g->priority->len);
    if(!copy->priority->heads) return NULL;
    copy->priority->tails = malloc(sizeof(struct node_t *) * g->priority->len);
    if(!copy->priority->tails) return NULL;

    // copy->strategy = g->strategy;

    for (int i = 0; i < n; i++)
    {
        copy->adjacency_matrix[i] = (char *)aligned_alloc(ALIGNMENT, copy->adjacency_size);
        if(!copy->adjacency_matrix[i]) return NULL;
        memcpy(copy->adjacency_matrix[i], g->adjacency_matrix[i], copy->adjacency_size);

        copy->nodes[i] = malloc(sizeof(struct node_t));
        if(!copy->nodes[i]) return NULL;
        copy->nodes[i]->id = g->nodes[i]->id;
        copy->nodes[i]->degree = g->nodes[i]->degree;
        copy->nodes[i]->is_deleted = g->nodes[i]->is_deleted;
        copy->nodes[i]->in_set = g->nodes[i]->in_set;
        copy->nodes[i]->priority_index = g->nodes[i]->priority_index;

        /* If the original has next/prev link to the node
            with the same id in the copy, otherwise
            to NULL pointer
        */
        if (g->nodes[i]->next)
        {
            copy->nodes[i]->next = copy->nodes[g->nodes[i]->next->id];
        }
        else
            copy->nodes[i]->next = NULL;
        if (g->nodes[i]->prev)
        {
            copy->nodes[i]->prev = copy->nodes[g->nodes[i]->prev->id];
        }
        else
            copy->nodes[i]->prev = NULL;
        copy->ordering[i] = g->ordering[i];
    }

    /* Link to new heads and tails if they were present in original*/
    for (int i = 0; i < g->priority->len; i++)
    {
        if (g->priority->heads[i])
        {
            copy->priority->heads[i] = copy->nodes[g->priority->heads[i]->id];
        }
        else
            copy->priority->heads[i] = NULL;
        if (g->priority->tails[i])
        {
            copy->priority->tails[i] = copy->nodes[g->priority->tails[i]->id];
        }
        else
            copy->priority->tails[i] = NULL;
    }

    return copy;
}

void graph_destroy(Graph g)
{
    if(!g) return;
    int i;

    for (i = 0; i < g->nodes_len; i++)
    {
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

void graph_add_edge(Graph g, int u, int v)
{
    if (graph_has_edge(g, u, v))
        return;

    g->adjacency_matrix[u][v / 8] |= 1 << (7 - v % 8);
    g->nodes[u]->degree++;
    g->adjacency_matrix[v][u / 8] |= 1 << (7 - u % 8);
    g->nodes[v]->degree++;

    /* bump edge count */
    g->m++;
}

/* From the adjacency matrix take the row corresponding
    to <vertex>, convert it to a list of indizes and write
    the result to buffer. The caller is responsible for
    sizing the buffer correctly! (degree of vertex) */
void convert_node_adj_to_list(Graph g, int vertex, int *buffer) {
    int neighbour = -1;
    for (int i = 0; i < g->nodes[vertex]->degree; i++)
    {
        neighbour = get_next_bit_index(g->adjacency_matrix[vertex], neighbour+1, g->adjacency_size);
        buffer[i] = neighbour;
    }
    
}

/* eliminate a vertex from the graph
   and return its degree upon elimination
*/
int graph_eliminate_vertex(Graph g, int vertex, int *neighbourhood)
{
    char need_to_free = 0;
    if (node_invalid(g, vertex))
        return -1; // TODO: what to return?
    
    int degree = g->nodes[vertex]->degree;
    assert(!(degree < 0));
    if (!neighbourhood) {
        neighbourhood = (int*)malloc(sizeof(int)*degree);
        need_to_free = 1;
    }
    /* get the list of neighbours */
    convert_node_adj_to_list(g, vertex, neighbourhood);

    /* To form a clique we have to bitwise-OR the adjacency
        list of vertex to all its neighbours */
    for (size_t i = 0; i < degree; i++)
    {
        int neighbour = neighbourhood[i];
        char* work = (char*)aligned_alloc(ALIGNMENT, g->adjacency_size);
        memset(work, 0, g->adjacency_size);
        bitwise_or(work,
                    g->adjacency_matrix[neighbour],
                    g->adjacency_matrix[vertex],
                    g->adjacency_size);
        /* Remove neighbour from its own adjacency list */
        work[neighbour / 8] &= ~(0x1 << (7 - neighbour % 8));

        /* Update adjacency matrix and degree of neighbour*/
        free(g->adjacency_matrix[neighbour]);
        g->adjacency_matrix[neighbour] = work;
        g->nodes[neighbour]->degree = number_of_set_bits(work, g->adjacency_size); 
    }    
    graph_delete_vertex(g, vertex);
    if (need_to_free) free(neighbourhood);
    return degree;
}

void graph_delete_vertex(Graph g, int vertex)
{
    if (node_invalid(g, vertex))
        return;
    struct node_t *current = g->nodes[vertex];
    /* decrease the degree of all neighbours */
    for (int i = 0; i < g->nodes_len; i++)
    {
        if (!graph_has_edge(g, vertex, i))
            continue;
        graph_delete_edge(g, vertex, i);
    }
    priority_delete_node(g, vertex);
    current->is_deleted = 1;
    g->n--;
}

void graph_delete_edge(Graph g, int vertex1, int vertex2)
{

    g->adjacency_matrix[vertex1][vertex2 / 8] &= ~(0x1 << (7 - vertex2 % 8));
    g->adjacency_matrix[vertex2][vertex1 / 8] &= ~(0x1 << (7 - vertex1 % 8));
    g->nodes[vertex1]->degree--;
    g->nodes[vertex2]->degree--;
    g->m--;
}

int graph_vertex_count(Graph g)
{
    return g->n;
}

int graph_edge_count(Graph g)
{
    return g->m;
}

int graph_vertex_priority(Graph g, int vertex)
{
    if (node_invalid(g, vertex) || g->nodes[vertex]->in_set)
        return -1;
    return g->nodes[vertex]->priority_index;
}

int graph_has_edge(Graph g, int source, int sink)
{
    return g->adjacency_matrix[source][sink / 8] & 1 << (7 - sink % 8);
}

int graph_min_vertex(Graph g,
                     int (*f)(Graph g, int vertex))
{
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
void calc_initial_degrees(Graph g)
{
    // g->strategy = degree;
    for (int i = 0; i < g->nodes_len; i++)
    {
        if (node_invalid(g, i))
            continue;
        g->nodes[i]->priority_index = g->nodes[i]->degree;
        priority_add_node(g, i, g->nodes[i]->priority_index);
    }
}

/*  Initialise the priority lists with 0 for every node
    for later use with the mcs method */
void calc_initial_mcs(Graph g)
{
    /* The cardinality set is still empty, so every
        node is in the 0 set
    */
    for (int i = 0; i < g->nodes_len; i++)
    {
        if (node_invalid(g, i))
            continue;
        priority_add_node(g, i, 0);
    }
}

/*  Initialise priority lists for min-fill-in strategy.
    A node gets the number of fill-in edges created
    if it was eliminated from the graph.
*/
int node_calc_fillin(Graph g, int node)
{
    int degree = g->nodes[node]->degree;
    int fill_in_edges = 0;
    int* neighbours = (int*)malloc(sizeof(int)*degree);
    convert_node_adj_to_list(g, node, neighbours);
    char* work = (char*)aligned_alloc(ALIGNMENT, g->adjacency_size);

    for (int neighbour = 0; neighbour < degree; neighbour++)
    {
        /* Calculate the edges to add for neighbour */
        calculate_uncommon_neigbours(work, NULL, g->adjacency_matrix[node], 
                    g->adjacency_matrix[neighbours[neighbour]], g->adjacency_size);
        /* Subtract 1 because neighbour needs no edge to itself */
        fill_in_edges += number_of_set_bits(work, g->adjacency_size) - 1;        
    }
    /* We counted each edge twice */
    fill_in_edges = fill_in_edges / 2;

    free(neighbours);
    free(work);
    return fill_in_edges;
}

/* Calculate the number of fill-in edges for
    every vertex and populate the priority lists
    accordingly
*/
void calc_initial_fillin(Graph g)
{
    for (int i = 0; i < g->nodes_len; i++)
    {
        if (node_invalid(g, i))
            continue;
        int fillin = node_calc_fillin(g, i);
        priority_add_node(g, i, fillin);
    }
}

/*  Update the priority lists for min-degree method
    upon eliminating the node g->nodes[node]
*/
void node_update_priority_degree(Graph g, int node)
{

    int newpriority = g->nodes[node]->degree;
    if (g->nodes[node]->priority_index == newpriority)
        return;
    priority_delete_node(g, node);
    priority_add_node(g, node, newpriority);
}

/* Update the the priorities of the neighbours
    when including vertex in the ordering when
    doing MCS strategy
*/
void node_update_priority_mcs(Graph g, int vertex)
{
    /* If a vertex is included, all its neighbours
        move up in the priority lists by one index
    */
    //assert(!g->nodes[vertex]->in_set);
    int size = sizeof(int)*g->nodes[vertex]->degree;
    int* neighbours = (int*)malloc(size);
    convert_node_adj_to_list(g, vertex, neighbours);
    for (int i = 0; i < g->nodes[vertex]->degree; i++)
    {
        
        int current = neighbours[i];
        priority_delete_node(g, current);
        priority_add_node(g, current, g->nodes[current]->priority_index + 1);
    }
    g->nodes[vertex]->in_set = 1;
    priority_delete_node(g, vertex);
    free(neighbours);
}

int node_update_priority_fillin_and_eliminate_vertex(Graph g, int vertex, char *common, char *vertex_minus_neighbour,
                                                     char *neighbour_minus_vertex, char *neighbour1_minus_neigbhour2, char *neighbour2_minus_neighbour1)
{
    if (node_invalid(g, vertex))
        return -1; // TODO: what to return?

    int degree = g->nodes[vertex]->degree;
    /* for each vertex of graph */
    char *adj_list = g->adjacency_matrix[vertex];

    /* get the next neighbour */
    int neighbour = get_next_bit_index(adj_list,
                                       0, g->adjacency_size);

    for (int i = 0; i < degree; i++)
    {

        if (node_invalid(g, neighbour)) return -1;

        memset(vertex_minus_neighbour, 0, g->adjacency_size);
        memset(neighbour_minus_vertex, 0, g->adjacency_size);
        calculate_uncommon_neigbours(
            vertex_minus_neighbour, neighbour_minus_vertex,
            adj_list, g->adjacency_matrix[neighbour], g->adjacency_size);

        /* Add edges to the clique:
            vertex_minus_neighbour contains the neighbours
            of vertex which neighbour is not yet connected to.
            Only add edges to vertices with index higher than
            neighbour to avoid doing it twice. */
        int new_neighbour = get_next_bit_index(vertex_minus_neighbour,
                                               neighbour + 1, g->adjacency_size);
        while (!node_invalid(g, new_neighbour))
        {
            graph_add_edge(g, neighbour, new_neighbour);
            memset(common, 0, g->adjacency_size);
            memset(neighbour1_minus_neigbhour2, 0, g->adjacency_size);
            memset(neighbour2_minus_neighbour1, 0, g->adjacency_size);
            calculate_common_uncommon_neighbours(common, neighbour1_minus_neigbhour2, neighbour2_minus_neighbour1,
                                                 g->adjacency_matrix[neighbour], g->adjacency_matrix[new_neighbour],
                                                 g->adjacency_size);
            /* remove new neighbours from each others' exclusive neighbour lists */
            /* TODO: Could this be achieved by calculating common and uncommon neighbours before
                adding the edge? 
            */
            neighbour1_minus_neigbhour2[new_neighbour / 8] &= ~(1 << (7 - new_neighbour % 8));
            neighbour2_minus_neighbour1[neighbour / 8] &= ~(1 << (7 - neighbour % 8));

            /*  Edges that we add now we don't need to add
                later. Neighbours that are common to
                neighbour and new_neighbour would have added this
                edge as well, thus we decrease the fillin count
                for these common neighbours
            */
            /* Remove vertex from common list, as it is being deleted */
            common[vertex / 8] &= ~(0x1 << (7 - vertex % 8));
            int common_neighbour = get_next_bit_index(common, 0, g->adjacency_size);
            while (!node_invalid(g, common_neighbour))
            {
                int current = g->nodes[common_neighbour]->priority_index;
                priority_delete_node(g, common_neighbour);
                priority_add_node(g, common_neighbour, current - 1);
                common_neighbour = get_next_bit_index(common,
                                                      common_neighbour + 1, g->adjacency_size);
            }

            /*  Since neighbour <a> and new_neighbour <b> are now neighbours,
                any neighbours that are not common to both add to each
                others' fill-in count. Consider a neighbour <c> to <a> which
                is not a neighbour to <b>. Eliminating <a> would now require
                to add an edge from <b> to <c>, therefore we must increase
                the fill-in count by 1 for every exclusive neighbour of <a>.
            */
            int increase_neighbour = number_of_set_bits(neighbour1_minus_neigbhour2, g->adjacency_size);
            
            int increase_new_neighbour = number_of_set_bits(neighbour2_minus_neighbour1, g->adjacency_size);
            if (increase_neighbour > 0)
            {
                int current = g->nodes[neighbour]->priority_index;
                priority_delete_node(g, neighbour);
                priority_add_node(g, neighbour, current+increase_neighbour);
            }
            if (increase_new_neighbour > 0)
            {
                int current = g->nodes[new_neighbour]->priority_index;
                priority_delete_node(g, new_neighbour);
                priority_add_node(g, new_neighbour, current+increase_new_neighbour);
            }

            new_neighbour = get_next_bit_index(vertex_minus_neighbour,
                                               new_neighbour + 1, g->adjacency_size);
        }

        /*  Consider the current vertex <a>, its neighbour <b> and
            a neighbour of b <c> where c is not a neigbour of a.
            Eliminating b would have required a fill-in edge
            between a and c. As vertex a is being deleted,
            this is no longer neccessary, thus decreasing
            the fill-in count of b by 1. So the fill-in count
            of b should be decreased by the number of bit in
            neighbour_minus_vertex.
        */
        /*  We need to remove the vertex itself from neighbour_minus_vertex
            since this would imply a loop on vertex, which would not have
            been added when eliminating neighbour
        */
        neighbour_minus_vertex[vertex / 8] &= ~(1 << (7 - vertex % 8));

        int decrease = number_of_set_bits(neighbour_minus_vertex, g->adjacency_size);
        if (decrease > 0)
        {
            int new_priority = g->nodes[neighbour]->priority_index - decrease;
            //assert(new_priority >= 0);
            priority_delete_node(g, neighbour);
            priority_add_node(g, neighbour, new_priority);
        }

        neighbour = get_next_bit_index(adj_list,
                                       neighbour + 1, g->adjacency_size);
    }
    graph_delete_vertex(g, vertex);
    return degree;
}

/* 
    TODO: Maybe check if at one point when eliminating vertices
    the intermediate graph has become complete. In this case we
    could just add nodes in an arbitrary order because any solution
    is equivalent.
*/

int graph_order_degree(Graph g)
{
    int size = graph_vertex_count(g);
    int width = 0;
    calc_initial_degrees(g);
    /* Buffer for neighbours of eliminated vertex */
    int *neighbours = (int *)malloc(sizeof(int) * g->nodes_len);
    /* Highest index that might be set */
    int d = g->n;
    for (int i = 0; i < size; i++)
    {
        struct node_t *best_node = g->priority->heads[g->priority->min_ptr];
        memset(neighbours, 0, sizeof(int)*d);
        int d = best_node->degree;
        int current_width = graph_eliminate_vertex(g, best_node->id, neighbours);
        if (current_width > width)
            width = current_width;
        g->ordering[i] = best_node->id;
        /* update index of the neighbours in priority lists */
        for (int j = 0; j < d; j++)
        {
            node_update_priority_degree(g, neighbours[j]);
        }
    }
    free(neighbours);
    return width;
}

int graph_order_fillin(Graph g)
{
    /* create bit vectors for calculations*/
    char *common = (char*)aligned_alloc(ALIGNMENT, g->adjacency_size);
    char *vertex_minus_neighbour = (char*)aligned_alloc(ALIGNMENT, g->adjacency_size);
    char *neighbour_minus_vertex = (char*)aligned_alloc(ALIGNMENT, g->adjacency_size);
    char *neighbour1_minus_neighbour2 = (char*)aligned_alloc(ALIGNMENT, g->adjacency_size);
    char *neighbour2_minus_neighbour1 = (char*)aligned_alloc(ALIGNMENT, g->adjacency_size);

    int size = graph_vertex_count(g);
    int width = 0;
    calc_initial_fillin(g);
    for (int i = 0; i < size; i++)
    {
        struct node_t *best_node = g->priority->heads[g->priority->min_ptr];
        int current_width = node_update_priority_fillin_and_eliminate_vertex(
            g, best_node->id, common, vertex_minus_neighbour,
            neighbour_minus_vertex, neighbour1_minus_neighbour2,
            neighbour2_minus_neighbour1);
        if (current_width < 0) {
            fprintf(stderr, "There was an error calculating current width\n");
            return -1;
        }
        if (current_width > width)
            width = current_width;
        g->ordering[i] = best_node->id;
#ifdef VALIDATE_FILLIN
        for (int j = 0; j < g->nodes_len; j++)
        {
            if (node_invalid(g, j))
                continue;
            int order = node_calc_fillin(g, j);
            assert(order == g->nodes[j]->priority_index);
        }
#endif
    }
    free(common);
    free(vertex_minus_neighbour);
    free(neighbour_minus_vertex);
    free(neighbour1_minus_neighbour2);
    free(neighbour2_minus_neighbour1);
    return width;
}

int graph_order_mcs(Graph g)
{
    int size = graph_vertex_count(g);
    int width = 0;
    /* need copy to later calculate the width */
    Graph copy = graph_copy(g);
    calc_initial_mcs(g);
    /* Do the ordering */
    for (int i = size - 1; i >= 0; i--)
    {
        /* do a secondary priority -> min degree */
        struct node_t* best_node = g->priority->heads[g->priority->max_ptr];
        int best_degree = best_node->degree;
        struct node_t* next = best_node->next;
        while (next)
        {
            if (next->degree < best_degree) {
                best_node = next;
                best_degree = next->degree;
            }
            next = next->next;
        }
        
        g->ordering[i] = best_node->id;
        node_update_priority_mcs(g, best_node->id);
        graph_delete_vertex(g, best_node->id);
    }

    /* calculate treewidth
        As we did not actually do any eliminations we have to do it now */
    for (int i = 0; i < size; i++)
    {
        int current_width = graph_eliminate_vertex(copy, g->ordering[i], NULL);
        if (current_width > width)
            width = current_width;
    }
    graph_destroy(copy);
    return width;
}

/* checks if every vertex was used exactly once
    in the ordering of the graph
*/
char graph_ordering_plausible(Graph g)
{
    char *used = (char *)malloc(g->nodes_len);
    memset(used, 0, g->nodes_len);
    for (int i = 0; i < g->nodes_len; i++)
    {
        int index = g->ordering[i];
        if (index == -1)
            return 0;
        if (used[index])
            return 0;
        used[index] = 1;
    }
    return 1;
}

void graph_print(Graph g, FILE *stream)
{
    if (g == NULL)
        return;
    if (stream == NULL)
        return;

    fprintf(stream, "# nodes %d\n", g->n);
    for (int i = 0; i < g->nodes_len; i++)
    {
        fprintf(stream, "%d", i);
        if (g->nodes[i]->is_deleted)
            fprintf(stream, " d");
        for (int j = 0; j < g->nodes_len; j++)
        {
            if (!graph_has_edge(g, i, j))
                continue;
            fprintf(stream, " %d", j);
        }
        fprintf(stream, "\n");
    }
}

void graph_print_ordering(Graph g, FILE *stream) {
    /* Check if ordering was already calculated 
        (entries are initialized to -1)
    */
    if (g->ordering[0] < 0) return;

    fprintf(stream, "%d", g->ordering[0]);
    for (int i = 1; i < g->nodes_len; i++)
    {
        fprintf(stream, " %d", g->ordering[i]);
    }
    
}

void eo_to_treedecomp(){}

void graph_eo_to_treedecomp(Graph g) {

    for (size_t i = 0; i < g->nodes_len; i++)
    {
        /* code */
    }
    
}

/* Expects a single line with space-separated integers */
char graph_import_ordering(Graph g, FILE *fstream) {
    if(!g|!fstream) return 0;
    char* line = NULL;
    size_t linelen;
    if(getline(&line, &linelen, fstream)<0) {free(line);return 0;}
    if (!line) {free(line);return 0;}
    char* tok = strtok(line, " ");
    for (size_t i = 0; i < g->nodes_len; i++)
    {
        if (!tok) {free(line);return 0;}
        if (sscanf(tok, "%d", &g->ordering[i]) != 1)
        {
            fprintf(stderr, "Conversion error\n");
            free(line);
            return 0;
        }
        tok = strtok(NULL, " ");
    }
    free(line);
    return 1;    
}
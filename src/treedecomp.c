#include "treedecomp.h"
#include "graph.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

char *optarg;
int optind, opterr, optopt;

/* calculate an elimination ordering for g
    according to the min degree heuristic.
    Writes the ordering into set, returns
    the width of the ordering.
    Be sure to allocate enough space to set
    (The number of vertices of g).
    Attention: this function alters the graph.
*/
int order_degree (Graph g, int* set) {
  int size = graph_vertex_count(g);
  int width = 0;
  for (int i = 0; i < size; i++)
  {
    int best_node = graph_min_degree_index(g);
    int current_width = graph_eliminate_vertex(g, best_node);
    if (current_width > width) width = current_width;
    set[i] = best_node;
  }
  return width;
}

/* Same as order_degree but use 
    min fill-in heuristic
*/
int order_fillin (Graph g, int* set) {
  int size = graph_vertex_count(g);
  int width = 0;
  for (int i = 0; i < size; i++)
  {
    int best_node = graph_min_fillin_index(g);
    int current_width = graph_eliminate_vertex(g, best_node);
    if (current_width > width) width = current_width;
    set[i] = best_node;
  }
  return width;  
}

/* Again same usage, but use 
    maximum cardinality heuristic.
*/
int order_mcs (Graph g, int* set) {
  int size = graph_vertex_count(g);
  int width = 0;
  /* we need to set the initially empty ordering
      to values that are surely not an index of
      the graph
  */
  for (int i = 0; i < size; i++)
  {
    set[i] = -1;
  }  
  for (int i = size-1; i > 0; i--) {
    int best_node = graph_max_cardinality_index(g, set, size);
    set[i] = best_node;
    int current_width = graph_vertex_degree(g, best_node);
    if (current_width > width) width = current_width;
  }
  return width;
}

void print_file_header(FILE* fptr) {
  fprintf(fptr, "Filename, Width Min-Degree, Time Min-Degree, "
              "Width Min-Fill-in, Time Min-Fill-in, "
              "Width MCS, Time MCS\n");
}

int benchmark (char* inputpath, FILE* resultfile) {
  Graph g = graph_import(inputpath);
  if(g==NULL) return 1;
  Graph g1 = graph_copy(g);
  Graph g2 = graph_copy(g); 
  int n = graph_vertex_count(g);

  float start, end;
  int *ordering_d = (int*)malloc(sizeof(int)*n);
  start = clock();
  int width_d = order_degree(g, ordering_d);
  end = clock();
  float time_d = (end - start)/CLOCKS_PER_SEC;

  int *ordering_f = (int*)malloc(sizeof(int)*n);
  start = clock();
  int width_f = order_fillin(g1, ordering_f);
  end = clock();
  float time_f = (end-start)/CLOCKS_PER_SEC;

  int *ordering_mcs = (int*)malloc(sizeof(int)*n);
  start = clock();  
  int width_mcs = order_fillin(g2, ordering_mcs);
  end = clock();
  float time_mcs = (end-start)/CLOCKS_PER_SEC;

  fprintf(resultfile, "%s, %d, %f, %d, %f, %d, %f\n",
           inputpath, width_d, time_d, width_f, time_f,
            width_mcs, time_mcs);
  graph_destroy(g);
  graph_destroy(g1);
  graph_destroy(g2);
  free(ordering_d);
  free(ordering_f);
  free(ordering_mcs);
  return 0;
}

int
main (int argc, char **argv)
{
  char *inputpath = NULL;
  int uselist = 0;
  int index;
  int c;

  opterr = 0;


  while ((c = getopt (argc, argv, "s:f:")) != -1)
    switch (c)
      {
      case 'f':
        if (inputpath != NULL) {
          fprintf(stdout, 
                  "Ignoring Single file as list was provided\n");
        }
        inputpath = optarg;
        uselist = 1;
        break;
      case 's':
        if(uselist==1) {
          fprintf(stdout, 
                  "Ignoring Single file as list was provided\n");
        } else inputpath = optarg;

        break;
      case '?':
        if (optopt == 'f')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        return 1;
      case ':':
        fprintf(stderr, "No argument provided, aborting...\n");
        return 1;
      default:
        abort ();
      }


  //printf ("inputpath = %s\n", inputpath);

  for (index = optind; index < argc; index++)
    fprintf (stdout, "Non-option argument %s not supported\n", argv[index]);


  if (uselist == 1){
    fprintf(stderr, "File list not yet implemented\n");
    return 1;
  }

  //FILE* results = fopen("results.csv", "w");
  print_file_header(stdout);
  float start, end;
  start = clock();
  Graph g = graph_import(inputpath);
  if(g==NULL) return 1;
  Graph g1 = graph_copy(g);
  Graph g2 = graph_copy(g); 
  int n = graph_vertex_count(g);
  end = clock();
  float time_su = (end-start)/CLOCKS_PER_SEC;
  
  int *ordering_d = (int*)malloc(sizeof(int)*n);
  start = clock();
  int width_d = order_mcs(g, ordering_d);
  end = clock();
  float time_d = (end - start)/CLOCKS_PER_SEC;
  graph_destroy(g);
  graph_destroy(g1);
  graph_destroy(g2);
  fprintf(stdout, "Time for setup: %f\n"
          "Time for execution: %f\n"
          "Width: %d\n", time_su, time_d, width_d);
  
  //benchmark(inputpath, stdout);
  
  
  return 0;
}

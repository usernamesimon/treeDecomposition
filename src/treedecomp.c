#include "treedecomp.h"
#include "graph.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

char *optarg;
int optind, opterr, optopt;

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
  //int n = graph_vertex_count(g);

  float start, end;
  start = clock();
  int width_d = graph_order_degree(g);
  end = clock();
  float time_d = (end - start)/CLOCKS_PER_SEC;
  
  start = clock();
  int width_f = graph_order_fillin(g1);
  end = clock();
  float time_f = (end-start)/CLOCKS_PER_SEC;

  start = clock();  
  int width_mcs = graph_order_fillin(g2);
  end = clock();
  float time_mcs = (end-start)/CLOCKS_PER_SEC;

  fprintf(resultfile, "%s, %d, %f, %d, %f, %d, %f\n",
           inputpath, width_d, time_d, width_f, time_f,
            width_mcs, time_mcs);
  graph_destroy(g);
  graph_destroy(g1);
  graph_destroy(g2);
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
  end = clock();
  float time_su = (end-start)/CLOCKS_PER_SEC;
  
  start = clock();
  int width_d = graph_order_mcs(g);
  end = clock();
  float time_d = (end - start)/CLOCKS_PER_SEC;
  fprintf(stdout, "Time for setup: %f\n"
          "Time for execution: %f\n"
          "Width: %d\n"
          "Ordering plausible: %c",
           time_su, time_d, width_d,
           graph_ordering_plausible(g));
  graph_destroy(g);
  
  //benchmark(inputpath, stdout);
  
  
  return 0;
}

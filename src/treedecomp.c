#include "treedecomp.h"
#include "graph.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#define FILENAME_MAX_LENGTH 50

char *optarg;
int optind, opterr, optopt;

void print_file_header(FILE* fptr) {
  fprintf(fptr, "Filename,Width Min-Degree,Time Min-Degree,"
              "Width Min-Fill-in,Time Min-Fill-in,"
              "Width MCS,Time MCS\n");
}

int benchmark (char* name, FILE* inputfile, FILE* resultfile) {
  Graph g = graph_import(inputfile);
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
  int width_mcs = graph_order_mcs(g2);
  end = clock();
  float time_mcs = (end-start)/CLOCKS_PER_SEC;

  fprintf(resultfile, "%s,%d,%f,%d,%f,%d,%f\n",
           name, width_d, time_d, width_f, time_f,
            width_mcs, time_mcs);
  printf("Analyzed file %s\n", name);
  fflush(resultfile);
  fflush(stdout);
  graph_destroy(g);
  graph_destroy(g1);
  graph_destroy(g2);
  return 0;
}

typedef enum method {degree, cardinality, fillin} method;
const char* METHOD[] = {"Min-Degree", "Maximum-Cardinality-Search", "Min-Fill-in-edges"};
int
main (int argc, char **argv)
{
  char *inputpath = NULL;
  char usagestring[] = "usage: treedecomp [-h] [-D|C|F] [-s filepath] [-f folderpath]\n\n"
  "Calculate tree decomposition of a provided graph or graphs\n\n"
  "options:\n"
  "\t-h\tdisplay this message\n"
  "\t-s filepath\tdo a tree decomposition of a single graph provided by filepath\n"
  "\t-l listfile\tdo a benchmark of a list of graphs (results of size and time in results.csv, no actual decomposition for each graph is saved)\n"
  "\t-D\tuse the min-degree heuristic (only for single graph mode)\n"
  "\t-C\tuse the max-cardinality heuristic (only for single graph mode)\n"
  "\t-F\tuse the min-fill-in heuristic (only for single graph mode)\n";
  int listmode = 0;
  int index;
  int c;
  method mode = degree;

  opterr = 0;


  while ((c = getopt (argc, argv, "s:l:hDCF")) != -1)
    switch (c)
      {
      case 'l':
        if (inputpath != NULL) {
          fprintf(stdout, 
                  "Ignoring Single file as folder was provided\n");
        }
        inputpath = optarg;
        listmode = 1;
        break;
      case 's':
        if(listmode==1) {
          fprintf(stdout, 
                  "Ignoring Single file as folder was provided\n");
        } else inputpath = optarg;

        break;
      case 'h':
          fprintf(stdout, "%s\n", usagestring);
          return 0;
        break;
      case 'D':
        mode = degree;
        break;
      case 'F':
        mode = fillin;
        break;
      case 'C':
        mode = cardinality;
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

  for (index = optind; index < argc; index++)
    fprintf (stdout, "Non-option argument %s not supported\n", argv[index]);


  if (listmode == 1){
    /* Open input file list and create result file */
    FILE* results = fopen("results.csv", "a");
    if (results == NULL) {perror("Error opening results file"); exit(1);}
    /* Check if appending or need to print header */
    fseek (results, 0, SEEK_END);
    long size = ftell(results);
    if (0 == size) {
      print_file_header(results);
    }
    FILE* inputfiles = fopen(inputpath, "r");
    if (inputfiles == NULL) {perror("Error opening input file list file"); exit(1);}

    /* Iterate through the entries in input file list and do benchmark for each file */
    char filename[FILENAME_MAX_LENGTH];
    FILE* current;
    int failed = 0;
    while (fgets(filename, FILENAME_MAX_LENGTH, inputfiles)!=NULL)
    {
      /* trim leading newline*/
      filename[strcspn(filename, "\n")] = 0;
      current = fopen(filename, "r");
      if (current == NULL) {
        fprintf(stderr, "Error opening file %s: ", filename);
        perror("");
        errno = 0;
        failed++;
        continue;
      }
      char benchmark_fail = benchmark(filename, current, results);
      if (benchmark_fail) {
        fprintf(stderr, "Error processing input file %s\n", filename);
        failed++;
      }
      fclose(current);
    }
    /* Check if we reached end or there was an error */
    if (errno) perror("Error reading input file list: ");
    else {
      printf("Benchmark completed, there were %d failures\n", failed);
    }
    fclose(inputfiles);
    fclose(results);
    exit(0);
  }
  else
  {
    FILE *inputfile = fopen(inputpath, "r");
    if (inputfile == NULL) {perror("Error opening input file"); exit(1);}


    Graph g = graph_import(inputfile);
    if (g == NULL) {fprintf(stderr, "Error importing graph\n"); exit(1);}

    int width;
    switch (mode)
    {
    case degree:
      width = graph_order_degree(g);
      break;
    case fillin:
      width = graph_order_fillin(g);
      break;
    case cardinality:
      width = graph_order_mcs(g);
      break;

    default:
      break;
    }
    printf("File: %s\n", inputpath);
    printf("Heuristic: %s\n", METHOD[mode]);
    printf("Ordering: "); 
    graph_print_ordering(g, stdout);
    printf("\nWidth: %d\n", width);
    graph_destroy(g);
    fclose(inputfile);
  }
  

  //FILE* results = fopen("results.csv", "w");
  /* print_file_header(stdout);
  float start, end;
  start = clock();
  Graph g = graph_import(inputpath);
  if(g==NULL) return 1;
  end = clock();
  float time_su = (end-start)/CLOCKS_PER_SEC;
  
  start = clock();
  int width_d = graph_order_degree(g);
  end = clock();
  float time_d = (end - start)/CLOCKS_PER_SEC;
  fprintf(stdout, "Time for setup: %f\n"
          "Time for execution: %f\n"
          "Width: %d\n"
          "Ordering plausible: %d\n"
          "Vertices: %d, Edges: %d\n",
           time_su, time_d, width_d,
           graph_ordering_plausible(g),
           graph_vertex_count(g),
           graph_edge_count(g));
  
  graph_destroy(g);
   */
  
  return 0;
}

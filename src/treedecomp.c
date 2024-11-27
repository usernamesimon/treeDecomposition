#include "graph.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#define FILENAME_MAX_LENGTH 50

const char *STRATEGY[] = {"Unspecified", "Min-Degree", "Min-Fill-in-edges", "Maximum-Cardinality-Search"};

char *optarg;
int optind, opterr, optopt;

void print_file_header(FILE *fptr)
{
  fprintf(fptr, "Filename,Width Min-Degree,Time Min-Degree,"
                "Width Min-Fill-in,Time Min-Fill-in,"
                "Width MCS,Time MCS\n");
}

int benchmark(char *name, FILE *inputfile, FILE *resultfile)
{
  Graph g = graph_import(inputfile);
  if (g == NULL)
    return 1;
  Graph g1 = graph_copy(g);
  Graph g2 = graph_copy(g);
  // int n = graph_vertex_count(g);

  float start, end;
  start = clock();
  int width_d = graph_order_degree(g);
  end = clock();
  float time_d = (end - start) / CLOCKS_PER_SEC;

  start = clock();
  int width_f = graph_order_fillin(g1);
  end = clock();
  float time_f = (end - start) / CLOCKS_PER_SEC;

  start = clock();
  int width_mcs = graph_order_mcs(g2);
  end = clock();
  float time_mcs = (end - start) / CLOCKS_PER_SEC;

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

typedef enum mode
{
  undefined,
  eo,
  td,
  list
} mode;
int main(int argc, char **argv)
{
  char *inputpath = NULL;
  char usagestring[] = "usage: treedecomp [-h] [-D|C|F] [-o|t|l] filepath [eo_filepath]\n\n"
                       "Calculate tree decomposition of a provided graph or graphs\n\n"
                       "options:\n"
                       "\t-h\tdisplay this message\n"
                       "\t-o\tcreate an elimination ordering (EO) of a single graph provided by <filepath>\n"
                       "\t-t\tcreate a tree decomposition from a graph provided by <filepath> and an elimination ordering provided by <eo_filepath>\n"
                       "\t-l\tdo a benchmark of elimination orderings of a list of graphs declared in <filepath> (results of "
                       "size and time in results.csv, no actual ordering for each graph is saved)\n"
                       "\t-v\tuse verbose printing\n"
                       "\t-D\tuse the min-degree heuristic when creating an elimination ordering\n"
                       "\t-C\tuse the max-cardinality heuristic when creating an elimination ordering\n"
                       "\t-F\tuse the min-fill-in heuristic when creating an elimination ordering\n";
  int c;
  strategy heuristic = degree;
  mode mode = undefined;
  int verbose_printing = 0;

  opterr = 0;

  while ((c = getopt(argc, argv, "otlc:hvDCF")) != -1)
    switch (c)
    {
    case 'l':
      if (mode != undefined)
      {
        fprintf(stderr,
                "Error: Can only use one of these options [-o -t -l -c]\n");
        exit(1);
      }
      else
        mode = list;
      break;
    case 'o':
      if (mode != undefined)
      {
        fprintf(stderr,
                "Error: Can only use one of these options [-o -t -l -c]\n");
        exit(1);
      }
      else
        mode = eo;
      break;
    case 't':
      if (mode != undefined)
      {
        fprintf(stderr,
                "Error: Can only use one of these options [-o -t -l -c]\n");
        exit(1);
      }
      else
        mode = td;
      break;
    case 'h':
      fprintf(stdout, "%s\n", usagestring);
      exit(0);
      break;
    case 'D':
      heuristic = degree;
      break;
    case 'F':
      heuristic = fillin;
      break;
    case 'C':
      heuristic = mcs;
      break;
    case 'v':
      verbose_printing = 1;
      break;
    case '?':
      if (optopt == 'f')
        fprintf(stderr, "Option -%c requires an argument.\n", optopt);
      else if (isprint(optopt))
        fprintf(stderr, "Unknown option `-%c'.\n", optopt);
      else
        fprintf(stderr,
                "Unknown option character `\\x%x'.\n",
                optopt);
      exit(1);
    case ':':
      fprintf(stderr, "No argument provided, aborting...\n");
      exit(1);
    default:
      abort();
    }

  inputpath = argv[optind];

  /* --------- Benchmark mode ------------- */
  if (mode == list)
  {
    /* Open input file list and create result file */
    FILE *results = fopen("results.csv", "a");
    if (results == NULL)
    {
      perror("Error opening results file");
      exit(1);
    }
    /* Check if appending or need to print header */
    fseek(results, 0, SEEK_END);
    long size = ftell(results);
    if (0 == size)
    {
      print_file_header(results);
    }
    FILE *inputfiles = fopen(inputpath, "r");
    if (inputfiles == NULL)
    {
      perror("Error opening input file list file");
      fclose(results);
      exit(1);
    }

    /* Iterate through the entries in input file list and do benchmark for each file */
    char filename[FILENAME_MAX_LENGTH];
    FILE *current;
    int failed = 0;
    while (fgets(filename, FILENAME_MAX_LENGTH, inputfiles) != NULL)
    {
      /* trim leading newline*/
      filename[strcspn(filename, "\n")] = 0;
      current = fopen(filename, "r");
      if (current == NULL)
      {
        fprintf(stderr, "Error opening file %s: ", filename);
        perror("");
        errno = 0;
        failed++;
        continue;
      }
      char benchmark_fail = benchmark(filename, current, results);
      if (benchmark_fail)
      {
        fprintf(stderr, "Error processing input file %s\n", filename);
        failed++;
      }
      fclose(current);
    }
    /* Check if we reached end or there was an error */
    if (errno)
      perror("Error reading input file list: ");
    else
    {
      printf("Benchmark completed, there were %d failures\n", failed);
    }
    fclose(inputfiles);
    fclose(results);
    exit(0);
  }
  /* ----------- Analyze single graph -------------- */
  else if (mode == eo || mode == td)
  {
    FILE *inputfile = fopen(inputpath, "r");
    if (inputfile == NULL)
    {
      perror("Error opening input file");
      exit(1);
    }
    Graph g = graph_import(inputfile);
    if (g == NULL)
    {
      fprintf(stderr, "Error importing graph\n");
      fclose(inputfile);
      exit(1);
    }
    fclose(inputfile);

    /* ------- Elimination Ordering only ----------- */
    if (mode == eo)
    {
      float start, end, time_f;
      if(verbose_printing) start = clock();

      int width;
      switch (heuristic)
      {
      case degree:
        width = graph_order_degree(g);
        break;
      case fillin:
        width = graph_order_fillin(g);
        break;
      case mcs:
        width = graph_order_mcs(g);
        break;

      default:
        abort();
        break;
      }
      if (verbose_printing)
      {
        end = clock();
        time_f = (end - start) / CLOCKS_PER_SEC;
        printf("File: %s\n", inputpath);
        printf("Heuristic: %s\n", STRATEGY[heuristic]);
        printf("Ordering: ");
      }
      graph_print_ordering(g, stdout);
      printf("\n");
      if (verbose_printing)
      {
        printf("Width: %d\n", width);
        printf("Execution time of ordering: %f\n", time_f);
      }
    }
    /* ------- Tree decomposition conversion ------- */
    else
    {
      if (optind + 1 >= argc)
      {
        fprintf(stderr, "Error: You need to provide both a graph file and an elimination ordering file\n");
        graph_destroy(g);
        exit(1);
      }
      char *eo_filepath = argv[optind + 1];
      FILE *eo_file = fopen(eo_filepath, "r");
      if (eo_file == NULL)
      {
        perror("Error opening input file");
        graph_destroy(g);
        exit(1);
      }
      if(!graph_import_ordering(g, eo_file)) {
        fprintf(stderr, "Error importing elimination ordering");
        graph_destroy(g);
        fclose(eo_file);
        exit(1);
      };
      fclose(eo_file);

      if (verbose_printing)
      {
        printf("Graph file: %s\n", inputpath);
        printf("Elimination ordering file: %s\n", eo_filepath);
        printf("Ordering: ");
        graph_print_ordering(g, stdout);
        printf("\n");
      }
      graph_eo_to_treedecomp(g);
    }
    graph_destroy(g);
  }
  exit(0);
}

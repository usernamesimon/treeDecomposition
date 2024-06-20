#include "treedecomp.h"
#include "graph.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
char *optarg;
int optind, opterr, optopt;

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

  

  Graph g = graph_import(inputpath);
  if(g==NULL) return 1;
  graph_print(g, stdout);
  graph_add_edge(g, 7, 9);
  graph_print(g, stdout);
  graph_destroy(g);
  
  return 0;
}

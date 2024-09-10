
# Build instructions

You can try to use the VS Code configuration, otherwise use cmake.  
Something like  
`mkdir build`  
`cd build`  
`cmake -S ..`  
`make`  
should work.

# Generate graphs

Make sure you have the python package networkx installed.  
Use graphs.py to generate Erlos Renyi graphs with provided size and probability.  
`python graphs.py --help` for usage.  
Use the `-n` option to create multiple random graphs with same parameters.  
For a single graph use `-outfile` option to specify a filename, for multiple graphs use `-outfolder` to specify a folder to create the files in.  
When creating multiple graphs, the individual files will be named according to the scheme `ER_<# of vertices>_<edge probability>_<instance ID>.al`.

# Do tree decompositions

Use `build/treedecomp` or `.vscode/treedecomp` depending on how you built it to analyze graphs.

## Input format

Provide the graphs in networkx's adjacency list format (https://networkx.org/documentation/stable/reference/readwrite/adjlist.html)

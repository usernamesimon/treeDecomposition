Build instructions
You can try to use the VS Code configuration, otherwise use cmake.
Something like
mkdir build
cd build
cmake -S ..
should work.

Generate graphs
Make sure you have the python package networkx installed.
Use graphs.py to generate Erlos Renyi graphs with provided size and probability.
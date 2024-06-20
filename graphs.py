import networkx as nx
import argparse
import os

def write_graph(G, file, comments="#", delimiter=" ", encoding="utf-8"):
    """
    Write the graph G to a file.
    First line defines the number of vertices.
    Then the graph is written in multiline adjacency list format
    """
    import sys
    import time
    import networkx
   
    with open(file, "wb") as fd:
        fd.write(f"{comments} nodes {G.number_of_nodes()}\n".encode(encoding))
        for line in networkx.generate_adjlist(G, delimiter):
            line += "\n"
            fd.write(line.encode(encoding))


parser = argparse.ArgumentParser(description='Create Erdos Renyi graphs')
parser.add_argument('N', metavar='size', type=int, help='Size of the graph')
parser.add_argument('p', metavar='probability', type=float,
                     help='Edge probability value')
parser.add_argument("-n", metavar="multiplicity", type=int, default=1,
                     help="Number of graphs to generate")
parser.add_argument("-outfile", metavar="filepath", nargs="?",
                     default="ER.al", help="Filename for single Graph")
parser.add_argument("-outfolder", metavar="dir", 
                    help="Name a folder to create files in", default="")
args = parser.parse_args()


size = args.N
prob = args.p
#Which generator is faster depends on the size of p relative to N
#for small values of p fast_gnp_random_graph is faster
critP = 1.0*(size-1)/(2*size - 1)

if prob < critP:
    generator = nx.fast_gnp_random_graph
else:
    generator = nx.gnp_random_graph

folder = os.path.normpath(os.path.join(os.getcwd(), args.outfolder))
if not os.path.exists(folder):
    os.makedirs(folder)

if args.n > 1:    
    for i in range(args.n):
        G = generator(size, prob, directed=False)

        write_graph(G, os.path.join(folder,f"ER_{size}_{prob:.2f}_{i}.al"))
else:
    G = generator(size, prob, directed=False)
    write_graph(G, os.path.join(folder,args.outfile))




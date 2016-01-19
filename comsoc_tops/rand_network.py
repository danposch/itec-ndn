from igraph import *
import random

NODES = 20
CLIENTS = 12
SERVERS = 4
MIN_DELAY = 5
MAX_DELAY = 20
CONTENT_POPULARITY = "uniform"
RUNS = 40

BW = {}
BW["LowBW"] = [2000,3000]
BW["MediumBW"] = [3000,4000]
BW["HighBW"] = [4000,5000]

CONNECTIVITY = {}
CONNECTIVITY["LowCon"] = 0.10
CONNECTIVITY["MediumCon"] = 0.15
CONNECTIVITY["HighCon"] = 0.20

def genRandomNetwork(EMU_RUN_NUMBER):

	print "Generating Random Network:"

	random.seed(EMU_RUN_NUMBER)

	if CLIENTS + SERVERS > NODES:
		print "To much Clients/Servers for this Network! Exiting..."
		exit(-1)

	is_connected = False
	while not is_connected:
		g = Graph.Erdos_Renyi(n=NODES, p=EDGE_PROBABILITY, directed=False, loops=False)
		is_connected = g.is_connected()

	#g.to_directed() # not needed will be done by the parser...

	with open(fname ,"w") as f:

		f.write("#number of nodes\n")
		f.write(str(NODES)+"\n")

		f.write("#nodes setting (n1,n2,bandwidth in kbits a -> b, bandwidth in kbits a <- b, delay a -> b in ms, delay b -> a in ms)\n")

		for edge in g.es:
			f.write( str(edge.source) + "," + str(edge.target) + ","
				 + str(int(random.uniform(MIN_BANDWIDTH, MAX_BANDWIDTH))) + ","
				 + str(int(random.uniform(MIN_BANDWIDTH, MAX_BANDWIDTH))) + ","
				 + str(int(random.uniform(MIN_DELAY, MAX_DELAY))) + "," 
				 + str(int(random.uniform(MIN_DELAY, MAX_DELAY))) + "\n")

		f.write("#properties (Client, Server)\n")

		nodes = range(0,20)
		servers = []
		clients = []

		for i in range(0,SERVERS):
			index = random.randint(0, len(nodes)-1)
			servers.append(nodes.pop(index))

		for i in range(0,CLIENTS):
			index = random.randint(0, len(nodes)-1)
			clients.append(nodes.pop(index))

		for client in clients:

			if CONTENT_POPULARITY == "uniform":
				f.write(str(client) + "," + str(servers[int(round(random.uniform(0,len(servers)-1)))]) +"\n")
			elif CONTENT_POPULARITY == "zipf":
				f.write(str(client) + "," + str(servers[int(round(random.uniform(0,len(servers)-1)))]) +"\n")
			else:
				print "Invalid Content Popularity! Exiting.."
				exit(-1)

		f.write("#eof //do not delete this")
		f.close()

	#plot(g)

	return fname

for run in range(0,RUNS):
		for bw in BW:
			MIN_BANDWIDTH = BW[bw][0]
			MAX_BANDWIDTH = BW[bw][1]
			for con in CONNECTIVITY:
				EDGE_PROBABILITY =  CONNECTIVITY[con]
	
				fname = str(bw)+"_"+str(con)+"_"+str(run)+".top"
				print fname
				genRandomNetwork(run)


#! /usr/bin/python

import time
import numpy
import os
import glob
import collections
import shutil
import re
import subprocess
from subprocess import call
import threading
import time
import operator
import random

import consumer_stats as consumer_stats

curActiveThreads = 0
invalid_runs = 0

class Thread(threading.Thread):
    # init
  def __init__(self,job_number, sys_cal, callback_method, src ,dst):
		super(Thread,self).__init__()
		self.sysCall = sys_cal
		self.jobNumber = job_number
		self.callback = callback_method
		self.src = src
		self.dst = dst

  # overwriting run method of threading.Thread (do not call this method, call thread.start() )
  def run(self):

		if not os.path.exists(self.src+"/traces"):
			os.makedirs(self.src+"/traces")

		fpOut = open("t_" + str(self.jobNumber) + ".stdout.txt", "w")

		# start subprocess
		proc = subprocess.Popen(self.sysCall,stdout=fpOut, cwd=self.src)
		proc.communicate() # wait until finished

		# sleep 0.5 seconds to be sure the OS really has finished the process
		time.sleep(0.5)

		fpOut.close()
		os.remove("t_" + str(self.jobNumber) + ".stdout.txt")

		# callback
		print "threadFinished(job_" + str(self.jobNumber) + ")"
		self.callback(self.jobNumber,self.src,self.dst, proc.returncode)

def threadFinished(job_number,src,dst,returncode):
	#compute statistics

	global curActiveThreads, invalid_runs

	if(returncode != 0):
		invalid_runs += 1
		print "Error in job_" + str(job_number) +". Simulation incomplete!"
	else:
		print "computeStats(job_" + str(job_number) + ")"
		try:
			print src
			#calculate_average.computeStats(src+"/traces/")
			consumer_stats.generateStatsPerSimulation(src)
		except Exception:
			invalid_runs += 1
			pass

	#copy results
	files = glob.glob(src + "/traces/*STATS*.txt")
        files.extend(glob.glob(src + "/traces/*cs-trace*.txt"))
	#files = glob.glob(src + "/traces/*.txt")

	if not os.path.exists(dst):
		os.makedirs(dst)

	for f in files:
		shutil.move(f, dst+"/"+os.path.basename(f))

	#print "DELTE FOLDER " + src
	shutil.rmtree(src)

	print "statsCollected(job_" + str(job_number) + ")"

	curActiveThreads -= 1

def	order_results(path):
	results = {}

	for root, dirs, files in os.walk(path):
		for subdir in dirs:
		
			if "output_run" in subdir:
				continue

			#print root+subdir

			files = glob.glob(root+subdir + "/*/*STATS*.txt" )
		
			avg_ratio = 0.0
			file_count = 0
			cache_hit_ratio = 0.0

			for file in files:

				#print file
				f = open(file, "r")
				for line in f:
					if(line.startswith("Ratio:")):
						avg_ratio += float(line[len("Ratio:"):])
						
					if(line.startswith("Cache_Hit_Ratio:")):
						cache_hit_ratio += float(line[len("Cache_Hit_Ratio:"):])
					
				file_count +=1
			

			if(file_count > 0):
	 			avg_ratio /= file_count
				cache_hit_ratio /= file_count
	
			#print avg_ratio
			results.update({"AVG_RATIO:"+ subdir : avg_ratio})
			results.update({"CACHE_HIT_RATIO:"+ subdir : cache_hit_ratio})

	sorted_results = reversed(sorted(results.items(), key=operator.itemgetter(1)))
	f = open(path + "/result.txt", "w")
	for entry in sorted_results:
		f.write(entry[0] + ":" + str(entry[1]) + "\n")
		
###NOTE Start this script FROM itec-scenarios MAIN-FOLDER!!!

SIMULATION_DIR=os.getcwd()

THREADS = 16
SIMULATION_RUNS = 30

SIMULATION_OUTPUT = SIMULATION_DIR 
SIMULATION_OUTPUT += "/comsoc_output/"

#brite config file
scenario="comsoc"

#comsocConfigFolder = "/local/users/ndnsim2/ndnSIM/itec-ndn/comsoc_tops"
comsocConfigFolder = "/home/dposch/ndnSIM/itec-ndn/comsoc_tops"
topFiles = glob.glob(comsocConfigFolder+"/*.top")

singleRoute="--route=single"
allRoute="--route=all"

bestRoute="--fw-strategy=bestRoute " + allRoute
ncc="--fw-strategy=ncc " + allRoute
broadcast="--fw-strategy=broadcast " + allRoute
saf="--fw-strategy=saf " + allRoute
omccrf="--fw-strategy=omccrf " + allRoute
#oracle="--fw-strategy=oracle " + allRoute

forwardingStrategies = [bestRoute, ncc, broadcast, saf, omccrf]
connectivities = ["HighCon", "LowCon", "MediumCon"]
capacities = ["HighBW", "LowBW", "MediumBW"]

SCENARIOS = {}

settings_counter = 0
for strategy in forwardingStrategies:
	for connectivity in connectivities:
		for bw in capacities:

			name = ""

			if("fw-strategy=bestRoute" in strategy):
				name += "BestRoute"
			elif("fw-strategy=smartflooding" in strategy):
				name += "SmartFlooding"
			elif("fw-strategy=saf" in strategy):
				name += "SAF"
			elif("fw-strategy=broadcast" in strategy):
				name += "Broadcast"
			elif("fw-strategy=ncc" in strategy):
				name += "NCC"
			elif("fw-strategy=omccrf" in strategy):
				name += "OMCCRF"
			else:
				name += "UnknownStrategy"

			name += "_"+connectivity+"_"+bw
		SCENARIOS.update({name : { "executeable": scenario, "params": [strategy], "bandwidth": bw, "connectivity": connectivity }})			
		settings_counter += 1

#build project before
call([SIMULATION_DIR + "/waf"])

###script start
print "\nCurring working dir = " + SIMULATION_DIR + "\n"

print str(settings_counter) + " different settings selected"

time.sleep(5)

###script start
job_number = 0

for scenarioName in SCENARIOS.keys():
	executeable = SCENARIOS[scenarioName]['executeable']

	connectivity = SCENARIOS[scenarioName]['connectivity']
	bandwidth = SCENARIOS[scenarioName]['bandwidth']

	run_tops = [ k for k in topFiles if bandwidth in k and connectivity in k]

	#print scenarioName
	#print connectivity
	#print bandwidth
	#print run_tops
	
	executeable = "build/" + executeable
	print "------------------------------------------------------------------------"
	print "Starting", len(run_tops) , "simulations of", scenarioName

	for top in run_tops:
  	# See if we are using all available threads
		while curActiveThreads >= THREADS:
			time.sleep(1) # wait 1 second

		print "----------"
		print "Simulation run " + top + " in progress..." 
		sysCall = [SIMULATION_DIR+"/" + executeable] +  SCENARIOS[scenarioName]['params'] + ["--outputFolder=traces"] ## working folder of subprocess is determined by Thread
		print sysCall

		dst = SIMULATION_OUTPUT+scenarioName + "/output_run"+str(re.findall(r'\d+', top)[-1])
		src = SIMULATION_OUTPUT+"../ramdisk/tmp_folder_" + str(job_number)

	   # start thread, get callback method to be called when thread is done
		thread = Thread(job_number, sysCall, threadFinished, src, dst)

		if(os.path.exists(dst)):
			print str(dst) + " exists.. SKIPPING!"
			job_number += 1
			continue
		thread.start()

		job_number += 1
		curActiveThreads += 1	
# end for

while curActiveThreads != 0:
    time.sleep(15)
    print "Active Threads: " + str(curActiveThreads)

order_results(SIMULATION_OUTPUT)

print ""
print "We had " + str(invalid_runs) + " invalid runs"
print "Finished."

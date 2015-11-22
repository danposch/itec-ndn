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
import dashplayer_stats as dashplayer_stats
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
			dashplayer_stats.computeStats(src+"/traces/")
			#consumer_stats.generateStatsPerSimulation(src)
		except Exception:
			invalid_runs += 1
			pass

	#copy results
	files = glob.glob(src + "/traces/*STATS*.txt")
	files.extend(glob.glob(src + "/traces/*dashplayer-trace*.txt"))
	files.extend(glob.glob(src + "/traces/*cs-trace*.txt"))
	#files = glob.glob(src + "/traces/*.txt") # copy everthing

	if not os.path.exists(dst):
		os.makedirs(dst)

	for f in files:
		shutil.move(f, dst+"/"+os.path.basename(f))

	#print "DELTE FOLDER " + src
	shutil.rmtree(src)

	print "statsCollected(job_" + str(job_number) + ")"

	curActiveThreads -= 1
		
def getScenarioName(strategy,linkfailure,adap):
	
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
	elif("fw-strategy=oracle" in strategy):
                name += "NRR"
	elif("fw-strategy=omccrf" in strategy):
                name += "OMCCRF"
	else:
		name += "UnknownStrategy"

	if("route=single" in strategy):
		name += "_SingleRoute"
	elif("route=all" in strategy):
		name += "_AllRoutes"
	else:
		name += "_UnkownRoute"

	if("--adaptation=buffer" in adap):
		name += "_BufferAdap"
	elif("--adaptation=rate" in adap):
		name += "_RateAdap"
	elif("--adaptation=nologic" in adap):
		name += "_NoAdap"
	else:
		name += "_UnknownAdaptation"

	failure = re.findall('\d+', linkfailure)

	if(len(failure) == 1):
		name += "_LinkFailure_"+failure[0]
	else:
		name +="_UnkownLinkFailurey"

	return name

def	order_dash_results(path):
	results_level = {}
	results_stalls = {}
	results_bitrate = {}
	results_cachehits = {}

	for root, dirs, files in os.walk(path):
		for subdir in dirs:
		
			if "output_run" in subdir:
				continue

			#print root+subdir

			files = glob.glob(root+subdir + "/*/DASH-STATS.txt" ) #match DASH-STATS exectly as there is STATS too
		
			avg_layer = 0.0
			avg_stalls_msec = 0.0
			avg_video_bitrate = 0.0
			avg_cache_hitratio = 0.0
			file_count = 0

			for file in files:

				#print file
				f = open(file, "r")
				for line in f:
					if(line.startswith("AVG Level per Client:")):
						avg_layer += float(line[len("AVG Level per Client:"):])
						
					if(line.startswith("AVG Stalls (msec):")):
						avg_stalls_msec += float(line[len("AVG Stalls (msec):"):])

					if(line.startswith("AVG Video Bitrate (bit/s):")):
						avg_video_bitrate += float(line[len("AVG Video Bitrate (bit/s):"):])

					if(line.startswith("Cache_Hit_Ratio:")):
						avg_cache_hitratio += float(line[len("Cache_Hit_Ratio:"):])
					
				file_count +=1
			

			if(file_count > 0):
	 			avg_layer /= file_count
				avg_stalls_msec /= file_count
				avg_video_bitrate /= file_count
				avg_cache_hitratio /= file_count
	
			#print avg_ratio
			results_level.update({"AVG Level per Client:"+ subdir : avg_layer})
			results_stalls.update({"AVG Stalls (msec):"+ subdir : avg_stalls_msec})
			results_bitrate.update({"AVG Video Bitrate (bit/s):"+ subdir : avg_video_bitrate})
			results_cachehits.update({"Cache_Hit_Ratio:"+ subdir : avg_cache_hitratio})

	result_list = [results_level, results_stalls, results_bitrate, results_cachehits]

	f = open(path + "/dash_result.txt", "w")
	for results in result_list: 
		sorted_results = reversed(sorted(results.items(), key=operator.itemgetter(1)))
		for entry in sorted_results:
			f.write(entry[0] + ": " + str(entry[1]) + "\n")
		f.write("\n\n")

'''def	order_consumer_results(path):
	results = {}

	for root, dirs, files in os.walk(path):
		for subdir in dirs:
		
			if "output_run" in subdir:
				continue

			#print root+subdir

			files = glob.glob(root+subdir + "/*/STATS.txt" ) # match stats exectly as there is DASH-STATS too
		
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
'''	

###NOTE Start this script FROM itec-ndn MAIN-FOLDER!!!

SIMULATION_DIR=os.getcwd()

THREADS = 5 
SIMULATION_RUNS = 25 
SIMULATION_OUTPUT = SIMULATION_DIR + "/output/"
CLIENT_START_DELAY = "--delay-model=exponential"

#order_dash_results(SIMULATION_OUTPUT)
#exit(0)

#brite config file
scenario="dash_fixed_top"
britePath="/home/danposch/ndnSIM/itec-ndn/"

briteConfig="--briteConfFile="+britePath+"brite_configs/fixed.conf"

singleRoute="--route=single"
allRoute="--route=all"

bestRoute="--fw-strategy=bestRoute " + allRoute
ncc="--fw-strategy=ncc " + allRoute
broadcast="--fw-strategy=broadcast " + allRoute
saf="--fw-strategy=saf " + allRoute
oracle="--fw-strategy=oracle " + allRoute
omccrf="--fw-strategy=omccrf " + allRoute

forwardingStrategies = [bestRoute, broadcast, saf, ncc, oracle]
#forwardingStrategies = [saf,bestRoute]
#forwardingStrategies = [oracle, omccrf]

buf = "--adaptation=buffer"
rate = "--adaptation=rate"
no = "--adaptation=nologic"

adaptationStrategies = [buf, rate, no]
#adaptationStrategies = [buf]

#linkFailures = ["--linkFailures=0", "--linkFailures=15", "--linkFailures=30", "--linkFailures=50", "--linkFailures=100"]
#linkFailures = ["--linkFailures=30", "--linkFailures=50", "--linkFailures=100"]
linkFailures = ["--linkFailures=0"]

SCENARIOS = {}

settings_counter = 0
for strategy in forwardingStrategies:
	for adap in adaptationStrategies:
		for failures in linkFailures:
			name = getScenarioName(strategy,failures,adap) 
			SCENARIOS.update({name : { "executeable": scenario, "numRuns": SIMULATION_RUNS, "params": [strategy, adap, failures, briteConfig, CLIENT_START_DELAY] }})			
		settings_counter += 1

#build project before
call([SIMULATION_DIR + "/waf"])

###script start
print "\nCurring working dir = " + SIMULATION_DIR + "\n"

print str(settings_counter) + " different settings selected"
print str(SIMULATION_RUNS) + " runs per setting"
print "Total runs: " + str(settings_counter*SIMULATION_RUNS)

time.sleep(5)

###script start
print "\nCurring working dir = " + SIMULATION_DIR + "\n"

job_number = 0

for scenarioName in SCENARIOS.keys():
	runs = SCENARIOS[scenarioName]['numRuns']
	executeable = SCENARIOS[scenarioName]['executeable']
	
	executeable = "build/" + executeable
	print "------------------------------------------------------------------------"
	print "Starting", runs , "simulations of", scenarioName

  #REMOVE JUST FOR SMALL TEST
  #if(job_number>4):
    #continue
	
	for i in range(0, runs):
  	# See if we are using all available threads
		while curActiveThreads >= THREADS:
			time.sleep(1) # wait 1 second

		print "----------"
		print "Simulation run " + str(i) + " in progress..." 
		sysCall = [SIMULATION_DIR+"/" + executeable] +  SCENARIOS[scenarioName]['params'] + ["--RngRun=" + str(1+i)] + ["--outputFolder=traces"] ## working folder of subprocess is determined by Thread
		print sysCall

		dst = SIMULATION_OUTPUT+scenarioName + "/output_run"+str(i)
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

order_dash_results(SIMULATION_OUTPUT) #dash_results
#order_consumer_results(SIMULATION_OUTPUT) # basic consumer results

print ""
print "We had " + str(invalid_runs) + " invalid runs"
print "Finished."

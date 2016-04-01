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

import consumer_stats as cs
import dashplayer_stats as ds

curActiveThreads = 0
invalid_runs = 0

def generateStats(rootdir):

	#calc voip stats
	voip_res = calcSimpleStats(rootdir, "voipstreamer")
	voip_res["Avg.DelayS"] -= (LOOKAHEAD_VOIP_DELAY / 1000.0) #removes the lookahead time for voip
	voip_res["Ppl"] = (1-(voip_res["SatisfiedInterests"]/voip_res["TotalInterests"])) *100 #Ppl is actually a probabilty * 100 WTF?
	voip_res["BurstR"] = calcVOIPBurstR(rootdir, voip_res["Ppl"])
	voip_res["AvgMos"] = calcVoIPMOS(voip_res)
	#print voip_res

	#calc video stats
	video_res = calcVideoStats(rootdir)
	#print video_res
	
	#calc backgroundtraffic stats
	data_res = calcSimpleStats(rootdir, "datastreamer")
	#print data_res
	
	#calc costs
	total_costs, avg_costs, raw_kilo_bytes_costs = calcCosts(rootdir)
	#print costs

	#write file
	output_file = open(rootdir+"/STATS.txt", "w")

	output_file.write("VoIP_Satisfaction_Ratio:" + str(voip_res["SatisfiedInterests"]/voip_res["TotalInterests"])+"\n")
	output_file.write("VoIP_Satisfaction_Delay:" + str(voip_res["Avg.DelayS"])+"s"+"\n")
	output_file.write("VoIP_Rtx:" + str(voip_res["Avg.Rtx"])+"\n")
	output_file.write("VoIP_HopCount:" + str(voip_res["Avg.HopCount"])+"\n")
	output_file.write("VoIP_MOS:" + str(voip_res["AvgMos"])+"\n")

	output_file.write("\n")
	output_file.write("Data_Satisfaction_Ratio:" + str(data_res["SatisfiedInterests"]/data_res["TotalInterests"])+"\n")
	output_file.write("Data_Satisfaction_Delay:" + str(data_res["Avg.DelayS"])+"s"+"\n")
	output_file.write("Data_Rtx:" + str(data_res["Avg.Rtx"])+"\n")
	output_file.write("Data_HopCount:" + str(data_res["Avg.HopCount"])+"\n")
	output_file.write("\n")
	output_file.write("Video_Representation:" +str(video_res["Avg.Representation"]) + "\n")
	output_file.write("Video_StallingMS:" +str(video_res["Avg.StallingMS"]) + "\n")
	output_file.write("Video_SegmentBitrate:" +str(video_res["Avg.SegmentBitrate"]) + "\n")
	output_file.write("Video_Switches:" +str(video_res["Avg.Switches"]) + "\n")
	output_file.write("Avg.QoE.VideoQuality:" +str(video_res["Avg.QoE.VideoQuality"]) + "\n")
	output_file.write("Avg.QoE.QualityVariations:" +str(video_res["Avg.QoE.QualityVariations"]) + "\n")
	output_file.write("Avg.QoE.StallingTime:" +str(video_res["Avg.QoE.StallingTime"]) + "\n")
	output_file.write("\n")
	output_file.write("Total_Costs:" + str(total_costs) + "\n")
	output_file.write("Avg_Costs:" + str(avg_costs) + "\n")
	output_file.write("Raw_Kilobytes_Costs:" + str(raw_kilo_bytes_costs) + "\n")

	output_file.close()

def calcVOIPBurstR(rootdir, Ppl):

	avg_from_no_loss_to_loss = 0.0
	processed_files = 0.0

	for root, dirs, files in os.walk(rootdir):
		for f in files:
			if "voipstreamer-burst-trace" in f:

				last = 0 #no loss
				loss_after_no_loss = 0.0
				no_loss_after_no_loss = 0.0

				fp = open(rootdir+"/"+f,"r")
				for line in fp:
					if("seq_nr, lost" in line): #skip header
						continue

					l = line.split(',')
					
					if last == 0:
						if("0" in l[1]):
							no_loss_after_no_loss += 1
							last = 0
						else:
							loss_after_no_loss += 1
							last = 1
					elif last == 1:
						if("0" in l[1]):
							last = 0

				fp.close();

				from_no_loss_to_loss = loss_after_no_loss / (loss_after_no_loss + no_loss_after_no_loss)
				avg_from_no_loss_to_loss += from_no_loss_to_loss
				processed_files += 1
				
	avg_from_no_loss_to_loss/=processed_files



	avg_loss_to_no_loss = 0.0
	processed_files = 0.0
	for root, dirs, files in os.walk(rootdir):
		for f in files:
			if "voipstreamer-burst-trace" in f:

				last = 0 #no loss
				no_loss_after_loss = 0.0
				loss_after_loss = 0.0

				fp = open(rootdir+"/"+f,"r")
				for line in fp:
					if("seq_nr, lost" in line): #skip header
						continue

					l = line.split(',')
					
					if last == 1:
						if("1" in l[1]):
							loss_after_loss += 1
							last = 1
						else:
							no_loss_after_loss += 1
							last = 0
					elif last == 0:
						if("1" in l[1]):
							last = 1

				fp.close();

				from_loss_to_no_loss = no_loss_after_loss / (no_loss_after_loss +loss_after_loss)
				avg_loss_to_no_loss += from_loss_to_no_loss
				processed_files += 1
				
	avg_loss_to_no_loss/=processed_files

	print "Ppl = " + str(Ppl)
	print "p = " + str(avg_from_no_loss_to_loss)
	print "q = " + str(avg_loss_to_no_loss)

	BurstR = (Ppl/100.0)/avg_from_no_loss_to_loss
	print "BurstR = " + str(BurstR)
	BurstR = 1/(avg_from_no_loss_to_loss+avg_loss_to_no_loss)
	print "BurstR = " + str(BurstR)

	return BurstR

def calcVoIPMOS(voip_res):

	print "calcVoIPMOS"

	#calc Id
	d = FIXED_JITTER_BUFFER + voip_res["Avg.DelayS"]*1000.0 + CODEC_DELAY

	print "d = " + str(d)

	if(d - 177.3 < 0):
		H = 0
	else:
		H = 1

	Id = 0.024*d+0.11*(d-177.3)*H
	print "Id = " + str(Id)

	#calc Ieeff

	Ppl = voip_res["Ppl"]
	#Bpl = 10 #for G.711
	Bpl =  34 #for G.711 with PCL
	Ie = 0.0
	BurstR = voip_res["BurstR"]
	Ieeff = Ie + (95.0 - Ie) * (Ppl / ( (Ppl / BurstR) + Bpl))

	print "Ieeff = " +str(Ieeff)

	#calc R
	R = 93.2 - Id - Ieeff

	print "R = " + str(R)

	if(R <= 0):
		MOS = 1
	elif(R <= 100):
		MOS = 1+0.035*R+R*(R-60)*(100-R)*7*0.000001
	else:
		MOS = 4.5

	print "MOS = " + str(MOS) +"\n"
	return MOS

def calcCosts(rootdir):

	cost_function = {}
	cost_function["257"] = 3
	cost_function["258"] = 2
	cost_function["259"] = 1

	costs = 0.0
	total_interests = 0.0
	raw_kilo_bytes_costs  = 0.0

	FACE_INDEX = 2 
	TYPE_INDEX = 4
	PACKET_NR_INDEX = 7
	KILOBYTES_RAW_INDEX = 8

	for root, dirs, files in os.walk(rootdir):
		for f in files:
			if "saf-router" in f:			
				
				fp = open(rootdir+"/"+f,"r")
				for line in fp:
					l = line.split('\t')

					if(len(l) < PACKET_NR_INDEX+1):
						continue

					if l[FACE_INDEX] in cost_function.keys():
						if "OutInterests" in l[TYPE_INDEX]:
							costs += cost_function[l[FACE_INDEX]] * float(l[PACKET_NR_INDEX])
							total_interests += float(l[PACKET_NR_INDEX])

						#gather raw bytes
						if "OutInterests" in l[TYPE_INDEX]:
							raw_kilo_bytes_costs  += cost_function[l[FACE_INDEX]] * float(l[KILOBYTES_RAW_INDEX])
						if "InData" in l[TYPE_INDEX]:
							raw_kilo_bytes_costs  += cost_function[l[FACE_INDEX]] * float(l[KILOBYTES_RAW_INDEX])

				break

	avg_costs = costs / total_interests	
	return costs, avg_costs, raw_kilo_bytes_costs 

def calcVideoStats(rootdir):

	avg_number_switches = 0.0
	avg_stalling_duration = 0.0
	avg_segment_bitrate = 0.0
	avg_representation = 0.0
	clients = 0

	avg_qoe_variations = 0.0
	avg_qoe_stalling = 0.0
	
	for root, dirs, files in os.walk(rootdir):
		for f in files:
			if "videostreamer-dashplayer" in f:			
				clients += 1
				ds_stats = ds.process_dash_trace(rootdir+"/"+f)
				#print ds_stats

				#see dashplayer_stats for magic numbers
				avg_number_switches += ds_stats[99]
				avg_stalling_duration += ds_stats[6]
				avg_segment_bitrate += ds_stats[5]
				avg_representation += ds_stats[4]
		
				avg_qoe_variations += ds_stats[98]
				avg_qoe_stalling += ds_stats[97]
					
	avg_number_switches /= clients
	avg_stalling_duration /= clients
	avg_segment_bitrate /= clients
	avg_representation /= clients
	avg_qoe_variations /= clients
	avg_qoe_stalling /= clients

	result = {}
	result["Avg.Switches"] = avg_number_switches
	result["Avg.StallingMS"] = avg_stalling_duration
	result["Avg.SegmentBitrate"] = avg_segment_bitrate
	result["Avg.Representation"] = avg_representation

	#QoE values according to: A Control-Theoretic Approach for Dynamic Adaptive Video Streaming over HTTP
	result["Avg.QoE.VideoQuality"] = avg_segment_bitrate
	result["Avg.QoE.QualityVariations"] = avg_qoe_variations
	result["Avg.QoE.StallingTime"] = avg_qoe_stalling
	

	return result

def calcSimpleStats(rootdir, filter_str):

	total_number_of_requests = 0.0
	total_number_of_statisfied_requests = 0.0

	avg_number_of_hops = 0.0
	avg_number_of_rtx = 0.0
	avg_delay_of_request = 0.0

	for root, dirs, files in os.walk(rootdir):
		for f in files:
			if filter_str+"-aggregate" in f:			
				ag_stats = cs.process_aggregate_trace(rootdir+"/"+f)
				#print ag_stats

				for key in ag_stats:
					total_number_of_requests += ag_stats[key]['InInterests']
					total_number_of_statisfied_requests += ag_stats[key]['InSatisfiedInterests']
		
			if filter_str+"-app-delays" in f:
				app_stats = cs.process_app_delay_trace(rootdir+"/"+f)
				#print app_stats

				for key in app_stats:
					avg_delay_of_request += app_stats[key]['DelayS']
					avg_number_of_hops += app_stats[key]['HopCount']
					avg_number_of_rtx += app_stats[key]['RtxCount']

	if total_number_of_statisfied_requests > 0:
		avg_number_of_hops /= total_number_of_statisfied_requests
		avg_number_of_rtx /= total_number_of_statisfied_requests
		avg_delay_of_request /= total_number_of_statisfied_requests
	else:
		avg_delay_of_request = 100.0 #well define some very high delay if nothing arrived.. (e.g. 100 sec)

	result = {}
 	result["TotalInterests"] = total_number_of_requests
 	result["SatisfiedInterests"] = total_number_of_statisfied_requests
 	result["Avg.HopCount"]= avg_number_of_hops
 	result["Avg.Rtx"]= avg_number_of_rtx - 1 #-1 as only rtx if x > 1
	result["Avg.DelayS"]=avg_delay_of_request

	return result 			

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
			#print src
			generateStats(src+"/traces/")
	
		except Exception:
			invalid_runs += 1
			pass

	#copy results
	#files = glob.glob(src + "/traces/*STATS*.txt")
  #files.extend(glob.glob(src + "/traces/*cs-trace*.txt"))
	files = glob.glob(src + "/traces/*.txt") #copy all.

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
		
def getScenarioName(strategy):
	
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
	elif("fw-strategy=oracle" in strategy):
		name += "NRR"
	elif("fw-strategy=ompif" in strategy):
		name += "OMPIF"
	else:
		name += "UnknownStrategy"


	return name
	

###NOTE Start this script FROM itec-scenarios MAIN-FOLDER!!!
SIMULATION_DIR=os.getcwd()

THREADS = 3
SIMULATION_RUNS = 20

SIMULATION_OUTPUT = SIMULATION_DIR 
SIMULATION_OUTPUT += "/output_saf/"
LOOKAHEAD_VOIP_DELAY = 250.0 #ms
FIXED_JITTER_BUFFER = 50.0 #ms
CODEC_DELAY = 0.25 #ms

#SIMULATION_OUTPUT = "/media/dposch/Volume/sims/computer_communication_review/output/"
'''for root, dirs, files in os.walk(SIMULATION_OUTPUT):
		for d in dirs:
			if "output_run" not in d:
				for r, folders, files in os.walk(SIMULATION_OUTPUT+d):
					for folder in folders:
						if "output_run" in folder:
							print SIMULATION_OUTPUT+d+"/"+folder
							generateStats(SIMULATION_OUTPUT+d+"/"+folder)
exit(0)'''


#brite config file
scenario="saf_scenario"

#britePath="/local/users/ndnsim2/ndnSIM/itec-ndn/"
itecNDNPath="/home/dposch/ndnSIM/itec-ndn/"

topology = "--topology="+itecNDNPath+"topologies/saf_scenario.top"

bestRoute="--fw-strategy=bestRoute"
ncc="--fw-strategy=ncc"
broadcast="--fw-strategy=broadcast"
saf="--fw-strategy=saf"
omccrf="--fw-strategy=omccrf"
oracle="--fw-strategy=oracle"
ompif="--fw-strategy=ompif"

#forwardingStrategies = [bestRoute, ncc, broadcast, saf, oracle, omccrf, ompif]
#forwardingStrategies = [saf, ompif, ncc, omccrf, broadcast, bestRoute, oracle]
forwardingStrategies = [saf]

SCENARIOS = {}

for strategy in forwardingStrategies:
	name = getScenarioName(strategy) 
	SCENARIOS.update({name : { "executeable": scenario, "numRuns": SIMULATION_RUNS, "params": [topology, strategy] }})			

#build project before
call([SIMULATION_DIR + "/waf"])

###script start
print "\nCurring working dir = " + SIMULATION_DIR + "\n"
print "OutputFolder=" + SIMULATION_OUTPUT

print str(SIMULATION_RUNS) + " runs per setting"

time.sleep(3)

###script start
print "\nCurring working dir = " + SIMULATION_DIR + "\n"

job_number = 0

for scenarioName in SCENARIOS.keys():
	runs = SCENARIOS[scenarioName]['numRuns']
	executeable = SCENARIOS[scenarioName]['executeable']
	
	executeable = "build/" + executeable
	print "------------------------------------------------------------------------"
	print "Starting", runs , "simulations of", scenarioName
	
	for i in range(0, runs):
  	# See if we are using all available threads
		while curActiveThreads >= THREADS:
			time.sleep(1) # wait 1 second

		print "----------"
		print "Simulation run " + str(i) + " in progress..." 
		sysCall = [SIMULATION_DIR+"/" + executeable] +  SCENARIOS[scenarioName]['params'] + ["--RngRun=" + str(i)] + ["--outputFolder=traces"] ## working folder of subprocess is determined by Thread
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

#order_results(SIMULATION_OUTPUT)

print ""
print "We had " + str(invalid_runs) + " invalid runs"
print "Finished."

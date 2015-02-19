#! /usr/bin/python

import time
import numpy
import os
import glob
import sys
import collections

def generateStatsPerSimulation(rootdir):

	#print "RootDir=" + rootdir

	sim_total_number_of_requests = 0.0
	sim_total_number_of_statisfied_requests = 0.0
	sim_avg_number_of_hops = 0.0
	sim_avg_number_of_rtx = 0.0
	sim_avg_delay_of_request = 0.0

	dir_count = 0

	for root, dirs, files in os.walk(rootdir):
		
		for subdir in dirs:

			dir_count +=1

			#print "processing dir: " + subdir

			files = glob.glob(root+"/"+subdir + "/*trace*.txt" );

			run_total_number_of_requests = 0.0
			run_total_number_of_statisfied_requests = 0.0
			run_avg_number_of_hops = 0.0
			run_avg_number_of_rtx = 0.0
			run_avg_delay_of_request = 0.0
		
			for file in files:
			
				if "aggregate-trace" in file:
					ag_stats = process_aggregate_trace(file)

					for key in ag_stats:
						run_total_number_of_requests += ag_stats[key]['InInterests']
						run_total_number_of_statisfied_requests += ag_stats[key]['OutData']

				if "app-delays-trace" in file:
					app_stats = process_app_delay_trace(file)

					for key in app_stats:
						run_avg_delay_of_request += app_stats[key]['DelayS']
						run_avg_number_of_hops += app_stats[key]['HopCount']
						run_avg_number_of_rtx += app_stats[key]['RtxCount']

			run_avg_number_of_hops /= run_total_number_of_statisfied_requests
			run_avg_number_of_rtx /= run_total_number_of_statisfied_requests
			run_avg_delay_of_request /= run_total_number_of_statisfied_requests

			output_file = open(root+"/"+subdir +"/STATS.txt", "w")

			output_file.write("Total Requests:" + str(run_total_number_of_requests) +"\n")
			output_file.write( "Statisfied Requests:" + str(run_total_number_of_statisfied_requests) + "\n")
			output_file.write("Ratio:" + str(run_total_number_of_statisfied_requests/run_total_number_of_requests)+"\n")
			output_file.write( "Avg Delay[s]:" + str(run_avg_delay_of_request) + "[sec]"+"\n")
			output_file.write( "Avg Hops:" + str(run_avg_number_of_hops)+"\n")
			output_file.write( "Avg Rtx:" + str(run_avg_number_of_rtx - 1)+"\n")

			sim_total_number_of_requests += run_total_number_of_requests
			sim_total_number_of_statisfied_requests += run_total_number_of_statisfied_requests
		
			sim_avg_number_of_hops += run_avg_number_of_hops
			sim_avg_number_of_rtx += run_avg_number_of_rtx
			sim_avg_delay_of_request += run_avg_delay_of_request

	sim_avg_number_of_hops /= dir_count
	sim_avg_number_of_rtx	/= dir_count
	sim_avg_delay_of_request /= dir_count

	output_file = open(rootdir+"/STATS.txt", "w")

	output_file.write("Total Requests:" + str(sim_total_number_of_requests) +"\n")
	output_file.write( "Statisfied Requests:" + str(sim_total_number_of_statisfied_requests) + "\n")
	output_file.write( "Ratio:" + str(sim_total_number_of_statisfied_requests/sim_total_number_of_requests)+"\n")
	output_file.write( "Avg Delay[s]:" + str(sim_avg_delay_of_request) + "[sec]"+"\n")
	output_file.write( "Avg Hops:" + str(sim_avg_number_of_hops)+"\n")
	output_file.write( "Avg Rtx:" + str(sim_avg_number_of_rtx - 1)+"\n")

def process_app_delay_trace(file):
	#print("app-delays-trace(" + app_file_name + ")")

	f = open(file,"r");

	stats = {}

	TIME_INDEX = 0
	NODE_INDEX = 1
	APP_ID_INDEX = 2 
	SEQ_NUM_INDEX = 3
	TYPE_INDEX = 4
	DELAY_S_INDEX = 5
	DELAY_US_INDEX = 6
	RETX_COUNT_INDEX = 7
	HOP_COUNT_INDEX = 8

	#create first lvl of dictionary and init second level with 0
	for line in f:
		l = line.split('\t')
		if(len(l) < HOP_COUNT_INDEX+1):
			continue

		if("FullDelay" in l[TYPE_INDEX]):
			if(l[NODE_INDEX] not in stats):
				stats[l[NODE_INDEX]] = {}
				stats[l[NODE_INDEX]].update({'DelayS': 0.0})
				stats[l[NODE_INDEX]].update({'RtxCount': 0})
				stats[l[NODE_INDEX]].update({'HopCount': 0})
	
	f.seek(0)
	for line in f:
			l = line.split('\t')
			if(len(l) < HOP_COUNT_INDEX+1):
				continue
			
			if("FullDelay" in l[TYPE_INDEX]):
				stats[l[NODE_INDEX]]['DelayS'] += float(l[DELAY_S_INDEX])
				stats[l[NODE_INDEX]]['RtxCount'] += float(l[RETX_COUNT_INDEX])
				stats[l[NODE_INDEX]]['HopCount'] += float(l[HOP_COUNT_INDEX])
		
	#print stats

	return stats


def process_aggregate_trace(file):
	#print("process-aggregate-trace(" + file + ")")
	
	f = open(file,"r");

	stats = {}

	TIME_INDEX = 0
	NODE_INDEX = 1
	FACE_INDEX = 2 
	FACE_DESCRIPTION_INDEX = 3
	TYPE_INDEX = 4
	PACKET_EWMA_INDEX = 5
	KILOBYTES_INDEX = 6
	PACKET_NR_INDEX = 7

	#create first lvl of dictionary and init second level with 0
	for line in f:
		l = line.split('\t')
		if(len(l) < PACKET_NR_INDEX+1):
			continue

		if("appFace" in l[FACE_DESCRIPTION_INDEX]):
			if(l[NODE_INDEX] not in stats):
				stats[l[NODE_INDEX]] = {}

			if(l[TYPE_INDEX] not in stats[l[NODE_INDEX]]):
				stats[l[NODE_INDEX]].update({l[TYPE_INDEX]: 0})

	#print stats

	#fill second levle with data
	f.seek(0)
	for line in f:
		l = line.split('\t')		
		if(len(l) < PACKET_NR_INDEX+1):
			continue

		if("appFace" in l[FACE_DESCRIPTION_INDEX]):
			stats[l[NODE_INDEX]][l[TYPE_INDEX]] += int(l[PACKET_NR_INDEX])
	
	#print stats
	return stats

#main

#SIMULATION_DIR=os.getcwd()
#SIMULATION_OUTPUT = SIMULATION_DIR + "/output/"
#generateStatsPerSimulation(SIMULATION_OUTPUT)

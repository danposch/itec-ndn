#! /usr/bin/python

import time
import numpy
import os
import glob
import sys
import collections
import re
       

###functions####################################################################

TIME_INDEX = 0
NODE_INDEX = 1
SEGMENT_NR_INDEX = 2
SEGMENT_DUR_INDEX = 3
SEGMENT_REP_INDEX = 4
SEGMENT_BITRATE_INDEX = 5
STALLING_MSEC_INDEX = 6

ALOGIC_NUMBER_SWITCHES_INDEX = 99

def process_dash_trace(f):
	

	file = open(f, "r")
	next(file) #skip header

	stats = {SEGMENT_REP_INDEX: 0.0,
					STALLING_MSEC_INDEX: 0.0,
					SEGMENT_BITRATE_INDEX: 0.0,
					ALOGIC_NUMBER_SWITCHES_INDEX: 0.0}
	
	line_counter = 0
	last_rep_layer = -1
	for line in file:
		l = re.split(r'\t+',line.rstrip('\t'))		
		#print l
		if(len(l) < STALLING_MSEC_INDEX+1):
			continue

		#print "adding stats"
		stats[SEGMENT_REP_INDEX] += float(l[SEGMENT_REP_INDEX])
		stats[SEGMENT_BITRATE_INDEX] += float(l[SEGMENT_BITRATE_INDEX])
		stats[STALLING_MSEC_INDEX] += float(l[STALLING_MSEC_INDEX])

		if line_counter > 1:
			if(last_rep_layer != l[SEGMENT_REP_INDEX]):
				stats[ALOGIC_NUMBER_SWITCHES_INDEX] += 1
				last_rep_layer = l[SEGMENT_REP_INDEX]
		else:
			last_rep_layer = l[SEGMENT_REP_INDEX]

		line_counter += 1

	if(line_counter > 0):
		stats[SEGMENT_REP_INDEX] /= line_counter
		stats[SEGMENT_BITRATE_INDEX] /= line_counter

	file.close()
	#print stats
	return stats

def process_cs_trace(file):
	#print("process_cs_trace(str(file))

	f = open(file,"r");

	stats = {}

	TIME_INDEX = 0
	NODE_INDEX = 1
	TYPE_INDEX = 2
	PACKETS_COUNT_INDEX = 3

	#create first lvl of dictionary and init second level with 0
	for line in f:
		l = line.split('\t')
		if(len(l) < PACKETS_COUNT_INDEX+1):
			continue

		if("CacheHits" in l[TYPE_INDEX] or "CacheMisses" in l[TYPE_INDEX]):
			if(l[NODE_INDEX] not in stats):
				stats[l[NODE_INDEX]] = {}
				stats[l[NODE_INDEX]].update({'CacheHits': 0})
				stats[l[NODE_INDEX]].update({'CacheMisses': 0})
			
			if("CacheHits" in l[TYPE_INDEX]):
				stats[l[NODE_INDEX]]['CacheHits'] += int(l[PACKETS_COUNT_INDEX])
			
			if("CacheMisses" in l[TYPE_INDEX]):
				stats[l[NODE_INDEX]]['CacheMisses'] += int(l[PACKETS_COUNT_INDEX])
		
	f.close()
	return stats

###programm#####################################################################

def computeStats(curdir):

	print "Curring working dir = " + curdir + "\n"

	#process dashplayer-trace_*
	files = glob.glob(curdir + "/dashplayer-trace_*.txt" );
	output_file = open(curdir+"/DASH-STATS.txt", "w")

	###statistic over all clients

	output_file.write("Statistic over all Clients:\n")

	avg_level_per_client = 0.0
	avg_stalls_msec_per_client = 0.0
	avg_consumed_video_bitrate = 0.0
	avg_quality_switches = 0.0

	for f in  files:
		
		#print f
		stats = process_dash_trace(f)
		
		avg_level_per_client += stats[SEGMENT_REP_INDEX]
		avg_stalls_msec_per_client += stats[STALLING_MSEC_INDEX]
		avg_consumed_video_bitrate += stats[SEGMENT_BITRATE_INDEX]
		avg_quality_switches += stats[ALOGIC_NUMBER_SWITCHES_INDEX]
		
	avg_level_per_client /= len(files)
	avg_stalls_msec_per_client /= len(files) 
	avg_consumed_video_bitrate /= len(files)
	avg_quality_switches /= len(files)

	output_file.write("AVG Level per Client: " + str(avg_level_per_client) + "\n")
	output_file.write("AVG Stalls (msec): " + str(avg_stalls_msec_per_client) + "\n")
	output_file.write("AVG Video Bitrate (bit/s):" + str(avg_consumed_video_bitrate) + "\n")
	output_file.write("AVG Number of Switches):" + str(avg_quality_switches) + "\n")

	#well here we can process cs-trace.txt, if we want:

	run_cache_hits = 0.0
	run_cache_misses = 0.0
	run_cache_hit_ratio = 0.0

	if os.path.exists(curdir+"/cs-trace.txt"):
		cs_stats = process_cs_trace(curdir+"/cs-trace.txt")
		for key in cs_stats:
			run_cache_hits += cs_stats[key]['CacheHits']
			run_cache_misses += cs_stats[key]['CacheMisses']
	else:
		print "Could not find cs-trace.txt, skipped parsing cache-hitratio"

	run_cache_hit_ratio = run_cache_hits / (run_cache_hits+run_cache_misses)
	output_file.write( "Cache_Hit_Ratio:" + str(run_cache_hit_ratio)+"\n")

# check if script has been called with a parameter
for arg in sys.argv:
	if 'dashplayer_stats.py' in arg:
		print "Calculating Average via commandline for folder output/, writeing results to output/STATS.txt:"
		computeStats("output/")
		
# end for


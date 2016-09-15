import os
import numpy as np
import scipy as sp
import scipy.stats
import glob
import re

def walklevel(some_dir, level=1):
    some_dir = some_dir.rstrip(os.path.sep)
    assert os.path.isdir(some_dir)
    num_sep = some_dir.count(os.path.sep)
    for root, dirs, files in os.walk(some_dir):
        yield root, dirs, files
        num_sep_this = root.count(os.path.sep)
        if num_sep + level <= num_sep_this:
            del dirs[:]

def getSubdirsForConnectivity(filter):

	simulations = []

	for root, dirs, files in walklevel(DATA_DIR, 0):
			for simulation in dirs:

				if(filter not in simulation):
					continue

				simulations.append(simulation)

	return simulations

def groupByBW(simulations, filter):

	group = []

	for sim in simulations:
				if(filter not in sim):
					continue

				group.append(sim)

	return group

def mean_confidence_interval(data, confidence=0.95):
    a = 1.0*np.array(data)
    n = len(a)
    m, se = np.mean(a), scipy.stats.sem(a)
    h = se * sp.stats.t._ppf((1+confidence)/2., n-1)
    return m, m-h, m+h

def readSetting(filename, setting):

	f = open(filename,"r")
	value = "NaN"

	#special case QoE

	if setting == "QoE":
		qoe_quality = 0.0
		qoe_var = 0.0
		qoe_stalls = 0.0
		for line in f:
			if(line.startswith("Avg.QoE.VideoQuality:")):
				qoe_quality=float(re.findall(r"[-+]?\d*\.\d+|\d+", line[len("Avg.QoE.VideoQuality:"):])[0])
			if(line.startswith("Avg.QoE.QualityVariations:")):
				qoe_var=float(re.findall(r"[-+]?\d*\.\d+|\d+", line[len("Avg.QoE.QualityVariations:"):])[0])
			if(line.startswith("Avg.QoE.StallingTime:")):
				qoe_stalls=float(re.findall(r"[-+]?\d*\.\d+|\d+", line[len("Avg.QoE.StallingTime:"):])[0])

		qoe = {}
		qoe["Avg.QoE.VideoQuality"] = qoe_quality
		qoe["Avg.QoE.QualityVariations"] = qoe_var
		qoe["Avg.QoE.StallingTime"] = qoe_stalls

		value = qoe_quality - LAMBDA * qoe_var - MI * qoe_stalls
		return value, qoe

	for line in f:
		if(line.startswith(setting)):
			value = line[len(setting+":"):]
			value = float(re.findall(r"[-+]?\d*\.\d+|\d+", value)[0])

			if ("Video_SegmentBitrate" in setting):
				value = value / 1000.0

			if ("Video_Switches" in setting):
				value = value * 10.0

			if ("Video_StallingMS" in setting):
				value = value / 1000.0

			break			

	f.close()

	#TODO ENABLE FOR COMBINED FIGURE
	#if(setting == "AVG_Cost_Per_Kilobyte"):
		#value *= 2500000 #scale for combined figure

	return value

# note link failures a not considered yet they are just grouped.
def calculateQuartiles(setting, algorithm):

	print str(setting) + " " + str(algorithm)

	sims = []
	qoe_list = []
	norm = 0
	not_norm = 0

	#extract runs only for algorithm
	simulations = []

	for root, dirs, files in walklevel(DATA_DIR, 0):
			for simulation in dirs:
				if algorithm == simulation:
					sims.append(simulation)

	values = []
	# take only runs of with certain link failures
	for sim in sims:

		for root, dirs, files in walklevel(DATA_DIR + sim, 1):
			for subdir in dirs:
				files = glob.glob(root+"/"+subdir + "/*STATS*.txt" )
				for f in files:

					if setting == "QoE":
						val, qoe_dict = readSetting(f, setting)
						values.append(val)
						qoe_list.append(qoe_dict)
					else:
						values.append(readSetting(f, setting))

	qoe = {}
	if setting == "QoE":
		qoe["Avg.QoE.VideoQuality"] = 0.0
		qoe["Avg.QoE.QualityVariations"] = 0.0
		qoe["Avg.QoE.StallingTime"] = 0.0
		for element in qoe_list:
			qoe["Avg.QoE.VideoQuality"] += element["Avg.QoE.VideoQuality"] / len(qoe_list)
			qoe["Avg.QoE.QualityVariations"] += element["Avg.QoE.QualityVariations"] / len(qoe_list)
			qoe["Avg.QoE.StallingTime"] += element["Avg.QoE.StallingTime"] / len (qoe_list)
	
	#print values

	x, pval = sp.stats.shapiro(values)
	
	print  "Valid_Runs(" + str(len(values)) +")"

	return mean_confidence_interval(values, 0.95), qoe

######################################################

SIMULATION_DIR=os.getcwd()
DATA_DIR=SIMULATION_DIR+"/output/"

#QoE preference "balanced"
LAMBDA = 1.0
MI = 3000.0

SETTINGS_FILTERS = {}

#SETTINGS_FILTERS["sat_ratio"] = ["VoIP_Satisfaction_Ratio", "Data_Satisfaction_Ratio"]
SETTINGS_FILTERS["sat_ratio"] = ["Data_Satisfaction_Ratio"]
SETTINGS_FILTERS["delay"] = ["VoIP_Satisfaction_Delay", "Data_Satisfaction_Delay"]
SETTINGS_FILTERS["video"] = ["Video_SegmentBitrate", "Video_Switches"]
SETTINGS_FILTERS["stalls"] = ["Video_StallingMS"]
SETTINGS_FILTERS["avg_costs"] = ["Avg_Costs"]
SETTINGS_FILTERS["total_costs"] = ["Total_Costs"]
#SETTINGS_FILTERS["kilobyte_costs"] = ["Raw_Kilobytes_Costs", "AVG_Cost_Per_Kilobyte"]
SETTINGS_FILTERS["kilobyte_costs"] = ["Total_Transmitted_Kilobytes", "AVG_Cost_Per_Kilobyte"]
SETTINGS_FILTERS["transmitted_bytes"] = ["Total_Transmitted_Kilobytes"]
SETTINGS_FILTERS["avg_kilobyte_costs"] = ["AVG_Cost_Per_Kilobyte"]
SETTINGS_FILTERS["qoe"] = ["QoE"]
SETTINGS_FILTERS["voip_mos"] = ["VoIP_MOS"]


#SETTINGS_FILTERS= ["VoIP_Satisfaction_Ratio", "VoIP_Satisfaction_Delay", "Data_Satisfaction_Ratio", "Data_Satisfaction_Delay", "Video_StallingMS", "Video_Representation", "Video_SegmentBitrate", "Video_Switches", "Costs"]

#ALGORITHM_FILTERS= ["Broadcast", "NCC", "OMCCRF", "OMPIF", "SAF", "NRR", "BestRoute". "SAF_CAA_1_100_1000"]
ALGORITHM_FILTERS= ["Broadcast", "NCC", "OMCCRF", "OMPIF", "SAF", "NRR", "BestRoute", "SAF_rel_90", "SAF_CAA_rel_90"]
#ALGORITHM_FILTERS= ["OMCCRF", "OMPIF", "SAF"]

#write interest_data_ratio

for group in SETTINGS_FILTERS:

	f = open(DATA_DIR + group + ".dat", "w")
	lines = ""
	lines_qoe_file = "algorithm, Avg.QoE.VideoQuality, Avg.QoE.QualityVariations, Avg.QoE.StallingTime\n"

	for setting in SETTINGS_FILTERS[group]:
		header = "descriptor setting "
		line = setting.translate(None, "_") + " "

		for algorithm in ALGORITHM_FILTERS:
			header += algorithm + ",+,- "
			res, qoe_vals = calculateQuartiles(setting, algorithm)
			line+= "%.4f" % res[0] + " %.4f" % (res[2]-res[0]) + " %.4f" % (res[1]-res[0]) + "\t"

			if(setting == "QoE"):
				lines_qoe_file += algorithm + ", "
				lines_qoe_file += str(qoe_vals["Avg.QoE.VideoQuality"]) + ", "
				lines_qoe_file += str(qoe_vals["Avg.QoE.QualityVariations"]) + ", "
				lines_qoe_file += str(qoe_vals["Avg.QoE.StallingTime"]) + "\n"
	
		lines += line +"\n"

	f.write(header+"\n")
	f.write(lines)
	f.close()

	if(setting == "QoE"):
		f = open(DATA_DIR + "qoe_raw_values.dat", "w")
		f.write(lines_qoe_file)
		f.close()

exit(0)

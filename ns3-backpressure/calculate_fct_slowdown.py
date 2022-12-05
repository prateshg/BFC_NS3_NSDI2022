import os
import sys
import math
from numpy import sort
from numpy import array
filename = sys.argv[1]

os.system('cat '+filename+' | grep DATA_RATE > temp1')
tfile = open('temp1', 'r')
filedata = ''
for input in tfile:
	filedata += input

filedata = filedata.split('\n')
dr = filedata[0].split('\t')[-1]
datarate = int(dr[0:-4])
os.system('rm temp1')
print(datarate)

Incast_size = -1
os.system('cat '+filename+' | grep \'Incast info\' > temp1')
tfile = open('temp1', 'r')
filedata = ''
for input in tfile:
	filedata += input

if filedata=='':
	Incast_size = -1
else:
	Incast_size = int((filedata.split('Num')[0]).split(' ')[-1])

print(Incast_size)

is_w3 = False
os.system('cat '+filename+' | grep WORKLOAD > temp1')
tfile = open('temp1', 'r')
filedata = ''
for input in tfile:
	filedata += input

if filedata=='':
	is_w3= False
else:
	#print(filedata)
	workload = filedata.split('\t')[-1]
	if workload[0:2]=='W3':
		is_w3 = True

print(is_w3)

# FLOW_SIZE_IN_INCAST
# filedata
os.system('rm temp1')
os.system('cat '+filename+' | grep port > temp1')
tfile = open('temp1', 'r')
filedata = ''
for input in tfile:
	filedata += input

num_switches = 128
filedata = filedata.split('\n')
arr = []
port_dict = [{} for _ in range(num_switches+1)]
size_dict = [{} for _ in range(num_switches+1)]
started = 0
finished = 0
for i in range(len(filedata)):
	if filedata[i][0:8]=='Flow fin':
		try:
			sep = filedata[i].split(':')
			sp = sep[0].split(' ')
			sending_node = int(int(sp[4])/2)
			port = int(sp[8][4:])
			np =int((sep[1].split(','))[0])
			ntime = int((sep[2].split('.'))[0])
			time2 = int(((sp[6].split('.'))[0]).split('+')[1])
			if False and time2>1010000000:
				continue
			base_time = 1
			if size_dict[sending_node][port] == 998:
				if port_dict[sending_node][port] == 1:
					base_time = 4000+(8000.0/datarate)*(np+1)
				else:
					base_time = 8000+(8000.0/datarate)*(np+3)
			else:
				if np != 1:
					print('Something wrong')
				if port_dict[sending_node][port] == 1:
					base_time = 4000+(size_dict[sending_node][port]*8.0/datarate)*(np+1)
				else:
					base_time = 8000+(size_dict[sending_node][port]*8.0/datarate)*(np+3)
			ratio = ntime*1.0/base_time
			finished += 1
			#print(ratio)
			arr.append([np,ntime, ratio])
		except:
			continue

	elif filedata[i][0:8]=='Received':
		try:
			sep = filedata[i].split(' ')
			rec_node = int(sep[-6])/2
			sending_node = int((sep[-4].split('.'))[1])
			size = int(sep[-2])
			size_dict[sending_node][int(sep[6])] = size
			if int((rec_node-1)/16)==int((sending_node-1)/16):
				port_dict[sending_node][int(sep[6])] = 1
			else:
				port_dict[sending_node][int(sep[6])] = 0
		except:
			continue
	elif filedata[i][0:8]=='Starting':
		started += 1

n = 10
nums = [0.0 for _ in range(n)]
sums = [0.0 for _ in range(n)]
maxims = [0.0 for _ in range(n)]
medians = [0.0 for _ in range(n)]
avgs = [0.0 for _ in range(n)]
percentiles = [0.0 for _ in range(n)]
percentiles_95 = [0.0 for _ in range(n)]
vals = [[] for _ in range(n)]

for i in range(len(arr)):
	j = -1
	if not is_w3:
		if arr[i][0]==Incast_size:
			j = 9
		elif arr[i][0] <= 3:
			j = 0
		elif arr[i][0] <= 12:
			j = 1
		elif arr[i][0] <= 48:
			j = 2
		elif arr[i][0] <= 192:
			j = 3
		elif arr[i][0] <= 768:
			j = 4
		elif arr[i][0] <= 3072:
			j = 5
		elif arr[i][0] <= 12288:
			j = 6
		elif arr[i][0] <= 49152:
			j = 7
		else:
			j = 8

	else:
		if arr[i][0]==Incast_size:
			j = 9
		elif arr[i][0] <= 1:
			j = 0
		elif arr[i][0] <= 2:
			j = 1
		elif arr[i][0] <= 4:
			j = 2
		elif arr[i][0] <= 8:
			j = 3
		elif arr[i][0] <= 16:
			j = 4
		elif arr[i][0] <= 64:
			j = 5
		elif arr[i][0] <= 256:
			j = 6
		elif arr[i][0] <= 1024:
			j = 7
		else:
			j = 8		
	nums[j] +=1
	sums[j] +=arr[i][2]
	vals[j].append(arr[i][2])
	# if arr[i][2] >= 10000:
	# 	print(j)
	# 	print(arr[i][1])
	# 	print("-----")

for i in range(len(medians)):
	if nums[i] != 0:
		s = sort(array(vals[i]))
		percentiles[i] = s[int(99.0/100.0*nums[i])]
		percentiles_95[i] = s[int(95.0/100.0*nums[i])]
		medians[i] = s[int(nums[i]/2)]
		avgs[i] = sums[i]/nums[i]
		maxims[i] = s[int(nums[i]-1)]
		#print(s)
		# print(len(s))


print("Flows started", end=" ")
print(started)
print("Flows completed", end=" ")
print(finished)
print("nums =", end=" ")
print(nums)
print("averages =", end=" ")
print(avgs)
print("medians =", end = " ")
print(medians)
print("95 percentile =", end=" ")
print(percentiles_95)
print("99 percentile =", end=" ")
print(percentiles)
print("Maximum =", end=" ")
print(maxims)
os.system('rm temp1')
	


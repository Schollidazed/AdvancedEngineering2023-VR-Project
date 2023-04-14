# Original code written by Trammel Hudson.
# Compute the position of a Lighthouse given
# sensor readings in a known configuration.

from sympy import *
from sympy import solve_poly_system
from math import pi
from sys import stdin


# The few vector math functions that we need
def cross(a, b):
	return [
		a[1]*b[2] - a[2]*b[1],
		a[2]*b[0] - a[0]*b[2],
		a[0]*b[1] - a[1]*b[0]
	]

def vecmul(a, k):
	return [
		a[0]*k,
		a[1]*k,
		a[2]*k
	]
def vecsub(a, b):
	return [
		a[0] - b[0],
		a[1] - b[1],
		a[2] - b[2]
	]

def dot(a, b):
	return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]

def len(a):
	return sqrt(dot(a,a))

def unitv(a):
	mag = len(a)
	return [a[0]/mag, a[1]/mag, a[2]/mag]

def ray(a1,a2):
	#print "a1=", a1*180/pi
	#print "a2=", a2*180/pi

	# Normal to X plane
	#      | 
	#   <- |  (This view is looking at lighthouse, sweeps right to left)
	#      |
	plane1 = [+cos(a1), 0, -sin(a1)]
	# Normal to Y plane
	#
	#  (Sweeps from bottom to top.)
	#             ^
	#             |
	#          _ _ _ _ 
	#
	plane2 = [0, +cos(a2), +sin(a2)]

	# Cross the two planes to get the ray vector
	#             |
	#          _ _|_ _
	#             |
	#             |
	# Intersection = where the ray vector is.
	# in this instance, with a1 as 0 and a2 as 0, 
 	# the ray vector is pointing straight in. [0,0,1]
	return unitv(cross(plane2,plane1))

#returns in radians (from -pi/2 to pi/2?, -90deg to 90deg?, 0 is straight ahead.)
def tick2angle(a):
	return pi * (a / 48.0 - 4000) / 8333

def microseconds2angle(a):
	return pi*(( a - 4166.5)/8333)

# My Sensor board is 100x100x25mm
# But the 4 sensors themselves are arranged in a 60mm square.
#
#        +Y
#     3      4
#  		 		+X
#     2      1
#
# (+Z is out of Screen)  
# 

#USER INPUT REQUIRED: 
pos = [
	[+30,-30,25],
	[-30,-30,25],
	[-30,+30,25],
	[+30,+30,25],
]

# Compute the distances between each of the sensors
r01 = N(len(vecsub(pos[0],pos[1])))
r02 = N(len(vecsub(pos[0],pos[2])))
r03 = N(len(vecsub(pos[0],pos[3])))
r12 = N(len(vecsub(pos[1],pos[2])))
r13 = N(len(vecsub(pos[1],pos[3])))
r23 = N(len(vecsub(pos[2],pos[3])))

# Stores Pitch, Roll, and Yaw Euler Rotation values of each Lighthouse from the origin's basis vectors.
# Ideally Leave [N][1] (that means do it. No roll. Only yaw and pitch.)
# Pitch: Rotate down = positive
# Someone else can figure this out lmao.
# Yaw: CCW (Top down) = positive.

#LH0: -135deg Yaw, +30deg Pitch
#LH1: +135deg Yaw, +30deg Pitch
LH_ROT = [
	[-135*pi/180, 0, 30*pi/180],
	[+135*pi/180, 0, 30*pi/180]
]

# Translate them into angles, compute each unit ray vector for each sensor
# and then compute the angles between them
#	
# This is it's own reference frame: Looking out from the lighthouse...
#			+ Y
#			^
#		-----------
#		|		  |
# +X  < |ligthouse|     (Through screen = +Z)
#		|		  |
#		-----------
#
def lighthouse_pos(samples,id):
	#find the unit ray vector for each sensor
	v1 = ray (microseconds2angle(samples[1][id*2]), microseconds2angle(samples[1][id*2+1]))
	v2 = ray (microseconds2angle(samples[2][id*2]), microseconds2angle(samples[2][id*2+1]))
	v3 = ray (microseconds2angle(samples[3][id*2]), microseconds2angle(samples[3][id*2+1]))
	v0 = ray (microseconds2angle(samples[0][id*2]), microseconds2angle(samples[0][id*2+1]))

	#Angles between each unit vector
	#Or rather the dot product.
	v01 = dot(v0,v1)
	v02 = dot(v0,v2)
	v03 = dot(v0,v3)
	v12 = dot(v1,v2)
	v13 = dot(v1,v3)
	v23 = dot(v2,v3)

	#print "v0=", v0
	#print "v1=", v1
	#print "v2=", v2
	#print "v3=", v3
	print ("v01=", acos(v01) * 180/pi, " deg")
	print ("v02=", acos(v02) * 180/pi, " deg")
	print ("v03=", acos(v03) * 180/pi, " deg")
	print ("v12=", acos(v12) * 180/pi, " deg")
	print ("v13=", acos(v13) * 180/pi, " deg")
	print ("v23=", acos(v23) * 180/pi, " deg")

	#Find the magnitude of the distances from the lighthouse to each sensor, 
    #using the angles calculated from the unit vectors.
	k0, k1, k2, k3 = symbols('k0, k1, k2, k3')
	sol = nsolve((
		k0**2 + k1**2 - 2*k0*k1*v01 - r01**2,
		k0**2 + k2**2 - 2*k0*k2*v02 - r02**2,
		k0**2 + k3**2 - 2*k0*k3*v03 - r03**2,
		k2**2 + k1**2 - 2*k2*k1*v12 - r12**2,
		k3**2 + k1**2 - 2*k3*k1*v13 - r13**2,
		k2**2 + k3**2 - 2*k2*k3*v23 - r23**2,
	),
		(k0, k1, k2, k3),
		(1000,1000,1000,1000),
		verify=False  # ignore tolerance of solution
	)

	#print sol
 
	# Position (XYZ) of each sensor relative to the lighthouse.
	p0 = vecmul(v0,sol[0])
	p1 = vecmul(v1,sol[1])
	p2 = vecmul(v2,sol[2])
	p3 = vecmul(v3,sol[3])

	print ("p0=", p0)
	print ("p1=", p1)
	print ("p2=", p2)
	print ("p3=", p3)

	# compute our own estimate of the error
	print ("err01=", len(vecsub(p0,p1)) - r01, " mm")
	print ("err02=", len(vecsub(p0,p2)) - r02, " mm")
	print ("err03=", len(vecsub(p0,p3)) - r03, " mm")
	print ("err12=", len(vecsub(p1,p2)) - r12, " mm")
	print ("err13=", len(vecsub(p1,p3)) - r13, " mm")
	print ("err23=", len(vecsub(p2,p3)) - r23, " mm")
	
 	#According to Trammel Hudson: k_n^2 = |P-p_n|^2
	#	Where P-p_n is a vector subtraction.
  	#	The | indicates an absolute value, indicates a magnitude., which is wat k_n is!
    #	Squares help us narrow down a solution.
	Px, Py, Pz = symbols('Px, Py, Pz')
	LighthousePos = nsolve((
		(Px-p0[0])**2 + (Py-p0[1])**2 + (Pz-p0[2])**2 - sol[0]**2,
		(Px-p1[0])**2 + (Py-p1[1])**2 + (Pz-p1[2])**2 - sol[1]**2,
		(Px-p2[0])**2 + (Py-p2[1])**2 + (Pz-p2[2])**2 - sol[2]**2,
		(Px-p3[0])**2 + (Py-p3[1])**2 + (Pz-p3[2])**2 - sol[3]**2,
	),
		(Px, Py, Pz),
		(0, 0, 0),
		verify=False  # ignore tolerance of solution
	)

	print("P", id, "x: ", LighthousePos[0])
	print("P", id, "y: ", LighthousePos[1])
	print("P", id, "z: ", LighthousePos[2])

	# We'll need to manually input the euler angles to find the rotation matrix...
	pitch = LH_ROT[id][0]
	roll = LH_ROT[id][1]
	yaw = LH_ROT[id][2]
	
	# Assuming you adjusted the yaw, then the pitch...
  	# https://learnopencv.com/rotation-matrix-to-euler-angles/
	Rz = Matrix([
    	[cos(yaw), sin(yaw), 0],
       	[-sin(yaw), cos(yaw), 0],
       	[0, 0, 1]
    ])
	Rx = Matrix([
		[1, 0, 0],
		[0, cos(pitch), -sin(pitch)],
		[0, sin(pitch), cos(pitch)]
	])
	R = Rz * Rx
	
	print(R)
 

 

#######################################################################

# Originally...
# The four parameter sets as input are the raw tick measurements
# in 48 MHz system clock values.
#

# Accumulate lots of samples for each sensor while they
# are stationary at the origin (0,0,0) and compute
# the average of them so that we have a better measurement.
#
# If we have a good view this should be just a few seconds.
# In this instance, I will be computing the averages myself.

# #Total sample count
# total_count = 0
# #Sample count for each sensor
# count = [0,0,0,0]
#Sample Array[Sensor][Lighthouse/Axis]
samples = [
	[3300.1428571428573, 3664.3030303030305, 2030.2424242424242, 3938.0847457627120],
	[3017.294117647059,  3722.629213483146,  2069.1111111111113, 3939.5495495495497],
	[2750.1739130434785, 3722.8117647058825, 2025.0212765957447, 3833.4919354838707],
	[3046.529411764706,  3553.814814814815,  1985.4166666666667, 3903.8629032258063],
]

# Throw away any old serial data
# for n in range(0,200):
# 	line = stdin.readline()

# #Read 200 Samples from stdin
# #Formatted: SensorID, LH0_Ax0(X), LH0_Ax1(Y), LH1_Ax0(X), LH1_Ax1(Y),
# #This adds all of the data recieved
# for n in range(0,200):
# 	line = stdin.readline()
# 	cols = line.split(",", 6)
# 	print (cols)
# 	i = int(cols[0])
# 	if i < 0 or i > 3:
# 		print ("parse error")
# 		continue

# 	total_count += 1
# 	count[i] += 1
# 	samples[i][0] += int(cols[1])
# 	samples[i][1] += int(cols[2])
# 	samples[i][2] += int(cols[3])
# 	samples[i][3] += int(cols[4])

# # Check that we have enough of each
# # at least 10% of the samples must be from this one
# # This section averages all the samples
# fail = False
# print (count)
# for i in range(0,4):
# 	if count[i] < total_count / 10:
# 		print (str(i) + ": too few samples")
# 		exit(-1)
# 	samples[i][0] /= count[i]
# 	samples[i][1] /= count[i]
# 	samples[i][2] /= count[i]
# 	samples[i][3] /= count[i]

# print (samples)

##################################################
## The Main Part...
lighthouse_pos(samples, 0)
lighthouse_pos(samples, 1)

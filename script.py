import sys
f = open(sys.argv[1])

enq = []
deq = []
recv = []
drop = []
for line in f.readlines():
	if("UdpHeader" in line):
		if(line[0] == '+'):
			enq.append(line.split(" "))
		elif(line[0] == '-'):
			deq.append(line.split(" "))
		elif(line[0] == 'r'):
			recv.append(line.split(" "))
		elif(line[0] == 'd'):
			drop.append(line.split(" "))
		
#def timeFirstTPacket

#print enq[0][-8]



print "timeFirstTPacket :",deq[0][1]
print "lastLastTPacket :",deq[-1][1]
print "timeFirstRPacket :",recv[0][1]
print "timeLastRPacket :",recv[-1][1]

delay=0

for i in range(len(recv)):
	delay+=float(recv[i][1])-float(deq[i][1])

print "delaySum :",delay
	












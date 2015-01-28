import sys
f = open(sys.argv[1])

enq = []
deq = []
recv = []
drop = []
tBytes = 0
rBytes = 0

str1=0
str0=0

sre0=0
sre1=0

sumDropped = 0
for line in f.readlines():
	if("UdpHeader" in line):
		if(line[0] == '+'):
			enq.append(line.split(" "))
		elif(line[0] == '-'):
			deq.append(line.split(" "))
			tBytes+=int((deq[-1][38])[6:-1])
			if(deq[-1][2][10]=="0"):
				str0+=int((deq[-1][38])[6:-1])
			elif(deq[-1][2][10]=="1"):
				str1+=int((deq[-1][38])[6:-1])
		elif(line[0] == 'r'):
			recv.append(line.split(" "))
			rBytes+=int((recv[-1][38])[6:-1])
			if(recv[-1][2][10]=="0"):
				sre0+=int((recv[-1][38])[6:-1])
			elif(recv[-1][2][10]=="1"):
				sre1+=int((recv[-1][38])[6:-1])
		elif(line[0] == 'd'):
			drop.append(line.split(" "))
			sumDropped+=int((drop[-1][38])[6:-1])
		
#def timeFirstTPacket

#print enq[0][-8]

for x in enq:
	if(x[2][10]=="0"):
		eFirst0=float(x[1])
		break
for x in enq:
	if(x[2][10]=="1"):
		eFirst1=float(x[1])
		break

for i in reversed(range(len(recv))):
	if(recv[i][2][10]=="0"):
		rLast0=float(recv[i][1])
		break

for i in reversed(range(len(recv))):
	if(recv[i][2][10]=="1"):
		rLast1=float(recv[i][1])
		break

print(eFirst0,eFirst1,rLast0,rLast1)
t0=rLast0-eFirst0
t1=rLast1-eFirst1


tot_time=float(recv[-1][1])-float(enq[-1][1])



print "timeFirstTPacket :",deq[0][1]
print "lastLastTPacket :",deq[-1][1]
print "timeFirstRPacket :",recv[0][1]
print "timeLastRPacket :",recv[-1][1]

delay=0


for i in range(len(recv)):
    for j in reversed(range(len(deq))):
        if(recv[i][18]==deq[j][18]):
            delay+=float(recv[i][1])-float(deq[j][1])
            break


print "delaySum :",delay
print "tBytes :",tBytes
print "rBytes :",rBytes
print "lostPackets :",len(deq)-len(recv)
print "timesForwarded :",len(deq)-len(enq)
print "packetsDropped :",len(drop), ", bytesDropped: ", sumDropped 
print "transmitterThroughput Node0: ", str0/t0, ", transmitterThroughput Node1: ",str1/t1
print "receiverThroughput Node0: ", sre0/t0, ", receiverThroughput Node1: ",sre1/t1








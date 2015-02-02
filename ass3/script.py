import sys
import os

throughput=[]
forward_delay=[]
jitter=[]
path_name=sys.argv[1]
if(path_name[-1]=='/'):
  path_name=str(path_name[0:-1])
print(os.path.basename(path_name))
params=os.path.basename(path_name).split("-")[2:-1]
interval=float(params[0])
packetSize=float(params[1])/1024
dataGenerationRate=(1/interval)*(packetSize)
print interval,"s,",packetSize,"KB,",dataGenerationRate, "KBps"


def calcSD(arr):
  avg=0
  sd=0
  for x in arr:
    avg+=x
  avg=avg/len(arr)
  for x in arr:
    sd+=(x-avg)**2
  #print(sd)
  sd=sd/len(arr)
  sd=sd**0.5
  return [avg,sd]


for u in range(0,10):
  enq = []
  deq = []
  recv = []
  drop = []
  time =0
  lasttime=0
  rBytes=0
  dic={}
  ind=['6','8','10','12']
  for x in ind:
    dic[x]={}
    dic[x]['enq']=[]
    dic[x]['deq']=[]
    dic[x]['recv']=[]
    dic[x]['drop']=[]
  
  #print(dic)

  f = open(path_name+"/"+os.path.basename(path_name)+"-"+str(u)+".tr")
  for line in f.readlines():
    if("UdpHeader" in line):
      lsplt=line.split(" ")
      added=int(lsplt[28][-1])+int(lsplt[30][-2])
      lasttime=float((lsplt)[1])
      if(time==0):
        time=lasttime
      if(line[0] == '+'):
        dic[str(added)]['enq'].append(lsplt)
      elif(line[0] == '-'):
        dic[str(added)]['deq'].append(lsplt)
        #tBytes+=int((deq[-1][38])[6:-1])
        #if(deq[-1][2][10]=="0"):
          #str0+=int((deq[-1][38])[6:-1])
        #elif(deq[-1][2][10]=="1"):
        #  str1+=int((deq[-1][38])[6:-1])
      elif(line[0] == 'r'):
        dic[str(added)]['recv'].append(lsplt)
        out=dic[str(added)]['recv'][-1][39].split(":")
        rBytes+=int(out[1][0:-1])-int(out[0][1:])
        #print(rBytes)
        #if(recv[-1][2][10]=="0"):
          #sre0+=int((recv[-1][38])[6:-1])
        #elif(recv[-1][2][10]=="1"):
        #  sre1+=int((recv[-1][38])[6:-1])
      elif(line[0] == 'd'):
        dic[str(added)]['drop'].append(lsplt)
        #sumDropped+=int((drop[-1][38])[6:-1])

  packets=0
  q_delay=0
  max_delay=-1
  min_delay=1000000000

  for x in ind:
    for i in range(0,len(dic[x]['recv'])):
      packets+=1
      q_delay+=float(dic[x]['recv'][i][1])-float(dic[x]['enq'][i][1]);
      delay=float(dic[x]['recv'][i][1])-float(dic[x]['deq'][i][1]);
      if(delay>max_delay):
        max_delay=delay
      if(delay<min_delay):
        min_delay=delay
  time=lasttime-time
  #print(i,len(dic['6']['enq']),len(dic['6']['deq']),len(dic['6']['drop']))
  throughput.append(rBytes/time)
  forward_delay.append(q_delay/packets)
  jitter.append(max_delay-min_delay)
  print(u, rBytes/time,q_delay/packets,max_delay-min_delay)
#jitter=[2,6,10]

throughput_fin=calcSD(throughput)
forward_delay_fin=calcSD(forward_delay)
jitter_fin=calcSD(jitter)





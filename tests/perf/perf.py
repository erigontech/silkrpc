import os
import csv
import sys
import signal
import subprocess
from datetime import datetime


#default vars
testsSequence="50+30-200+30-200+60-400+30" 
repTests=10
patternTarFile=""
gethHomeDir="../../../turbo-geth/"
daemonOnCore="-"
gethNode="localhost"

#
#
#
def processVarArgs():
    global patternTarFile
    global repTests
    global testsSequence
    global gethHomeDir
    global daemonOnCore
    global gethNode
    if (len(sys.argv) == 2):
       patternTarFile=sys.argv[1]
    elif (len(sys.argv) == 3):
       patternTarFile=sys.argv[1]
       daemonOnCore=sys.argv[2]
    elif (len(sys.argv) == 4):
       patternTarFile=sys.argv[1]
       daemonOnCore=sys.argv[2]
       gethNode=sys.argv[3]
    elif (len(sys.argv) == 5):
       patternTarFile=sys.argv[1]
       daemonOnCore=sys.argv[2]
       gethNode=sys.argv[3]
       gethHomeDir=sys.argv[4]
    elif (len(sys.argv) == 6):
       patternTarFile=sys.argv[1]
       daemonOnCore=sys.argv[2]
       gethNode=sys.argv[3]
       gethHomeDir=sys.argv[4]
       repTests=int(sys.argv[5])
    elif (len(sys.argv) == 7): 
       patternTarFile=sys.argv[1]
       daemonOnCore=sys.argv[2]
       gethNode=sys.argv[3]
       gethHomeDir=sys.argv[4]
       repTests=int(sys.argv[5])
       testsSequence=sys.argv[6]
    elif (len(sys.argv) != 2):
       print ("usage: " + sys.argv[0] + " <patternTarFile> [daemonOnCore i.e - or 0-1 or 0-2 ...] [gethNode i.e localhost or IP] [gethHomeDir i.e ../../../turbo-geth/] [test repetition i.e 6] [test seq i.e 200+30-400+10]");
       sys.exit(-1)

#
#
#
def copiesPatternFile():
    cmd="/bin/cp -f " + patternTarFile + " /tmp"
    os.system(cmd)
    cmd="cd /tmp"
    os.system(cmd)
    print ("Extracting vegeta patten ...")
    cmd="tar xvf " + patternTarFile
    os.system(cmd)
    cmd="cd -"
    os.system(cmd)

#
#
#
def startRpcDaemon():
    if (daemonOnCore == "-"):
       cmd=gethHomeDir + "build/bin/rpcdaemon --private.api.addr="+gethNode+":9090 --http.api=eth,debug,net,web3 &"
    else:
       cmd ="taskset -c " + daemonOnCore + " " + gethHomeDir + "build/bin/rpcdaemon --private.api.addr="+gethNode+":9090 --http.api=eth,debug,net,web3 &"
    print("RpcDaemon starting ...: ",cmd)
    os.system(cmd)
    os.system("sleep 1")

#
#
#
def stopRpcDaemon():
    os.system("kill -9 $(ps aux | grep 'rpcdaemon' | grep -v 'grep' | awk '{print $2}') 2>1&")
    print("RpcDaemon stopped")
    os.system("sleep 1")

#
#
#
def startSilkDaemon():
    if (daemonOnCore == "-"):
       cmd ="../../build_gcc_release/silkrpc/silkrpcdaemon --target "+gethNode+":9090 --local "+gethNode+":51515 --logLevel c &"
    else:
       cmd="taskset -c " +daemonOnCore+ " ../../build_gcc_release/silkrpc/silkrpcdaemon --target "+gethNode+":9090 --local "+gethNode+":51515 --logLevel c &"
    print("SilkDaemon starting ...: ",cmd)
    os.system(cmd)
    os.system("sleep 1")

#
#
#
def stopSilkDaemon():
    os.system("kill $(ps aux | grep 'silk' | grep -v 'grep' | awk '{print $2}') 2>1&")
    print("SilkDaemon stopped")
    os.system("sleep 1")

#
#
#
def execute(name,qps,time):
    scriptName = "./vegeta_attack_getLogs_"+name+".sh" +" "+str(qps) + " " + str(time)
    print(name+": execute test qps:",str(qps) + " time:"+str(time))
    ret=os.system(scriptName);

#
#
#
def getResult(testNo,name,qps,time):
    testReportFileName = "./getLogs_"+str(qps)+"qps_"+str(time)+"s_"+name+"_perf.hrd"
    file = open(testReportFileName)
    try:
       r=file.readlines()
       latency=r[2]
       latencyValues=latency.split(",")
       tmp=latencyValues[6].split(']')
       min=tmp[1]
       mean=latencyValues[7]
       fifty=latencyValues[8]
       ninty=latencyValues[9]
       nintyfive=latencyValues[10]
       nintynine=latencyValues[11]
       max=latencyValues[12]
    finally:
       file.close()
    writeCsvFile(name,testNo,qps,time,min,mean,fifty,ninty,nintyfive,nintynine,max)
    cmd="/bin/rm " + testReportFileName
    os.system(cmd)

#
#
#
def writeCsvFileHeader():
    command = "sum "+ patternTarFile
    checksum = os.popen(command).read().split('\n')

    command = "gcc --version"
    gccVers = os.popen(command).read().split(',')

    os_info = os.uname()
    writer.writerow(["","","","","","","","","","","","","PC", os_info.nodename])
    writer.writerow(["","","","","","","","","","","","","DaemonRunOnCore", daemonOnCore ])
    writer.writerow(["","","","","","","","","","","","","VegetaFile", patternTarFile])
    writer.writerow(["","","","","","","","","","","","","VegetaChecksum", checksum[0]])
    writer.writerow(["","","","","","","","","","","","","GccVers", gccVers[0]])
    writer.writerow(["","","","","","","","","","","","","SilkVersion", "TBD"])
    writer.writerow(["","","","","","","","","","","","","RpcDaemon", "TBD"])
    writer.writerow([])
    writer.writerow([])
    writer.writerow(["Daemon","TestNo","Qps","Time","Min","Mean","50","90","95","99","Max"])

#
#
#
def writeCsvFile(daemon,testNo,qps,time,min,mean,fifty,ninty,nintyfive,nintynine,max):
    writer.writerow([daemon, str(testNo), qps, time, str(min), str(mean), str(fifty), str(ninty), str(nintyfive), str(nintynine), str(max)])
    
#
# main
# usage: python perf.py <patternTarFile> [daemonOnCore i.e - or 0-1 or 0-2 ...] [gethNode i.e localhost or IP] [gethHomeDir i.e ../../../turbo-geth/] [test repetition i.e 6] [test seq i.e 200+30-400+10]
#
processVarArgs();

#
# stops any previos daemon 
#
stopRpcDaemon()
stopSilkDaemon()

copiesPatternFile()
date=datetime.today().strftime('%Y-%m-%d-%H:%M:%S')
csvFileName=date+"_perf.csv"

print ("\nTest using repetition: "+ str(repTests) + " on sequence: " +  str(testsSequence) + " for pattern: " + str(patternTarFile));
print ("Creating report file:"+csvFileName+"\n");

csvFile=open(csvFileName, 'w', newline='')
writer = csv.writer(csvFile)
writeCsvFileHeader()
csvFile.flush()

startSilkDaemon()


testsSequenceStr=str(testsSequence)
currentSeq=testsSequenceStr.split('-')
for test in currentSeq:
   for testNo in range(0,repTests):
      qps=test.split('+')[0]
      time=test.split('+')[1]
      execute("silkrpc",qps,time)
      getResult(testNo,"silkrpc",qps,time)
      csvFile.flush()
      os.system("sleep 1")
      

stopSilkDaemon()
print ("\n")
startRpcDaemon()

for test in currentSeq:
   for testNo in range(0,repTests):
      qps=test.split('+')[0]
      time=test.split('+')[1]
      execute("rpcdaemon",qps,time)
      getResult(testNo,"rpcdaemon",qps,time)
      csvFile.flush()
      os.system("sleep 1")

csvFile.close()
stopRpcDaemon()
sys.exit(0)


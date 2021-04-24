#!/usr/bin/env python3

""" This script is responsible using vegeta to make a list of performance tests (configured via command line) and saves its result on csv file
"""

import os
import csv
import sys
from datetime import datetime

DEFAULT_TEST_SEQUENCE = "50:30,200:30,200:60,400:30"
DEFAULT_REPETITIONS = 10
DEFAULT_VEGETA_PATTERN_TAR_FILE = ""
DEFAULT_DAEMON_ON_CORE = "-"
DEFAULT_GETH_HOME_DIR = "../../../turbo-geth/"
DEFAULT_TG_ADDRESS = "localhost:9090"


class Config:
    """ This class manage configuration params
    """
    def __init__(self, argv):
        """ Processes the command line contained in argv
        """
        self.test_sequence = DEFAULT_TEST_SEQUENCE
        self.repetitions = DEFAULT_REPETITIONS
        self.vegeta_pattern_tar_file = DEFAULT_VEGETA_PATTERN_TAR_FILE
        self.geth_homedir = DEFAULT_GETH_HOME_DIR
        self.daemon_on_core = DEFAULT_DAEMON_ON_CORE
        self.tg_addr = DEFAULT_TG_ADDRESS

        if len(argv) == 2:
            self.vegeta_pattern_tar_file = argv[1]
        elif len(argv) == 3:
            self.vegeta_pattern_tar_file = argv[1]
            self.daemon_on_core = argv[2]
        elif len(argv) == 4:
            self.vegeta_pattern_tar_file = argv[1]
            self.daemon_on_core = argv[2]
            self.tg_addr = argv[3]
        elif len(argv) == 5:
            self.vegeta_pattern_tar_file = argv[1]
            self.daemon_on_core = argv[2]
            self.tg_addr = argv[3]
            self.geth_homedir = argv[4]
        elif len(argv) == 6:
            self.vegeta_pattern_tar_file = argv[1]
            self.daemon_on_core = argv[2]
            self.tg_addr = argv[3]
            self.geth_homedir = argv[4]
            self.repetitions = int(argv[5])
        elif len(argv) == 7:
            self.vegeta_pattern_tar_file = argv[1]
            self.daemon_on_core = argv[2]
            self.tg_addr = argv[3]
            self.geth_homedir = argv[4]
            self.repetitions = int(argv[5])
            self.test_sequence = argv[6]
        else:
            print("Usage: " + argv[0] + " vegetaPatternTarFile [daemonOnCore] [turboGethAddress] [turboGethHomeDir] [testRepetitions] [testSequence]")
            print("")
            print("Launch an automated performance test sequence on Silkrpc and RPCDaemon using Vegeta")
            print("")
            print("vegetaPatternTarFile     path to the request file for Vegeta attack")
            print("daemonOnCore             logical cpu list in taskset format (e.g. - or 0-1 or 0-2 or 0,2...)              [default: " + DEFAULT_DAEMON_ON_CORE +"]")
            print("turboGethAddress         address of TG Core component as <address>:<port> (e.g. localhost:9090)           [default: " + DEFAULT_TG_ADDRESS + "]")
            print("turboGethHomeDir         path to TG home folder (e.g. ../../../turbo-geth/)                               [default: " + DEFAULT_GETH_HOME_DIR + "]")
            print("testRepetitions          number of repetitions for each element in test sequence (e.g. 10)                [default: " + str(DEFAULT_REPETITIONS) + "]")
            print("testSequence             list of query-per-sec and duration tests as <qps1>:<t1>,... (e.g. 200:30,400:10) [default: " + DEFAULT_TEST_SEQUENCE + "]")
            sys.exit(-1)


class PerfTest:
    """ This class manage performance test
    """
    def __init__(self, test_report, config):
        """ The initialization routine stop any previos server
        """
        self.test_report = test_report
        self.config = config
        self.rpc_daemon = 0
        self.silk_daemon = 0
        self.stop_rpc_daemon()
        self.stop_silk_daemon()
        self.copy_pattern_file()

    def copy_pattern_file(self):
        """ copy the vegeta pattern file into /tmp and untar zip file
        """
        cmd = "/bin/cp -f " + self.config.vegeta_pattern_tar_file + " /tmp"
        os.system(cmd)
        print("Extracting vegeta patten ...")
        cmd = "cd /tmp; tar xvf " + self.config.vegeta_pattern_tar_file + " > /dev/null"
        os.system(cmd)

    def start_rpc_daemon(self):
        """ Starts RPC daemon server
        """
        self.rpc_daemon = 1
        if self.config.daemon_on_core == "-":
            cmd = self.config.geth_homedir + "build/bin/rpcdaemon --private.api.addr="+self.config.tg_addr+" --http.api=eth,debug,net,web3 &"
        else:
            cmd = "taskset -c " + self.config.daemon_on_core + " " + \
                   self.config.geth_homedir + "build/bin/rpcdaemon --private.api.addr="+self.config.tg_addr+" --http.api=eth,debug,net,web3 &"
        print("RpcDaemon starting ...: ", cmd)
        os.system(cmd)
        os.system("sleep 1")

    def stop_rpc_daemon(self):
        """ Stops the RPC daemon server
        """
        self.rpc_daemon = 0
        os.system("kill -9 $(ps aux | grep 'rpcdaemon' | grep -v 'grep' | awk '{print $2}') 2> /dev/null ")
        print("RpcDaemon stopped")
        os.system("sleep 1")

    def start_silk_daemon(self):
        """ Starts SILKRPC daemon
        """
        self.rpc_daemon = 1
        if self.config.daemon_on_core == "-":
            cmd = "../../build_gcc_release/silkrpc/silkrpcdaemon --target "+self.config.tg_addr+" --local localhost:51515 --logLevel c &"
        else:
            cmd = "taskset -c " + self.config.daemon_on_core + \
              " ../../build_gcc_release/silkrpc/silkrpcdaemon --target "+self.config.tg_addr+" --local localhost:51515 --logLevel c &"
        print("SilkDaemon starting ...: ", cmd)
        os.system(cmd)
        os.system("sleep 1")

    def stop_silk_daemon(self):
        """ Stops SILKRPC daemon
        """
        self.silk_daemon = 0
        os.system("kill $(ps aux | grep 'silk' | grep -v 'grep' | awk '{print $2}') 2> /dev/null")
        print("SilkDaemon stopped")
        os.system("sleep 1")


    def execute(self, test_number, name, qps_value, time_value):
        """ Executes the tests using qps and time variable
        """
        script_name = "./vegeta_attack_getLogs_"+name+".sh" +" "+str(qps_value) + " " + str(time_value)
        print(test_number+" "+name+": executes test qps:", str(qps_value) + " time:"+str(time_value))
        os.system(script_name)
        self.get_result(test_number, name, qps_value, time_value)


    def get_result(self, test_number, daemon_name, qps_value, time_value):
        """ Processes the report file generated by vegeta and reads latency data
        """
        test_report_filename = "./getLogs_"+str(qps_value)+"qps_"+str(time_value)+"s_"+daemon_name+"_perf.hrd"
        file = open(test_report_filename)
        try:
            file_raws = file.readlines()
            newline = file_raws[2].replace('\n', ' ')
            latency_values = newline.split(',')
            min_latency = latency_values[6].split(']')[1]
            max_latency = latency_values[12]
            newline = file_raws[5].replace('\n', ' ')
            ratio = newline.split(' ')[34]
            threads = os.popen("ps -efL | grep tg | wc -l").read().replace('\n', ' ')
        finally:
            file.close()

        self.test_report.write_test_report(daemon_name, test_number, threads, qps_value, time_value, min_latency, latency_values[7], latency_values[8], \
                                           latency_values[9], latency_values[10], latency_values[11], max_latency, ratio)
        os.system("/bin/rm " + test_report_filename)


class TestReport:
    """ This class write the test report
    """

    def __init__(self, config):
        """
        """
        self.csv_file = ''
        self.writer = ''
        self.config = config

    def open(self):
        """ Writes on CVS file the header
        """
        csv_filename = datetime.today().strftime('%Y-%m-%d-%H:%M:%S')+"_perf.csv"
        self.csv_file = open(csv_filename, 'w', newline='')
        self.writer = csv.writer(self.csv_file)

        print("Creating report file: "+csv_filename+"\n")

        command = "sum "+ self.config.vegeta_pattern_tar_file
        checksum = os.popen(command).read().split('\n')

        command = "gcc --version"
        gcc_vers = os.popen(command).read().split(',')

        command = "cat /proc/cpuinfo | grep 'model name' | uniq"
        model = os.popen(command).readline().replace('\n', ' ').split(':')
        command = "cat /proc/cpuinfo | grep 'bogomips' | uniq"
        tmp = os.popen(command).readline().replace('\n', '').split(':')[1]
        bogomips = tmp.replace(' ', '')

        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "PC", model[1]])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "Bogomips", bogomips])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "DaemonRunOnCore", self.config.daemon_on_core])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "TG address", self.config.tg_addr])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "VegetaFile", self.config.vegeta_pattern_tar_file])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "VegetaChecksum", checksum[0]])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "GccVers", gcc_vers[0]])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "SilkVersion", "master"])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "RpcDaemon", "master"])
        self.writer.writerow([])
        self.writer.writerow([])
        self.writer.writerow(["Daemon", "TestNo", "TG-Threads", "Qps", "Time", "Min", "Mean", "50", "90", "95", "99", "Max", "Ratio"])
        self.csv_file.flush()


    def write_test_report(self, daemon, test_number, threads, qps_value, time_value, min_latency, mean, fifty, ninty, nintyfive, nintynine, max_latency, ratio):
        """ Writes on CVS the latency data for one completed test
        """
        self.writer.writerow([daemon, str(test_number), threads, qps_value, time_value, min_latency, mean, fifty, ninty, nintyfive, nintynine, max_latency, ratio])
        self.csv_file.flush()


    def close(self):
        """ Close the report
        """
        self.csv_file.flush()
        self.csv_file.close()


#
# main
#
def main(argv):
    """ Executes tests on selected user configuration
    """

    config = Config(argv)

    test_report = TestReport(config)
    perf_test = PerfTest(test_report, config)

    print("\nTest using repetition: "+ str(config.repetitions) + " on sequence: " +  str(config.test_sequence) + " for pattern: " + str(config.vegeta_pattern_tar_file))
    test_report.open()

    perf_test.start_silk_daemon()

    current_sequence = str(config.test_sequence).split(',')
    test_number = 1
    for test in current_sequence:
        for test_rep in range(0, config.repetitions):
            qps = test.split(':')[0]
            time = test.split(':')[1]
            perf_test.execute(str(test_number)+"."+str(test_rep+1), "silkrpc", qps, time)
            os.system("sleep 1")
        test_number = test_number + 1
        print("")

    perf_test.stop_silk_daemon()
    print("")

    perf_test.start_rpc_daemon()

    test_number = 1
    for test in current_sequence:
        for test_rep in range(0, config.repetitions):
            qps = test.split(':')[0]
            time = test.split(':')[1]
            perf_test.execute(str(test_number)+"."+str(test_rep+1), "rpcdaemon", qps, time)
            os.system("sleep 1")
        test_number = test_number + 1
        print("")

    perf_test.stop_rpc_daemon()
    test_report.close()


#
# module as main
#
if __name__ == "__main__":
    main(sys.argv)
    sys.exit(0)

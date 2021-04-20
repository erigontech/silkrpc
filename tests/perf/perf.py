""" This script is responsible using vegeta to make a list of performance tests (configured via command line) and saves its result on csv file
"""

import os
import csv
import sys
from datetime import datetime

DEFAULT_TEST_SEQUENCE = "50+30-200+30-200+60-400+30"
DEFAULT_REPETITION_VALUE = 10
DEFAULT_PATTERN_TAR_FILE = ""
DEFAULT_DAEMON_ON_CORE = "-"
DEFAULT_GETH_HOME_DIR = "../../../turbo-geth/"
DEFAULT_GETH_NODE_ADDRESS = "localhost"


class Config:
    """ This class manage configuration params
    """
    def __init__(self):
        """ Processes the command line contained in argv
        """
        self.test_sequence = DEFAULT_TEST_SEQUENCE
        self.rep_tests = DEFAULT_REPETITION_VALUE
        self.pattern_tar_file = DEFAULT_PATTERN_TAR_FILE
        self.geth_homedir = DEFAULT_GETH_HOME_DIR
        self.daemon_on_core = DEFAULT_DAEMON_ON_CORE
        self.geth_node_addr = DEFAULT_GETH_NODE_ADDRESS

        if len(sys.argv) == 2:
            self.pattern_tar_file = sys.argv[1]
        elif len(sys.argv) == 3:
            self.pattern_tar_file = sys.argv[1]
            self.daemon_on_core = sys.argv[2]
        elif len(sys.argv) == 4:
            self.pattern_tar_file = sys.argv[1]
            self.daemon_on_core = sys.argv[2]
            self.geth_node_addr = sys.argv[3]
        elif len(sys.argv) == 5:
            self.pattern_tar_file = sys.argv[1]
            self.daemon_on_core = sys.argv[2]
            self.geth_node_addr = sys.argv[3]
            self.geth_homedir = sys.argv[4]
        elif len(sys.argv) == 6:
            self.pattern_tar_file = sys.argv[1]
            self.daemon_on_core = sys.argv[2]
            self.geth_node_addr = sys.argv[3]
            self.geth_homedir = sys.argv[4]
            self.rep_tests = int(sys.argv[5])
        elif len(sys.argv) == 7:
            self.pattern_tar_file = sys.argv[1]
            self.daemon_on_core = sys.argv[2]
            self.geth_node_addr = sys.argv[3]
            self.geth_homedir = sys.argv[4]
            self.rep_tests = int(sys.argv[5])
            self.test_sequence = sys.argv[6]
        elif len(sys.argv) != 2:
            print("usage: "+sys.argv[0]+ " <patternTarFile> [daemonOnCore - or 0-1 or 0-2 ...]")
            print("               [gethNode localhost or IP] [geth_homedir i.e ../../../turbo-geth/] [test rep i.e 10] [test seq i.e 200+30-400+10]")
            sys.exit(-1)


class PerfTest:
    """ This class manage performance test
    """
    def __init__(self, test_report, config_class):
        """ The initialization routine stop any previos server
        """
        self.test_report = test_report
        self.config_class = config_class
        self.rpc_daemon = 0
        self.silk_daemon = 0
        self.stop_rpc_daemon()
        self.stop_silk_daemon()
        self.copies_pattern_file()

    def copies_pattern_file(self):
        """ copies the vegeta pattern file into /tmp and untar zip file
        """
        cmd = "/bin/cp -f " + self.config_class.pattern_tar_file + " /tmp"
        os.system(cmd)
        print("Extracting vegeta patten ...")
        cmd = "cd /tmp; tar xvf " + self.config_class.pattern_tar_file + " > /dev/null"
        os.system(cmd)

    def start_rpc_daemon(self):
        """ Starts RPC daemon server
        """
        self.rpc_daemon = 1
        if self.config_class.daemon_on_core == "-":
            cmd = self.config_class.geth_homedir + "build/bin/rpcdaemon --private.api.addr="+self.config_class.geth_node_addr+":9090 --http.api=eth,debug,net,web3 &"
        else:
            cmd = "taskset -c " + self.config_class.daemon_on_core + " " + \
                   self.config_class.geth_homedir + "build/bin/rpcdaemon --private.api.addr="+self.config_class.geth_node_addr+":9090 --http.api=eth,debug,net,web3 &"
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
        if self.config_class.daemon_on_core == "-":
            cmd = "../../build_gcc_release/silkrpc/silkrpcdaemon --target "+self.config_class.geth_node_addr+":9090 --local "+self.config_class.geth_node_addr+":51515 --logLevel c &"
        else:
            cmd = "taskset -c " + self.config_class.daemon_on_core + \
              " ../../build_gcc_release/silkrpc/silkrpcdaemon --target "+self.config_class.geth_node_addr+":9090 --local "+self.config_class.geth_node_addr+":51515 --logLevel c &"
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


    def execute(self, index, name, qps_value, time_value):
        """ Executes the tests using qps and time variable
        """
        script_name = "./vegeta_attack_getLogs_"+name+".sh" +" "+str(qps_value) + " " + str(time_value)
        print(str(index)+") "+name+": executes test qps:", str(qps_value) + " time:"+str(time_value))
        os.system(script_name)
        self.get_result(index, name, qps_value, time_value)


    def get_result(self, test_number, name, qps_value, time_value):
        """ Processes the report file generated by vegeta and reads latency data
        """
        test_report_filename = "./getLogs_"+str(qps_value)+"qps_"+str(time_value)+"s_"+name+"_perf.hrd"
        file = open(test_report_filename)
        try:
            line = file.readlines()[2]
            newline = line.replace('\n', ' ');
            latency_values = newline.split(',')
            min_latency = latency_values[6].split(']')[1]
            mean = latency_values[7]
            fifty = latency_values[8]
            ninty = latency_values[9]
            nintyfive = latency_values[10]
            nintynine = latency_values[11]
            max_latency = latency_values[12]
        finally:
            file.close()

        self.test_report.write_test_report(name, test_number, qps_value, time_value, min_latency, mean, fifty, ninty, nintyfive, nintynine, max_latency)
        os.system("/bin/rm " + test_report_filename)


class TestReport:
    """ This class write the test report
    """

    def __init__(self, config_class):
        """
        """
        self.csv_file = ''
        self.writer = ''
        self.config_class = config_class

    def open(self):
        """ Writes on CVS file the header
        """
        csv_filename = datetime.today().strftime('%Y-%m-%d-%H:%M:%S')+"_perf.csv"
        self.csv_file = open(csv_filename, 'w', newline='')
        self.writer = csv.writer(self.csv_file)

        print("Creating report file: "+csv_filename+"\n")

        command = "sum "+ self.config_class.pattern_tar_file
        checksum = os.popen(command).read().split('\n')

        command = "gcc --version"
        gcc_vers = os.popen(command).read().split(',')

        command = "cat /proc/cpuinfo | grep 'model name' | uniq"
        model = os.popen(command).readline().split(':')
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "PC", model[1]])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "DaemonRunOnCore", self.config_class.daemon_on_core])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "VegetaFile", self.config_class.pattern_tar_file])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "VegetaChecksum", checksum[0]])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "GccVers", gcc_vers[0]])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "SilkVersion", "TBD"])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "RpcDaemon", "TBD"])
        self.writer.writerow([])
        self.writer.writerow([])
        self.writer.writerow(["Daemon", "TestNo", "Qps", "Time", "Min", "Mean", "50", "90", "95", "99", "Max"])
        self.csv_file.flush()


    def write_test_report(self, daemon, test_number, qps_value, time_value, min_latency, mean, fifty, ninty, nintyfive, nintynine, max_latency):
        """ Writes on CVS the latency data for one completed test
        """
        self.writer.writerow([daemon, str(test_number), qps_value, time_value, min_latency, mean, fifty, ninty, nintyfive, nintynine, max_latency])
        self.csv_file.flush()


    def close(self):
        """ Close the report
        """
        self.csv_file.flush()
        self.csv_file.close()


#
# main
#
# usage: python perf.py <patternTarFile> [daemonOnCore - or 0-1 or 0-2 ...] [gethNode localhost or IP]
#                       [geth_homedir i.e ../../../turbo-geth/] [test rep i.e 10] [test seq i.e 200+30-400+10]
#

CONFIG_CLASS = Config()

TEST_REPORT = TestReport(CONFIG_CLASS)

PERF_TEST = PerfTest(TEST_REPORT, CONFIG_CLASS)

print("\nTest using repetition: "+ str(CONFIG_CLASS.rep_tests) + " on sequence: " +  str(CONFIG_CLASS.test_sequence) + " for pattern: " + str(CONFIG_CLASS.pattern_tar_file))
TEST_REPORT.open()

PERF_TEST.start_silk_daemon()


TEST_SEQUENCE_STR = str(CONFIG_CLASS.test_sequence)
CURRENT_SEQ = TEST_SEQUENCE_STR.split('-')
for test in CURRENT_SEQ:
    for test_no in range(0, CONFIG_CLASS.rep_tests):
        qps = test.split('+')[0]
        time = test.split('+')[1]
        PERF_TEST.execute(test_no, "silkrpc", qps, time)
        os.system("sleep 1")
    print("")

PERF_TEST.stop_silk_daemon()
print("\n")
PERF_TEST.start_rpc_daemon()

for test in CURRENT_SEQ:
    for test_no in range(0, CONFIG_CLASS.rep_tests):
        qps = test.split('+')[0]
        time = test.split('+')[1]
        PERF_TEST.execute(test_no, "rpcdaemon", qps, time)
        os.system("sleep 1")
    print("")

PERF_TEST.stop_rpc_daemon()
TEST_REPORT.close()
sys.exit(0)

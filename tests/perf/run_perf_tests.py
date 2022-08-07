#!/usr/bin/env python3
""" This script uses Vegeta to execute a list of performance tests (configured via command line) and saves its result in CSV file
"""

# pylint: disable=consider-using-with

import os
import csv
import sys
import getopt
from datetime import datetime

DEFAULT_TEST_SEQUENCE = "50:30,200:30,500:30,700:30,1000:30,1500:30,1700:30,2000:30"
DEFAULT_REPETITIONS = 10
DEFAULT_VEGETA_PATTERN_TAR_FILE = "./vegeta/erigon_stress_test_eth_getLogs_goerly_001.tar"
DEFAULT_DAEMON_VEGETA_ON_CORE = "-:-"
DEFAULT_ERIGON_ADDRESS = "localhost:9090"
DEFAULT_ERIGON_BUILD_DIR = "../../../erigon/build/"
DEFAULT_SILKRPC_BUILD_DIR = "../../build_gcc_release/"
DEFAULT_SILKRPC_NUM_CONTEXTS = ""
DEFAULT_RPCDAEMON_ADDRESS = "localhost"
DEFAULT_TEST_MODE = "3"
DEFAULT_WAITING_TIME = "5"
DEFAULT_TEST_TYPE = "eth_getLogs"
DEFAULT_WAIT_MODE = "blocking"
DEFAULT_WORKERS = "16"

VEGETA_PATTERN_DIRNAME = "erigon_stress_test"
VEGETA_REPORT = "vegeta_report.hrd"
VEGETA_TAR_FILE_NAME = "vegeta_TAR_File"
VEGETA_PATTERN_SILKRPC_BASE = "/tmp/" + VEGETA_PATTERN_DIRNAME + "/vegeta_geth_"
VEGETA_PATTERN_RPCDAEMON_BASE = "/tmp/" + VEGETA_PATTERN_DIRNAME + "/vegeta_erigon_"

def usage(argv):
    """ Print script usage
    """
    print("Usage: " + argv[0] + " -h -p vegetaPatternTarFile -c daemonOnCore  -t erigonAddress -g erigonBuildDir -s silkrpcBuildDir -r testRepetitions - t testSequence")
    print("")
    print("Launch an automated performance test sequence on Silkrpc and RPCDaemon using Vegeta")
    print("")
    print("-h                      print this help")
    print("-D                      perf command")
    print("-z                      do not start server                                                                    ")
    print("-y                      test type (i.e eth_call, eth_logs, ...)                                                [default: " + DEFAULT_TEST_TYPE + "]")
    print("-d rpcDaemonAddress     address of daemon eg (10.1.1.20)                                                       [default: " + DEFAULT_RPCDAEMON_ADDRESS +"]")
    print("-p vegetaPatternTarFile path to the request file for Vegeta attack                                             [default: " + DEFAULT_VEGETA_PATTERN_TAR_FILE +"]")
    print("-c daemonVegetaOnCore   cpu list in taskset format for daemon & vegeta (e.g. 0-1:2-3 or 0-2:3-4 or 0,2:3,4...) [default: " + DEFAULT_DAEMON_VEGETA_ON_CORE +"]")
    print("-a erigonAddress        address of ERIGON Core component as <address>:<port> (e.g. localhost:9090)             [default: " + DEFAULT_ERIGON_ADDRESS + "]")
    print("-g erigonBuildDir       path to ERIGON build folder (e.g. ../../../erigon/build)                               [default: " + DEFAULT_ERIGON_BUILD_DIR + "]")
    print("-s silkrpcBuildDir      path to Silkrpc build folder (e.g. ../../build_gcc_release/)                           [default: " + DEFAULT_SILKRPC_BUILD_DIR + "]")
    print("-r testRepetitions      number of repetitions for each element in test sequence (e.g. 10)                      [default: " + str(DEFAULT_REPETITIONS) + "]")
    print("-t testSequence         list of query-per-sec and duration tests as <qps1>:<t1>,... (e.g. 200:30,400:10)       [default: " + DEFAULT_TEST_SEQUENCE + "]")
    print("-i                      wait mode                                                                              [default: " + DEFAULT_WAIT_MODE + "]")
    print("-n numContexts          number of Silkrpc execution contexts (i.e. 1+1 asio+grpc threads)                      [default: " + DEFAULT_SILKRPC_NUM_CONTEXTS + "]")
    print("-m mode                 tests type silkrpc(1), rpcdaemon(2) and both (3) (i.e. 3)                              [default: " + str(DEFAULT_TEST_MODE) + "]")
    print("-w                      waiting time                                                                           [default: " + DEFAULT_WAITING_TIME + "]")
    print("-o                      workers                                                                                [default: " + DEFAULT_WORKERS + "]")
    sys.exit(-1)


class Config:
    # pylint: disable=too-many-instance-attributes
    """ This class manage configuration params
    """
    def __init__(self, argv):
        """ Processes the command line contained in argv
        """
        self.vegeta_pattern_tar_file = DEFAULT_VEGETA_PATTERN_TAR_FILE
        self.daemon_vegeta_on_core = DEFAULT_DAEMON_VEGETA_ON_CORE
        self.erigon_addr = DEFAULT_ERIGON_ADDRESS
        self.erigon_builddir = DEFAULT_ERIGON_BUILD_DIR
        self.silkrpc_build_dir = DEFAULT_SILKRPC_BUILD_DIR
        self.silkrpc_num_contexts = DEFAULT_SILKRPC_NUM_CONTEXTS
        self.repetitions = DEFAULT_REPETITIONS
        self.test_sequence = DEFAULT_TEST_SEQUENCE
        self.rpc_daemon_address = DEFAULT_RPCDAEMON_ADDRESS
        self.test_mode = DEFAULT_TEST_MODE
        self.test_type = DEFAULT_TEST_TYPE
        self.waiting_time = DEFAULT_WAITING_TIME
        self.user_perf_command = ""
        self.workers = DEFAULT_WORKERS
        self.start_server = "1"
        self.wait_mode = DEFAULT_WAIT_MODE

        self.__parse_args(argv)

    def __parse_args(self, argv):
        try:
            local_config = 0
            opts, _ = getopt.getopt(argv[1:], "D:hm:d:p:c:a:g:s:r:t:n:y:zw:i:o:")

            for option, optarg in opts:
                if option in ("-h", "--help"):
                    usage(argv)
                elif option == "-m":
                    self.test_mode = optarg
                elif option == "-D":
                    self.user_perf_command = optarg
                elif option == "-d":
                    if local_config == 1:
                        print ("ERROR: incompatible option -d with -a -g -s -n")
                        usage(argv)
                    local_config = 2
                    self.rpc_daemon_address = optarg
                elif option == "-p":
                    self.vegeta_pattern_tar_file = optarg
                elif option == "-c":
                    self.daemon_vegeta_on_core = optarg
                elif option == "-a":
                    if local_config == 2:
                        print ("ERROR: incompatible option -d with -a -g -s -n")
                        usage(argv)
                    local_config = 1
                    self.erigon_addr = optarg
                elif option == "-g":
                    if local_config == 2:
                        print ("ERROR: incompatible option -d with -a -g -s -n")
                        usage(argv)
                    local_config = 1
                    self.erigon_builddir = optarg
                elif option == "-s":
                    if local_config == 2:
                        print ("ERROR: incompatible option -d with -a -g -s -n")
                        usage(argv)
                    local_config = 1
                    self.silkrpc_build_dir = optarg
                elif option == "-o":
                    self.workers = optarg
                elif option == "-r":
                    self.repetitions = int(optarg)
                elif option == "-t":
                    self.test_sequence = optarg
                elif option == "-z":
                    self.start_server = "0"
                elif option == "-w":
                    self.waiting_time = optarg
                elif option == "-y":
                    self.test_type = optarg
                elif option == "-i":
                    self.wait_mode = optarg
                elif option == "-n":
                    if local_config == 2:
                        print ("ERROR: incompatible option -d with -a -g -s -n")
                        usage(argv)
                    local_config = 1
                    self.silkrpc_num_contexts = optarg
                else:
                    usage(argv)
        except getopt.GetoptError as err:
            # print help information and exit:
            print(err)
            usage(argv)
            sys.exit(-1)


class PerfTest:
    """ This class manage performance test
    """
    def __init__(self, test_report, config):
        """ The initialization routine stop any previos server
        """
        self.test_report = test_report
        self.config = config
        self.cleanup()
        self.stop_silk_daemon()
        self.stop_rpc_daemon()
        print("\nSetup temporary daemon to verify configuration is OK")
        self.start_silk_daemon(0)
        self.stop_silk_daemon()
        self.start_rpc_daemon(0)
        self.stop_rpc_daemon()
        self.copy_pattern_file()

    def cleanup(self):
        """ cleanup
        """
        self.silk_daemon = 0
        self.rpc_daemon = 0
        cmd = "/bin/rm -f " +  " /tmp/" + VEGETA_TAR_FILE_NAME
        os.system(cmd)
        cmd = "/bin/rm -f -rf /tmp/" + VEGETA_PATTERN_DIRNAME
        os.system(cmd)
        cmd = "/bin/rm -f perf.data.old perf.data"
        os.system(cmd)

    def copy_pattern_file(self):
        """ copy the vegeta pattern file into /tmp and untar zip file
        """
        cmd = "/bin/cp -f " + self.config.vegeta_pattern_tar_file + " /tmp/" + VEGETA_TAR_FILE_NAME
        print("Copy vegeta pattern: ", cmd)
        status = os.system(cmd)
        if int(status) != 0:
            print("Copy failed. Test Aborted!")
            sys.exit(-1)

        #file_path = self.config.vegeta_pattern_tar_file
        #tokenize_path = file_path.split('/')
        #n_subdir = len(tokenize_path)
        #file_name = tokenize_path[n_subdir-1]
        #cmd = "cd /tmp; tar xvf " + file_name + " > /dev/null"
        cmd = "cd /tmp; tar xvf " + VEGETA_TAR_FILE_NAME + " > /dev/null"
        print("Extracting vegeta pattern: ", cmd)
        status = os.system(cmd)
        if int(status) != 0:
            print("untar failed. Test Aborted!")
            sys.exit(-1)
        # if address is provided substitute the address and port of daemon in the vegeta file
        if  self.config.rpc_daemon_address != "localhost":
            cmd = "sed -i 's/localhost/" + self.config.rpc_daemon_address + "/g' " + VEGETA_PATTERN_SILKRPC_BASE + self.config.test_type + ".txt"
            os.system(cmd)
            cmd = "sed -i 's/localhost/" + self.config.rpc_daemon_address + "/g' " + VEGETA_PATTERN_RPCDAEMON_BASE + self.config.test_type + ".txt"
            os.system(cmd)

    def start_rpc_daemon(self, start_test):
        """ Starts RPC daemon server
        """
        if self.config.rpc_daemon_address != "localhost":
            return
        if self.config.start_server == "0":
            if start_test:
                print ("RPCDaemon NOT started")
            return
        if self.config.test_mode == "1":
            return
        self.rpc_daemon = 1
        on_core = self.config.daemon_vegeta_on_core.split(':')
        if on_core[0] == "-":
            cmd = self.config.erigon_builddir + "bin/rpcdaemon --private.api.addr="+self.config.erigon_addr+" --http.api=eth,debug,net,web3  2>/dev/null &"
        else:
            cmd = "taskset -c " + on_core[0] + " " + \
                   self.config.erigon_builddir + "bin/rpcdaemon --private.api.addr="+self.config.erigon_addr+" --http.api=eth,debug,net,web3  2>/dev/null &"
        print("RpcDaemon starting ...: ", cmd)
        status = os.system(cmd)
        if int(status) != 0:
            print("Start rpc daemon failed: Test Aborted!")
            sys.exit(-1)
        os.system("sleep 2")
        pid = os.popen("ps aux | grep 'rpcdaemon' | grep -v 'grep' | awk '{print $2}'").read()
        if pid == "":
            print("Start rpc daemon failed: Test Aborted!")
            sys.exit(-1)

    def stop_rpc_daemon(self):
        """ Stops the RPC daemon server
        """
        if self.config.rpc_daemon_address != "localhost":
            return
        if self.config.start_server == "0":
            return
        if self.config.test_mode == "1":
            return
        self.rpc_daemon = 0
        os.system("kill -9 $(ps aux | grep 'rpcdaemon' | grep -v 'grep' | awk '{print $2}') 2> /dev/null ")
        print("RpcDaemon stopped")
        os.system("sleep 5")

    def start_silk_daemon(self, start_test):
        """ Starts SILKRPC daemon
        """
        if self.config.rpc_daemon_address != "localhost":
            return
        if self.config.start_server == "0":
            if start_test:
                print ("SilkRPCDaemon NOT started")
            return
        if self.config.test_mode == "2":
            return
        self.rpc_daemon = 1
        on_core = self.config.daemon_vegeta_on_core.split(':')
        if self.config.user_perf_command != "" and start_test == 1:
            perf_cmd = self.config.user_perf_command
        else:
            perf_cmd = ""
        wait_mode_str = " --wait_mode " + self.config.wait_mode

        base_params = self.config.silkrpc_build_dir + "cmd/silkrpcdaemon --target " + self.config.erigon_addr + " --http_port localhost:51515 --log_verbosity c --num_workers " \
                      + str(self.config.workers)

        if self.config.silkrpc_num_contexts != "":
            base_params += " --num_contexts " + str(self.config.silkrpc_num_contexts)
        if on_core[0] == "-":
            cmd = perf_cmd  + base_params + wait_mode_str + " &"
        else:
            cmd = perf_cmd + "taskset -c " + on_core[0] + " "  + base_params + wait_mode_str + " &"

        print("SilkDaemon starting ...: ", cmd)
        status = os.system(cmd)
        if int(status) != 0:
            print("Start silkrpc daemon failed: Test Aborted!")
            sys.exit(-1)
        os.system("sleep 2")
        pid = os.popen("ps aux | grep 'silkrpc' | grep -v 'grep' | awk '{print $2}'").read()
        if pid == "":
            print("Start silkrpc daemon failed: Test Aborted!")
            sys.exit(-1)

    def stop_silk_daemon(self):
        """ Stops SILKRPC daemon
        """
        if self.config.rpc_daemon_address != "localhost":
            return
        if self.config.start_server == "0":
            return
        if self.config.test_mode == "2":
            return
        self.silk_daemon = 0
        os.system("kill -2 $(ps aux | grep 'silk' | grep -v 'grep' | grep -v 'python' | awk '{print $2}') 2> /dev/null")
        print("SilkDaemon stopped")
        os.system("sleep 3")


    def execute(self, test_number, name, qps_value, time_value):
        """ Executes the tests using qps and time variable
        """
        if name == "silkrpc":
            pattern = VEGETA_PATTERN_SILKRPC_BASE + self.config.test_type + ".txt"
        else:
            pattern = VEGETA_PATTERN_RPCDAEMON_BASE + self.config.test_type + ".txt"
        on_core = self.config.daemon_vegeta_on_core.split(':')
        if on_core[1] == "-":
            cmd = "cat " + pattern + " | " \
                  "vegeta attack -keepalive -rate="+qps_value+" -format=json -duration="+time_value+"s -timeout=300s | " \
                  "vegeta report -type=text > " + VEGETA_REPORT + " &"
        else:
            cmd = "taskset -c " + on_core[1] + " cat " + pattern + " | " \
                  "taskset -c " + on_core[1] + " vegeta attack -keepalive -rate="+qps_value+" -format=json -duration="+time_value+"s -timeout=300s | " \
                  "taskset -c " + on_core[1] + " vegeta report -type=text > " + VEGETA_REPORT + " &"
        #print("\n" + cmd)
        print(test_number+" "+name+": executes test qps:", str(qps_value) + " time:"+str(time_value)+" -> ", end="")
        sys.stdout.flush()
        status = os.system(cmd)
        if int(status) != 0:
            print("vegeta test fails: Test Aborted!")
            return 0

        while 1:
            os.system("sleep 3")
            if name == "silkrpc":
                pid = os.popen("ps aux | grep 'silkrpc' | grep -v 'grep' | awk '{print $2}'").read()
            else:
                pid = os.popen("ps aux | grep 'rpcdaemon' | grep -v 'grep' | awk '{print $2}'").read()
            if pid == "":
                # the server is dead; kill vegeta and returns fails
                os.system("kill -2 $(ps aux | grep 'vegeta' | grep -v 'grep' | grep -v 'python' | awk '{print $2}') 2> /dev/null")
                return 0

            pid = os.popen("ps aux | grep 'vegeta report' | grep -v 'grep' | awk '{print $2}'").read()
            if pid == "":
                # the vegeta has terminate its works, generate report, returns OK
                self.get_result(test_number, name, qps_value, time_value)
                return 1

    def get_result(self, test_number, daemon_name, qps_value, time_value):
        """ Processes the report file generated by vegeta and reads latency data
        """
        test_report_filename = VEGETA_REPORT
        file = open(test_report_filename, encoding='utf8')
        try:
            file_raws = file.readlines()
            newline = file_raws[2].replace('\n', ' ')
            latency_values = newline.split(',')
            min_latency = latency_values[6].split(']')[1]
            max_latency = latency_values[12]
            newline = file_raws[5].replace('\n', ' ')
            ratio = newline.split(' ')[34]
            if len(file_raws) > 8:
                error = file_raws[8]
                print(" [ Ratio="+ratio+", MaxLatency="+max_latency+ " Error: " + error +"]")
            else:
                error = ""
                print(" [ Ratio="+ratio+", MaxLatency="+max_latency+"]")
            threads = os.popen("ps -efL | grep erigon | grep bin | wc -l").read().replace('\n', ' ')
        finally:
            file.close()

        self.test_report.write_test_report(daemon_name, test_number, threads, qps_value, time_value, min_latency, latency_values[7], latency_values[8], \
                                           latency_values[9], latency_values[10], latency_values[11], max_latency, ratio, error)
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
        csv_filename = "/tmp/" + datetime.today().strftime('%Y-%m-%d-%H:%M:%S')+"_perf.csv"
        self.csv_file = open(csv_filename, 'w', newline='', encoding='utf8')
        self.writer = csv.writer(self.csv_file)

        print("Creating report file: "+csv_filename+"\n")

        command = "sum "+ self.config.vegeta_pattern_tar_file
        checksum = os.popen(command).read().split('\n')

        command = "gcc --version"
        gcc_vers = os.popen(command).read().split(',')

        command = "go version"
        go_vers = os.popen(command).read().replace('\n', '')

        command = "uname -r"
        kern_vers = os.popen(command).read().replace('\n', "").replace('\'', '')

        command = "cat /proc/cpuinfo | grep 'model name' | uniq"
        model = os.popen(command).readline().replace('\n', ' ').split(':')
        command = "cat /proc/cpuinfo | grep 'bogomips' | uniq"
        tmp = os.popen(command).readline().replace('\n', '').split(':')[1]
        bogomips = tmp.replace(' ', '')

        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "PC", model[1]])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "Bogomips", bogomips])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "Kernel", kern_vers])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "DaemonVegetaRunOnCore", self.config.daemon_vegeta_on_core])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "TG address", self.config.erigon_addr])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "VegetaFile", self.config.vegeta_pattern_tar_file])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "VegetaChecksum", checksum[0]])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "GccVers", gcc_vers[0]])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "GoVers", go_vers])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "SilkVersion", "master"])
        self.writer.writerow(["", "", "", "", "", "", "", "", "", "", "", "", "RpcDaemon/TG", "master"])
        self.writer.writerow([])
        self.writer.writerow([])
        self.writer.writerow(["Daemon", "TestNo", "TG-Threads", "Qps", "Time", "Min", "Mean", "50", "90", "95", "99", "Max", "Ratio", "Error"])
        self.csv_file.flush()

    def write_test_report(self, daemon, test_number, threads, qps_value, time_value, min_latency, mean, fifty, ninty, nintyfive, nintynine, max_latency, ratio, error):
        """ Writes on CVS the latency data for one completed test
        """
        self.writer.writerow([daemon, str(test_number), threads, qps_value, time_value, min_latency, mean, fifty, ninty, nintyfive, nintynine, max_latency, ratio, error])
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

    current_sequence = str(config.test_sequence).split(',')


    if config.test_mode in ("1", "3"):
        perf_test.start_silk_daemon(1)
        test_number = 1
        for test in current_sequence:
            for test_rep in range(0, config.repetitions):
                qps = test.split(':')[0]
                time = test.split(':')[1]
                test_name = "[{:d}.{:2d}] "
                test_name_formatted = test_name.format(test_number, test_rep+1)
                result = perf_test.execute(test_name_formatted, "silkrpc", qps, time)
                if result == 0:
                    print("Server dead test Aborted!")
                    test_report.close()
                    sys.exit(-1)
                cmd = "sleep " + config.waiting_time
                os.system(cmd)
            test_number = test_number + 1
            print("")

        perf_test.stop_silk_daemon()
        if config.test_mode == "3":
            print("--------------------------------------------------------------------------------------------\n")

    if config.test_mode in ("2", "3"):
        perf_test.start_rpc_daemon(1)

        test_number = 1
        for test in current_sequence:
            for test_rep in range(0, config.repetitions):
                qps = test.split(':')[0]
                time = test.split(':')[1]
                test_name = "[{:d}.{:2d}] "
                test_name_formatted = test_name.format(test_number, test_rep+1)
                result = perf_test.execute(test_name_formatted, "rpcdaemon", qps, time)
                if result == 0:
                    print("Server dead test Aborted!")
                    test_report.close()
                    sys.exit(-1)
                cmd = "sleep " + config.waiting_time
                os.system(cmd)
            test_number = test_number + 1
            print("")

        perf_test.stop_rpc_daemon()

    test_report.close()
    print("Test Terminated successfully.")


#
# module as main
#
if __name__ == "__main__":
    main(sys.argv)
    sys.exit(0)

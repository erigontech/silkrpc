#!/usr/bin/python3
""" Run the JSON RPC API curl commands as integration tests """

import json
import shlex
import subprocess
import sys
import getopt


def run_shell_command(command: str, expected_response: str, exit_on_fail) -> int:
    """ Run the specified command as shell. """
    command_and_args = shlex.split(command)
    process = subprocess.run(command_and_args, stdout=subprocess.PIPE,
                             universal_newlines=True, check=True)
    if process.returncode != 0:
        sys.exit(process.returncode)
    process.stdout = process.stdout.strip('\n')
    response = json.loads(process.stdout)

    if "error" in response:
        print("--> KO: ", command_and_args, " Error: ", response["error"], "\n")
        if exit_on_fail:
            sys.exit(1)
    elif expected_response["result"] is not None and response != expected_response:
        print("--> KO: Unexpected Result", command_and_args)
        print("Response: ", response)
        print("ExpRsp: ", expected_response, "\n")
        if exit_on_fail:
            sys.exit(1)

def run_tests(json_filename, verbose, silk, exit_on_fail, req_test):
    """ Run integration tests. """
    with open(json_filename) as json_file:
        jsonrpc_commands = json.load(json_file)
        test_number = 0
        for json_rpc in jsonrpc_commands:
            if req_test in (-1, test_number):
                request = json.dumps(json_rpc["request"])
                if verbose:
                    print (str(test_number) + ") " + request)
                response = json_rpc["response"]
                if silk:
                    run_shell_command(
                         '''curl --silent -X POST -H "Content-Type: application/json" --data \'''' +
                         request + '''\' localhost:51515''',
                         response, exit_on_fail)
                else:
                    run_shell_command(
                         '''curl --silent -X POST -H "Content-Type: application/json" --data \'''' +
                         request + '''\' localhost:8545''',
                         response, exit_on_fail)
            test_number = test_number + 1
#
# usage
#
def usage(argv):
    """ Print script usage
    """
    print("Usage: " + argv[0] + " -h -c -r -v")
    print("")
    print("Launch an automated test sequence on Silkrpc or RPCDaemon")
    print("")
    print("-h print this help")
    print("-c runs all tests even if one test fails [ default exit at first test fail ]")
    print("-t test_number (-1 all test)")
    print("-r connect to rpcdaemon [ default connect to silk ] ")
    print("-v verbose")



#
# main
#
def main(argv):
    """ parse command line and execute tests
    """
    exit_on_fail = 1
    silk = 1
    verbose = 0
    req_test = -1

    try:
        opts, _ = getopt.getopt(argv[1:], "hrcvt:")
        for option, optarg in opts:
            if option in ("-h", "--help"):
                usage(argv)
                sys.exit(-1)
            elif option == "-c":
                exit_on_fail = 0
            elif option == "-r":
                silk = 0
            elif option == "-v":
                verbose = 1
            elif option == "-t":
                req_test = int(optarg)
            else:
                usage(argv)
                sys.exit(-1)

    except getopt.GetoptError as err:
        # print help information and exit:
        print(err)
        usage(argv)
        sys.exit(-1)

    run_tests('./jsonrpc_commands_goerli.json', verbose, silk, exit_on_fail, req_test)

#
# module as main
#
if __name__ == "__main__":
    main(sys.argv)
    sys.exit(0)

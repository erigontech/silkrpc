#!/usr/bin/python3
""" Run the JSON RPC API curl commands as integration tests """

import json
import shlex
import subprocess
import sys
import os

import getopt
import jsondiff


def get_target(silk: bool, method: str):
    "Determine where silkrpc is supposed to be serving at."
    if "engine_" in method:
        return "localhost:8550"
    if silk:
        return "localhost:51515"

    return "localhost:8545"

def run_shell_command(command: str, expected_response: str, verbose: bool, exit_on_fail: bool):
    """ Run the specified command as shell. If exact result or error don't care, they are null but present in expected_response. """
    command_and_args = shlex.split(command)
    process = subprocess.run(command_and_args, stdout=subprocess.PIPE, universal_newlines=True, check=True)
    if process.returncode != 0:
        sys.exit(process.returncode)
    process.stdout = process.stdout.strip('\n')
    response = json.loads(process.stdout)
    if response != expected_response:
        if "result" in response and "result" in expected_response and expected_response["result"] is None:
            # response and expected_response are different but don't care
            return
        if "error" in response and "error" in expected_response and expected_response["error"] is None:
            # response and expected_response are different but don't care
            return
        response_diff = jsondiff.diff(expected_response, response)
        print(f"--> KO: unexpected result for command: {command}\n--> DIFF expected-received: {response_diff}")
        if verbose:
            print(f"\n--> expected_response: {expected_response}")
            print(f"\n--> response: {response}")
        if exit_on_fail:
            sys.exit(1)

def run_tests(json_filename: str, verbose: bool, silk: bool, exit_on_fail: bool, req_test: int):
    """ Run integration tests. """
    with open(json_filename, encoding='utf8') as json_file:
        jsonrpc_commands = json.load(json_file)
        test_number = 0
        for json_rpc in jsonrpc_commands:
            if req_test in (-1, test_number):
                request = json_rpc["request"]
                request_dumps = json.dumps(request)
                target = get_target(silk, request["method"])
                if verbose:
                    print (str(test_number) + ") " + request_dumps)
                response = json_rpc["response"]
                run_shell_command(
                    '''curl --silent -X POST -H "Content-Type: application/json" --data \'''' +
                    request_dumps + '''\' ''' + target,
                    response,
                    verbose,
                    exit_on_fail)
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
    print("-t api_name")
    print("-r connect to rpcdaemon [ default connect to silk ] ")
    print("-l number of loops")
    print("-v verbose")


#
# main
#
def main(argv):
    """ parse command line and execute tests
    """
    exit_on_fail = 1
    silk = 1
    loop_number = 1
    verbose = 0
    req_test = -1
    api_name = ""

    try:
        opts, _ = getopt.getopt(argv[1:], "hrcvt:l:")
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
                api_name = optarg
            elif option == "-l":
                loop_number = int(optarg)
            else:
                usage(argv)
                sys.exit(-1)

    except getopt.GetoptError as err:
        # print help information and exit:
        print(err)
        usage(argv)
        sys.exit(-1)

    for test_rep in range(0, loop_number):
        if verbose:
            print("Test iteration: ", test_rep + 1)
        dirs = os.listdir('./json')
        for api_file in dirs:
            test_dir = "./json/" + api_file
            test_lists = os.listdir(test_dir)

            if api_name in ("", api_file):
                print ("Testing API: " + api_file)
                for test_name in test_lists:
                    test_file = test_dir+"/"+test_name
                    run_tests(test_file, verbose, silk, exit_on_fail, req_test)

#
# module as main
#
if __name__ == "__main__":
    main(sys.argv)
    sys.exit(0)

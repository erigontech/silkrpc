#!/usr/bin/python3
""" Run the JSON RPC API curl commands as integration tests """

import json
import shlex
import subprocess
import sys
import getopt
import jsondiff

def get_silkrpc_target(silk: bool, method: str):
    "Determine where silkrpc is supposed to be serving at."
    if "engine_" in method:
        return "localhost:8550"
    elif silk:
        return "localhost:51515"
    else:
        return "localhost:8545"

def run_shell_command(command: str, expected_response: str, exit_on_fail):
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
        print("--> KO: unexpected result for command: {0}\n--> DIFF expected-received: {1}".format(command, response_diff))
        if exit_on_fail:
            sys.exit(1)

def run_tests(json_filename, verbose, silk, exit_on_fail, req_test):
    """ Run integration tests. """
    with open(json_filename, encoding='utf8') as json_file:
        jsonrpc_commands = json.load(json_file)
        test_number = 0
        for json_rpc in jsonrpc_commands:
            if req_test in (-1, test_number):
                request = json_rpc["request"]
                request_dumps = json.dumps(request)
                target = get_silkrpc_target(silk, request["method"])
                if verbose:
                    print (str(test_number) + ") " + request_dumps)
                response = json_rpc["response"]
                run_shell_command(
                    '''curl --silent -X POST -H "Content-Type: application/json" --data \'''' +
                    request_dumps + '''\' ''' + target,
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
                req_test = int(optarg)
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
        run_tests('./jsonrpc_commands_goerli.json', verbose, silk, exit_on_fail, req_test)

#
# module as main
#
if __name__ == "__main__":
    main(sys.argv)
    sys.exit(0)

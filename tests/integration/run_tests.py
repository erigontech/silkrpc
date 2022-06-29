#!/usr/bin/python3
""" Run the JSON RPC API curl commands as integration tests """

import json
import shlex
import subprocess
import sys
import os
import shutil
import tarfile

import getopt
import jsondiff


def get_target(silk: bool, method: str):
    "Determine where silkrpc is supposed to be serving at."
    if "engine_" in method:
        return "localhost:8550"
    if silk:
        return "localhost:51515"

    return "localhost:8545"

def run_shell_command(command: str, command1: str, expected_response: str, verbose: bool, exit_on_fail: bool, output_dir: str, silk_file: str,
                      rpc_file: str, diff_file: str):
    """ Run the specified command as shell. If exact result or error don't care, they are null but present in expected_response. """

    command_and_args = shlex.split(command)
    process = subprocess.run(command_and_args, stdout=subprocess.PIPE, universal_newlines=True, check=True)
    if process.returncode != 0:
        sys.exit(process.returncode)
    process.stdout = process.stdout.strip('\n')
    response = json.loads(process.stdout)
    if command1 != "":
        command_and_args = shlex.split(command1)
        process = subprocess.run(command_and_args, stdout=subprocess.PIPE, universal_newlines=True, check=True)
        if process.returncode != 0:
            sys.exit(process.returncode)
        process.stdout = process.stdout.strip('\n')
        expected_response = json.loads(process.stdout)

    if response != expected_response:
        if (silk_file != "" and os.path.exists(output_dir) == 0):
            os.mkdir (output_dir)
        if silk_file != "":
            with open(silk_file, 'w', encoding='utf8') as json_file_ptr:
                json_file_ptr.write(json.dumps(response, indent = 6))
        if rpc_file != "":
            with open(rpc_file, 'w', encoding='utf8') as json_file_ptr:
                json_file_ptr.write(json.dumps(expected_response, indent = 6))
        if "result" in response and "result" in expected_response and expected_response["result"] is None:
            # response and expected_response are different but don't care
            return
        if "error" in response and "error" in expected_response and expected_response["error"] is None:
            # response and expected_response are different but don't care
            return
        response_diff = jsondiff.diff(expected_response, response, marshal=True)
        if diff_file != "":
            with open(diff_file, 'w', encoding='utf8') as json_file_ptr:
                json_file_ptr.write(json.dumps(response_diff, indent = 6))
        print(f"--> KO: unexpected result for command: {command}\n--> DIFF expected-received: {response_diff}")
        if verbose:
            print(f"\n--> expected_response: {expected_response}")
            print(f"\n--> response: {response}")
        if exit_on_fail:
            sys.exit(1)

def run_tests(test_dir: str, output_dir: str, json_file: str, verbose: bool, silk: bool, exit_on_fail: bool, test_number: int, verify_with_rpc: bool):
    """ Run integration tests. """
    json_filename = test_dir + json_file
    ext = os.path.splitext(json_file)[1]

    if ext in (".zip", ".tar"):
        with tarfile.open(json_filename,encoding='utf-8') as tar:
            files = tar.getmembers()
            if len(files) != 1:
                print ("bad archive file " + json_filename)
                sys.exit(1)
            file = tar.extractfile(files[0])
            buff = file.read()
            tar.close()
            jsonrpc_commands = json.loads(buff)
    else:
        with open(json_filename, encoding='utf8') as json_file_ptr:
            jsonrpc_commands = json.load(json_file_ptr)
    for json_rpc in jsonrpc_commands:
        request = json_rpc["request"]
        request_dumps = json.dumps(request)
        target = get_target(silk, request["method"])
        if verbose:
            print (str(test_number) + ") " + request_dumps)
        if verify_with_rpc == 0:
            cmd = '''curl --silent -X POST -H "Content-Type: application/json" --data \'''' + request_dumps + '''\' ''' + target
            cmd1 = ""
            response = json_rpc["response"]
            output_dir_name = ""
            silk_file = ""
            rpc_file = ""
            diff_file = ""
        else:
            output_api_filename = output_dir + json_file[:-5]
            output_dir_name = output_api_filename[:output_api_filename.rfind("/")]
            response = ""
            target = get_target(1, request["method"])
            cmd = '''curl --silent -X POST -H "Content-Type: application/json" --data \'''' + request_dumps + '''\' ''' + target
            target1 = get_target(0, request["method"])
            cmd1 = '''curl --silent -X POST -H "Content-Type: application/json" --data \'''' + request_dumps + '''\' ''' + target1
            silk_file = output_api_filename + "-silk.json"
            rpc_file = output_api_filename + "-rpcdaemon.json"
            diff_file = output_api_filename + "-diff.json"
            run_shell_command(
                cmd,
                cmd1,
                response,
                verbose,
                exit_on_fail,
                output_dir_name,
                silk_file,
                rpc_file,
                diff_file)

#
# usage
#
def usage(argv):
    """ Print script usage
    """
    print("Usage: " + argv[0] + " -h -c -r -v -a <api_name> -t < test_number> -l < no of loops> -d")
    print("")
    print("Launch an automated test sequence on Silkrpc or RPCDaemon")
    print("")
    print("-h print this help")
    print("-c runs all tests even if one test fails [ default exit at first test fail ]")
    print("-r connect to rpcdaemon [ default connect to silk ] ")
    print("-l <number of loops>")
    print("-a <test api >")
    print("-t <test_number>")
    print("-d verify real time with rpc")
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
    verify_with_rpc = 0
    json_dir = "./goerly/"
    output_dir = "./int_test_results/"

    try:
        opts, _ = getopt.getopt(argv[1:], "hrcvt:l:a:d")
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
            elif option == "-a":
                api_name = optarg
            elif option == "-l":
                loop_number = int(optarg)
            elif option == "-d":
                verify_with_rpc = 1
            else:
                usage(argv)
                sys.exit(-1)

    except getopt.GetoptError as err:
        # print help information and exit:
        print(err)
        usage(argv)
        sys.exit(-1)

    if os.path.exists(output_dir):
        shutil.rmtree(output_dir)

    os.mkdir (output_dir)
    for test_rep in range(0, loop_number):
        if verbose:
            print("Test iteration: ", test_rep + 1)
        dirs = sorted(os.listdir(json_dir))
        test_number = 0
        for api_file in dirs:
            test_dir = json_dir + api_file
            test_lists = sorted(os.listdir(test_dir))
            for test_name in test_lists:
                if api_name in ("", api_file):
                    test_file = api_file + "/" + test_name
                    if req_test in (-1, test_number):
                        if verbose:
                            print("Test name: ", test_file)
                        run_tests(json_dir, output_dir, test_file, verbose, silk, exit_on_fail, test_number, verify_with_rpc)
                test_number = test_number + 1

#
# module as main
#
if __name__ == "__main__":
    main(sys.argv)
    sys.exit(0)

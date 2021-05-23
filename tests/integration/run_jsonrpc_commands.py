#!/usr/bin/python3
""" Run the JSON RPC API curl commands as integration tests """

import json
import shlex
import subprocess
import sys

def run_shell_command(command: str, expected_response: str) -> int:
    """ Run the specified command as shell. """
    command_and_args = shlex.split(command)
    process = subprocess.run(command_and_args, stdout=subprocess.PIPE, universal_newlines=True, check=True)
    if process.returncode != 0:
        sys.exit(process.returncode)
    process.stdout = process.stdout.strip('\n')
    response = json.loads(process.stdout)

    if expected_response["result"] is not None and response != expected_response:
        print("KO: unexpected result for command: {0}\nexpected: {1}\nreceived: {2}".format(command, expected_response, response))
        sys.exit(1)
    if "error" in response:
        print("KO: error {0} for command: {1}".format(response["error"], command))
        sys.exit(1)

def run_tests(json_filename):
    """ Run integration tests. """
    with open(json_filename) as json_file:
        jsonrpc_commands = json.load(json_file)
        for json_rpc in jsonrpc_commands:
            request = json.dumps(json_rpc["request"])
            response = json_rpc["response"]
            run_shell_command('''curl --silent -X POST -H "Content-Type: application/json" --data \'''' + request + '''\' localhost:51515''', response)

run_tests('./jsonrpc_commands_goerli.json')

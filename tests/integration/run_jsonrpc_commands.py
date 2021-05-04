#!/usr/bin/python3
""" Run the JSON RPC API curl commands as integration tests """

# pylint: disable=line-too-long

import json
import shlex
import subprocess
import sys

def run_shell_command(command: str) -> int:
    """ Run the specified command as shell. """
    command_and_args = shlex.split(command)
    process = subprocess.run(command_and_args, stdout=subprocess.PIPE, universal_newlines=True, check=True)
    if process.returncode != 0:
        sys.exit(process.returncode)
    response = json.loads(process.stdout)
    if "error" in response:
        print("KO: ", command_and_args, " ERROR: ", response["error"])
        sys.exit(1)

run_shell_command('''curl --silent -X POST -H "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"web3_clientVersion","params":[],"id":1}' localhost:51515''')
run_shell_command('''curl --silent -X POST -H "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"web3_sha3","params":["0x00"],"id":1}' localhost:51515''')

run_shell_command('''curl --silent -X POST -H "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"net_listening","params":[],"id":1}' localhost:51515''')
run_shell_command('''curl --silent -X POST -H "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"net_peerCount","params":[],"id":1}' localhost:51515''')
run_shell_command('''curl --silent -X POST -H "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"net_version","params":[],"id":1}' localhost:51515''')

run_shell_command('''curl --silent -X POST -H "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"eth_blockNumber","params":[],"id":1}' localhost:51515''')
run_shell_command('''curl --silent -X POST -H "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"eth_chainId","params":[],"id":1}' localhost:51515''')
run_shell_command('''curl --silent -X POST -H "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"eth_protocolVersion","params":[],"id":1}' localhost:51515''')
run_shell_command('''curl --silent -X POST -H "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"eth_syncing","params":[],"id":1}' localhost:51515''')

run_shell_command('''curl --silent -X POST -H "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"eth_getBlockByHash","params":["0xd268bdabee5eab4914d0de9b0e0071364582cfb3c952b19727f1ab429f4ba2a8", true],"id":25388}' localhost:51515''')
run_shell_command('''curl --silent -X POST -H "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"eth_getBlockByNumber","params":["0x41b57c", true],"id":25388}' localhost:51515''')
run_shell_command('''curl --silent -X POST -H "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"eth_getBlockTransactionCountByNumber","params":["0x41b57c"],"id":1}' localhost:51515''')
run_shell_command('''curl --silent -X POST -H "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"eth_getBlockTransactionCountByHash","params":["0x814672e6913a3879217169c6ba461114fa032d9510a56a409d3aab19f668e299"],"id":1}' localhost:51515''')

run_shell_command('''curl --silent -X POST -H "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"eth_getUncleByBlockNumberAndIndex","params":["0x41b57c", "0x0"],"id":1}' localhost:51515''')

run_shell_command('''curl --silent -X POST -H "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"eth_getLogs","params":[{"fromBlock": "0x3d0900", "toBlock": "0x3d0964", "address": "0x2a89f54a9f8e727a7be754fd055bb8ea93d0557d"}],"id":3}' localhost:51515''')

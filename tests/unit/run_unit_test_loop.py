#!/usr/bin/env python3
""" Stress the unit test suite on multiple compiler + build configurations
"""

import getopt
import os
import sys

DEFAULT_TEST_NAME : str = None
DEFAULT_NUM_ITERATIONS : int = 1000

SILKRPC_CLANG_COVERAGE_BUILDDIR : str = "../../build_clang_coverage/"
SILKRPC_CLANG_DEBUG_BUILDDIR : str = "../../build_clang_debug/"
SILKRPC_CLANG_RELEASE_BUILDDIR : str = "../../build_clang_release/"
SILKRPC_GCC_DEBUG_BUILDDIR : str = "../../build_gcc_debug/"
SILKRPC_GCC_RELEASE_BUILDDIR : str = "../../build_gcc_release/"

class UnitTest:
    """ The unit test executable """

    def __init__(self, name: str, build_dir: str):
        """ Create a new unit test execution """
        self.name = name
        self.build_dir = build_dir

    def execute(self, num_iterations: int, test_name: str = None) -> None:
        """ Execute the unit tests `num_iterations` times """
        cmd = self.build_dir + "cmd/unit_test"
        if test_name is not None:
            cmd = cmd + " \"" + test_name + "\""
        print(cmd + "\n")

        print("Unit test stress for " + self.name + " STARTED")
        for i in range(num_iterations):
            status = os.system(cmd)
            if status != 0:
                print("Unit test stress for " + self.name + " FAILED [i=" + str(i) + "]")
                sys.exit(-1)
        print("Unit test stress for " + self.name + " COMPLETED [" + str(num_iterations) + "]")


def usage(argv):
    """ Print usage """
    print("Usage: " + argv[0] + " [-h] [-i iterations] [-t test]")
    print("")
    print("Launch an automated unit test sequence on all compiler/build configurations")
    print("")
    print("-h\tprint this help")
    print("-i\titerations")
    print("  \tthe number of iterations to perform for each configuration (default: 1000)")
    print("-t\ttest")
    print("  \tthe name of the unique TEST to execute (default: run all tests)")
    sys.exit(0)

#
# main
#
def main(argv):
    """ Main entry point
    """
    opts, _ = getopt.getopt(argv[1:], "hi:t:")

    test_name = DEFAULT_TEST_NAME
    iterations = DEFAULT_NUM_ITERATIONS

    for option, optarg in opts:
        if option in ("-h", "--help"):
            usage(argv)
        elif option == "-i":
            iterations = int(optarg)
        elif option == "-t":
            test_name = optarg

    unit_tests = [
        UnitTest("CLANG_COVERAGE", SILKRPC_CLANG_COVERAGE_BUILDDIR),
        UnitTest("CLANG_DEBUG", SILKRPC_CLANG_DEBUG_BUILDDIR),
        UnitTest("CLANG_RELEASE", SILKRPC_CLANG_RELEASE_BUILDDIR),
        UnitTest("GCC_DEBUG", SILKRPC_GCC_DEBUG_BUILDDIR),
        UnitTest("GCC_RELEASE", SILKRPC_GCC_RELEASE_BUILDDIR),
    ]

    for test in unit_tests:
        test.execute(iterations, test_name)


#
# module as main
#
if __name__ == "__main__":
    main(sys.argv)
    sys.exit(0)

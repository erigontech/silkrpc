#!/usr/bin/env python3
""" Stress the unit test suite on multiple compiler + build configurations
"""

from enum import Enum
import getopt
import os
import sys
from typing import Dict, List

SILKRPC_CLANG_COVERAGE_BUILDDIR : str = "../../build_clang_coverage/"
SILKRPC_CLANG_DEBUG_BUILDDIR : str = "../../build_clang_debug/"
SILKRPC_CLANG_RELEASE_BUILDDIR : str = "../../build_clang_release/"
SILKRPC_GCC_DEBUG_BUILDDIR : str = "../../build_gcc_debug/"
SILKRPC_GCC_RELEASE_BUILDDIR : str = "../../build_gcc_release/"

class Build(str, Enum):
    """ The build configuration """
    CLANG_COVERAGE = 'CLANG_COVERAGE'
    CLANG_DEBUG = 'CLANG_DEBUG'
    CLANG_RELEASE = 'CLANG_RELEASE'
    GCC_DEBUG = 'GCC_DEBUG'
    GCC_RELEASE = 'GCC_RELEASE'

    @classmethod
    def has_item(cls, name: str) -> bool:
        """ Return true if name is a valid enumeration item, false otherwise """
        return name in Build._member_names_ # pylint: disable=no-member

class UnitTest:
    """ The unit test executable """

    def __init__(self, build_config: Build, build_dir: str):
        """ Create a new unit test execution """
        self.build_config = build_config
        self.build_dir = build_dir

    def execute(self, num_iterations: int, test_name: str = None) -> None:
        """ Execute the unit tests `num_iterations` times """
        cmd = self.build_dir + "cmd/unit_test"
        if test_name is not None:
            cmd = cmd + " \"" + test_name + "\""
        print("Unit test runner: " + cmd + "\n")

        print("Unit test stress for " + self.build_config.name + " STARTED")
        for i in range(num_iterations):
            print("Unit test stress for " + self.build_config.name + " RUN [i=" + str(i) + "]")
            status = os.system(cmd)
            if status != 0:
                print("Unit test stress for " + self.build_config.name + " FAILED [i=" + str(i) + "]")
                sys.exit(-1)
        print("Unit test stress for " + self.build_config.name + " COMPLETED [" + str(num_iterations) + "]")

        if self.build_config.name == Build.CLANG_COVERAGE:
            os.remove('default.profraw')

UNIT_TESTS_BY_BUILD : Dict[Build, UnitTest] = {
    Build.CLANG_COVERAGE : UnitTest(Build.CLANG_COVERAGE, SILKRPC_CLANG_COVERAGE_BUILDDIR),
    Build.CLANG_DEBUG : UnitTest(Build.CLANG_DEBUG, SILKRPC_CLANG_DEBUG_BUILDDIR),
    Build.CLANG_RELEASE : UnitTest(Build.CLANG_RELEASE, SILKRPC_CLANG_RELEASE_BUILDDIR),
    Build.GCC_DEBUG : UnitTest(Build.GCC_DEBUG, SILKRPC_GCC_DEBUG_BUILDDIR),
    Build.GCC_RELEASE: UnitTest(Build.GCC_RELEASE, SILKRPC_GCC_RELEASE_BUILDDIR)
}

DEFAULT_TEST_NAME : str = None
DEFAULT_NUM_ITERATIONS : int = 1000
DEFAULT_BUILD_NAMES : List[str] = [
    Build.CLANG_COVERAGE,
    Build.CLANG_DEBUG,
    Build.CLANG_RELEASE,
    Build.GCC_DEBUG,
    Build.GCC_RELEASE
]

def usage(argv):
    """ Print usage """
    print("Usage: " + argv[0] + " [-h] [-b builds] [-i iterations] [-t test]")
    print("")
    print("Launch an automated unit test sequence on all compiler/build configurations")
    print("")
    print("-h\tprint this help")
    print("-b\tbuilds")
    print("  \tthe list of build configurations separated by comma (default: " + ",".join(DEFAULT_BUILD_NAMES) + ")")
    print("-i\titerations")
    print("  \tthe number of iterations to perform for each configuration (default: " + str(DEFAULT_NUM_ITERATIONS) + ")")
    print("-t\ttest")
    print("  \tthe name of the unique TEST to execute (default: run all tests)")
    sys.exit(0)

#
# main
#
def main(argv):
    """ Main entry point """
    opts, _ = getopt.getopt(argv[1:], "hb:i:t:")

    test_name = DEFAULT_TEST_NAME
    iterations = DEFAULT_NUM_ITERATIONS
    build_names = DEFAULT_BUILD_NAMES

    for option, optarg in opts:
        if option in ("-h", "--help"):
            usage(argv)
        elif option == "-b":
            build_names = str(optarg).split(",")
        elif option == "-i":
            iterations = int(optarg)
        elif option == "-t":
            test_name = optarg

    for build_name in build_names:
        build_name = build_name.upper()
        if not Build.has_item(build_name):
            print("Invalid build name [" + build_name + "], ignored")
            continue
        build = Build[build_name]
        unit_test = UNIT_TESTS_BY_BUILD[build]
        unit_test.execute(iterations, test_name)


#
# module as main
#
if __name__ == "__main__":
    main(sys.argv)
    sys.exit(0)

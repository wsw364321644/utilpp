#pragma once
#include "std_ext.h"
#include <cstdlib>
enum class ESimpleExitCode : int {
    Success = EXIT_SUCCESS,
    Failure = EXIT_FAILURE,
    InvalidArguments = 2,
    NetworkError = 3,
    IOError = 3
};

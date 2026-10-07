#pragma once

// passed to build.c as "--<name>" to turn that sanitizer on
// passed to the test program as "<name>" to trigger the bug that sanitizer should catch
#define SANITIZER_TEST_ARG_UNDEFINED_BEHAVIOR	"undefined"
#define SANITIZER_TEST_ARG_MEMORY				"memory"
#define SANITIZER_TEST_ARG_ADDRESS				"address"
#define SANITIZER_TEST_ARG_LEAK					"leak"
#define SANITIZER_TEST_ARG_THREAD				"thread"

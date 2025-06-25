# Contributing

We are accepting PRs for Builder!

## House Rules

* Your PR must be submitted in the form of a fork.
* Your PR fixes one of the issues in the repo's [issues list](https://github.com/dangmoody/Builder/issues).
* I am grateful for any and all PRs that get submitted, but it's entirely my discretion as to whether or not the PR gets accepted.  I will do my best to work with you on the changes before rejecting a PR.

## Developer Setup

### Pre-requisites

You'll need the same version of Clang installed that Builder is currently building with as a separate install.  This is because you need a compiler to build Builder with.  You can check which version Builder is using inside `builder.h`.

#### Linux

You'll need `libuuid`.  We are working to remove this dependency in future, but for now you will need it.

### Compiling Builder

For those using Visual Studio:
1. Open `builder.sln` and build the `builder` project.
2. You should now be good to go.

Builder can also be compiled without relying Visual Studio:
1. Run `build.bat <config>` where `<config>` is either `debug` or `release`.

To build and run the tests:
1. If you are using Visual Studio:
	a. Set the `tests` project as the startup project and run it.
2. If you are NOT using Visual Studio:
	a. Run `build_tests.bat <config>` and then run `bin/<platform>/<config>/builder_tests.exe`.

This should be all you need to get setup, compiling, and running Builder.
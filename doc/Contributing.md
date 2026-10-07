# Contributing

We are accepting PRs for Builder!


## House Rules

* Your PR must be submitted in the form of a fork to the `dev` branch.
* I am grateful for any and all PRs that get submitted, but it's entirely my discretion as to whether or not the PR gets accepted.
	* I will do my best to work with you on the changes before I potentially reject it.


## Git Branches

The naming convention for branches is `lower-case-separated-by-hyphens`.

* Development branches are prefixed `dev-`.
* Experimental branches are prefixed `exp-`.


## Developer Setup

Run `scripts/download_dependencies` (there are both Batch and Bash variants depending on what platform you're developing on).

### Windows

If you want to use Visual Studio, then run `generate_solution.exe` to generate a solution.

### Linux

You'll need `libuuid` (we are working to remove this dependency in future, but for now you will need it).


### Building and runing the tests:

Build the tests by running the `build_tests` executable.

Run the tests by running the `builder_tests` executable.

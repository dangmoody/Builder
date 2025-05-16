#if BUILDER_DOING_USER_CONFIG_BUILD

#include <builder.h>

BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {
	BuildConfig win64Common = {
		.binary_name			= "SDL",
		.binary_folder			= "bin",
		.binary_type			= BINARY_TYPE_DYNAMIC_LIBRARY,
		.source_files			= { "src/*.c" },
		.defines				= { "_CRT_SECURE_NO_WARNINGS" },
		.additional_includes = {
			"src",	// this feels dirty, are we sure we want to do this?
			"include",
			"include/build_config",
		},
		.ignore_warnings = {
			"-Wno-pedantic",
			"-Wno-pre-c11-compat",
			"-Wno-reserved-identifier",
			"-Wno-reserved-macro-identifier",
			"-Wno-documentation-unknown-command",
			"-Wno-nonportable-system-include-path",
			"-Wno-microsoft-include",
			"-Wno-microsoft-enum-value",
			"-Wno-microsoft-flexible-array",
			"-Wno-sign-conversion",
			"-Wno-sign-compare",
			"-Wno-implicit-int-conversion",
			"-Wno-implicit-int-float-conversion",
			"-Wno-missing-variable-declarations",
			"-Wno-extra-semi-stmt",
			"-Wno-unused-parameter",
			"-Wno-switch-default",
			"-Wno-format-non-iso",
			"-Wno-format",
			"-Wno-float-equal",
			"-Wno-atomic-implicit-seq-cst",
			"-Wno-cast-function-type-strict",
			"-Wno-four-char-constants",
			"-Wno-missing-prototypes",
			"-Wno-string-conversion",
			"-Wno-ignored-qualifiers",
			"-Wno-assign-enum",
			"-Wno-disabled-macro-expansion",
			"-Wno-unreachable-code-break",
			"-Wno-unreachable-code-return",
			"-Wno-bitfield-enum-conversion",
			"-Wno-conditional-uninitialized",
			"-Wno-tautological-value-range-compare",
		},
	};

	add_build_config( options, &win64Common );
}

#endif // BUILDER_DOING_USER_CONFIG_BUILD
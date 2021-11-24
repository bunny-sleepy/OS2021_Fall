##load("@rules_cc//cc:defs.bzl", "cc_test")  #load the test target

cc_binary(
    name = "q0",
    srcs = [
        "q0.cc"
            ],
    deps = [
        "//lib:embedding_lib",
        "//lib:instruction_lib",
        "//lib:utils_lib",
        "//lib:model_lib"
    ],
    copts = [
        "-std=c++11"
    ],
    data = glob(["data/q0*"]),
)

cc_binary(
    name = "format",
    srcs = [
        "format.cc"
            ],
)

cc_binary(
    name = "q1",
    srcs = [
        "q1.cc",
	    "q1.hh"
            ],
    deps = [
        "//lib:embedding_lib",
        "//lib:instruction_lib",
        "//lib:utils_lib",
        "//lib:model_lib"
    ],
    copts = [
        "-std=c++11"
    ],
    linkopts = ["-lpthread"],
    data = glob(["data/q1*"]),
)

cc_binary(
    name = "q2",
    srcs = [
        "q2.cc",
	    "q2.hh"
            ],
    deps = [
        "//lib:embedding_lib",
        "//lib:instruction_lib",
        "//lib:utils_lib",
        "//lib:model_lib"
    ],
    copts = [
        "-std=c++11"
    ],
    linkopts = ["-lpthread"],
    data = glob(["data/q2*"]),
)

cc_binary(
    name = "q3",
    srcs = [
        "q3.cc",
	    "q3.hh"
            ],
    deps = [
        "//lib:embedding_lib",
        "//lib:instruction_lib",
        "//lib:utils_lib",
        "//lib:model_lib"
    ],
    copts = [
        "-std=c++11"
    ],
    linkopts = ["-lpthread"],
    data = glob(["data/q3*"]),
)

cc_binary(
    name = "q4",
    srcs = [
        "q4.cc",
	    "q4.hh"
            ],
    deps = [
        "//lib:embedding_lib",
        "//lib:instruction_lib",
        "//lib:utils_lib",
        "//lib:model_lib"
    ],
    copts = [
        "-std=c++11"
    ],
    linkopts = ["-lpthread"],
    data = glob(["data/q4*"]),
)

cc_binary(
    name = "q5",
    srcs = [
        "q5.cc",
	    "q5.hh"
            ],
    deps = [
        "//lib:embedding_lib",
        "//lib:instruction_lib",
        "//lib:utils_lib",
        "//lib:model_lib"
    ],
    copts = [
        "-std=c++11"
    ],
    linkopts = ["-lpthread"],
    data = glob(["data/q4*"]),
)

cc_binary(
    name = "test",
    srcs = [
        "test.cc"
    ],
)

cc_test(
  name = "benchmark",
  size = "small",
  srcs = ["benchmark.cc"],
  deps = [
      "@gbench//:benchmark",
      ],
  copts = [
        "-O3",
        "-std=c++11",
  ],
  data = glob(["data/*"]),
)

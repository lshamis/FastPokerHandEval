package(default_visibility = ["//visibility:private"])

cc_binary(
    name = "generate_tables",
    srcs = ["generate_tables.cc"],
    deps = [
        ":memory_layout",
        ":phe",
        "//third_party/senzee",
    ],
)

cc_library(
    name = "fsm",
    hdrs = [
        "fsm.h",
        "fsm.inl",
    ],
    deps = [
        ":common",
        "@git_sparsehash//:sparsehash",
    ],
)

cc_library(
    name = "phe",
    hdrs = [
        "phe.h",
        "phe.inl",
    ],
    deps = [
        ":common",
        ":fsm",
        ":memory_layout",
        "//:poker_hand_eval",
    ],
)

cc_library(
    name = "memory_layout",
    hdrs = [
        "memory_layout.h",
        "memory_layout.inl",
    ],
    deps = [":fsm"],
)

cc_library(
    name = "common",
    srcs = ["common.cc"],
    hdrs = ["common.h"],
)

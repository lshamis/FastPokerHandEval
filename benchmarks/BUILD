cc_binary(
    name = "benchmarks",
    srcs = ["benchmarks.cc"],
    data = ["//tables:phe"],
    linkopts = ["-lpthread"],  # "@com_github_google_benchmark//:benchmark" should link to pthread :/
    deps = [
        "//:poker_hand_eval",
        "@com_github_google_benchmark//:benchmark",
    ],
)

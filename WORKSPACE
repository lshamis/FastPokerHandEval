new_git_repository(
    name = "git_sparsehash",
    build_file_content = """
cc_library(
    name = "sparsehash",
    includes = ["."],
    hdrs = glob(["sparsehash/**/*"]),
    visibility = ["//visibility:public"],
)""",
    commit = "47a55825ca3b35eab1ca22b7ab82b9544e32a9af",
    remote = "https://github.com/sparsehash/sparsehash-c11.git",
)

git_repository(
    name = "com_github_google_benchmark",
    remote = "https://github.com/google/benchmark.git",
    tag = "v1.4.0",
)

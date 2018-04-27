# Package for aHDLC.

package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])

exports_files(["LICENSE"])

cc_library(
    name = "ahdlc",
    srcs = [
        "src/lib/crc_16.c",
        "src/lib/frame_layer.c",
    ],
    hdrs = [
        "src/lib/inc/crc_16.h",
        "src/lib/inc/frame_layer.h",
        "src/lib/inc/frame_layer_types.h",
    ],
)

cc_test(
    name = "ahdlc_test",
    srcs = [
      "src/unit_tests/tests/unit_tests.cc",
    ],
    deps = [
        ":ahdlc",
    ],
)

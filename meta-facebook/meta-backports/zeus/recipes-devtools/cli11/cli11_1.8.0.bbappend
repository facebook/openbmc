SRC_URI_remove = "gitsm://github.com/CLIUtils/CLI11"
SRC_URI_append = "git://github.com/CLIUtils/CLI11"
EXTRA_OECMAKE += "-DBUILD_TESTING=OFF"
DEPENDS += "gtest gmock nlohmann-json"

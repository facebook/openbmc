SRC_URI:remove = "gitsm://github.com/CLIUtils/CLI11"
SRC_URI:append = "git://github.com/CLIUtils/CLI11"
EXTRA_OECMAKE += "-DBUILD_TESTING=OFF"
DEPENDS += "gtest gmock nlohmann-json"

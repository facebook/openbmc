# Do not bring in nlohmann-json,gtest,gmock as submodules. Instead mark them as depends.
SRC_URI_remove = "gitsm://github.com/CLIUtils/CLI11;branch=v1"
SRC_URI_append = "git://github.com/CLIUtils/CLI11;branch=v1"
DEPENDS += "gtest gmock nlohmann-json"

# While we work on making test use external headers for nlohmann-json etc,
# disable building testing.
EXTRA_OECMAKE += "-DBUILD_TESTING=OFF"

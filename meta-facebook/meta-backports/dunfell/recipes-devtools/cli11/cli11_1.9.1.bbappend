# Do not bring in nlohmann-json,gtest,gmock as submodules. Instead mark them as depends.
SRC_URI:remove = "gitsm://github.com/CLIUtils/CLI11;branch=v1;protocol=https"
SRC_URI:prepend = "git://github.com/CLIUtils/CLI11;branch=v1;protocol=https "
DEPENDS += "gtest gmock nlohmann-json"

# While we work on making test use external headers for nlohmann-json etc,
# disable building testing.
EXTRA_OECMAKE += "-DBUILD_TESTING=OFF"

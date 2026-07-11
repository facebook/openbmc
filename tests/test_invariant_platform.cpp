#include <gtest/gtest.h>
#include <cstring>
#include <vector>
#include <cstdint>

// Forward declare the vulnerable function from platform.cpp
extern "C" {
    struct pldm_msg_hdr {
        uint8_t instance_id;
        uint8_t type;
        uint8_t command;
    };
    
    struct pldm_msg {
        pldm_msg_hdr hdr;
        uint8_t payload[256];
    };
    
    // Declare the handler function signature
    int handle_set_effecter_states(pldm_msg *request, size_t payload_len);
}

class PlatformPayloadBoundaryTest : public ::testing::TestWithParam<std::pair<std::vector<uint8_t>, bool>> {};

TEST_P(PlatformPayloadBoundaryTest, PayloadLengthValidationEnforced) {
    // Invariant: memcpy operations must not read beyond the actual payload boundary
    // The handler must either validate payload length or safely handle undersized buffers
    
    auto [payload_data, should_succeed] = GetParam();
    
    pldm_msg request = {};
    request.hdr.instance_id = 0;
    request.hdr.type = 0x05;  // Platform PLDM type
    request.hdr.command = 0x39;  // Set Effecter States command
    
    // Copy test payload into request
    size_t copy_len = std::min(payload_data.size(), size_t(253));
    std::memcpy(request.payload, payload_data.data(), copy_len);
    
    // Call the actual handler with the payload length
    int result = handle_set_effecter_states(&request, copy_len);
    
    // Security property: handler must not crash or read uninitialized memory
    // Valid payloads (>= 3 + 8 bytes) should process; undersized should reject gracefully
    if (copy_len >= 11) {  // 3 byte header offset + 8 bytes minimum for one effecter state
        EXPECT_NE(result, -1) << "Valid payload should not cause handler failure";
    } else {
        // Undersized payload must be rejected, not cause buffer overread
        EXPECT_EQ(result, -1) << "Undersized payload must be rejected";
    }
}

INSTANTIATE_TEST_SUITE_P(
    AdversarialPayloads,
    PlatformPayloadBoundaryTest,
    ::testing::Values(
        // Exact exploit: payload too short (only 2 bytes after offset 3)
        std::make_pair(std::vector<uint8_t>{0x00, 0x01, 0x02}, false),
        // Boundary: exactly at minimum valid size (3 + 8 = 11 bytes)
        std::make_pair(std::vector<uint8_t>(11, 0xAA), true),
        // Valid: full 8-byte effecter state field
        std::make_pair(std::vector<uint8_t>(19, 0xBB), true),
        // Boundary: one byte short of minimum
        std::make_pair(std::vector<uint8_t>(10, 0xCC), false),
        // Empty payload
        std::make_pair(std::vector<uint8_t>{}, false)
    )
);

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
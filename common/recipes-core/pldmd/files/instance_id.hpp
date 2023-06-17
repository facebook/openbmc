#pragma once

#include <bitset>
#include <cstdint>

namespace pldm
{

constexpr size_t maxInstanceIds = 32;

/** @class InstanceId
 *  @brief Implementation of PLDM instance id as per DSP0240 v1.0.0
 */
class InstanceId
{
  public:
    /** @brief Get next unused instance id
     *  @return - PLDM instance id
     */
    int next();

    /** @brief Mark an instance id as unused
     *  @param[in] instanceId - PLDM instance id to be freed
     */
    void markFree(uint8_t instanceId)
    {
        id.set(instanceId, false);
    }

  private:
    std::bitset<maxInstanceIds> id;
};

} // namespace pldm

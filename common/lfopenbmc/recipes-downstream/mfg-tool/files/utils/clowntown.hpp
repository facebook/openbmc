#pragma once

namespace mfgtool::clowntown
{

/** Reference to the global "expert mode" flag, for binding to the top-level
 *  --clowntown CLI option.
 *
 *  When set, mfg-tool skips its safety guardrails -- for example it will no
 *  longer refuse to run when multiple D-Bus services publish conflicting data
 *  for the same object.  It is the user's way of telling the tool "I know what
 *  I'm doing, stop being so careful."
 */
bool& flag();

/** True when the user passed --clowntown. */
inline bool enabled()
{
    return flag();
}

} // namespace mfgtool::clowntown

/**
 * @file filter.cpp
 * @brief Implementation of Filter abstract base class.
 * @author Laurent Jiang
 * @date 2026-02-03
 */

#include "core/filters/filter.h"

namespace gimp {

bool Filter::setParameter(const std::string& name, float value)
{
    (void)name;    // Suppress unused parameter warning
    (void)value;
    return false;
}

bool Filter::getParameter(const std::string& name, float& value) const
{
    (void)name;
    (void)value;
    return false;
}

float Filter::progress() const
{
    return 1.0F;
}

bool Filter::isRunning() const
{
    return false;
}

}  // namespace gimp

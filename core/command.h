/**
 * @file command.h
 * @brief Undoable command interface.
 * @author Laurent Jiang
 * @date 2025-12-08
 */

#pragma once

namespace gimp
{
class Command
{
public:
    virtual ~Command() = default;
    virtual void apply() = 0;
    virtual void undo() = 0;
};
} // namespace gimp

/**
 * @file tool.h
 * @brief Abstract base class for all drawing/editing tools with state machine.
 * @author Laurent Jiang
 * @date 2026-01-26
 */

#pragma once

#include <QPoint>
#include <Qt>

#include <memory>
#include <string>

namespace gimp {

class Document;
class CommandBus;

/**
 * @brief Input event data passed to tools during mouse interactions.
 */
struct ToolInputEvent {
    QPoint canvasPos;                 ///< Position in canvas coordinates.
    QPoint screenPos;                 ///< Position in screen coordinates.
    Qt::MouseButtons buttons;         ///< Currently pressed mouse buttons.
    Qt::KeyboardModifiers modifiers;  ///< Active keyboard modifiers.
    float pressure = 1.0F;            ///< Pen pressure (0.0 to 1.0), 1.0 for mouse.
};

/**
 * @brief Tool state machine states.
 */
enum class ToolState {
    Idle,    ///< Tool is inactive, waiting for input.
    Active,  ///< Tool is actively processing input (e.g., dragging).
    Commit   ///< Tool is committing the operation (issuing command).
};

/**
 * @brief Abstract base class for all tools.
 *
 * Implements a state machine with transitions:
 * - Idle -> Active (on mouse press)
 * - Active -> Active (on mouse move while dragging)
 * - Active -> Commit (on mouse release)
 * - Commit -> Idle (after command issued)
 */
class Tool {
  public:
    virtual ~Tool() = default;

    /*! @brief Returns the unique tool identifier.
     *  @return Tool ID string.
     */
    [[nodiscard]] virtual std::string id() const = 0;

    /*! @brief Returns the human-readable tool name.
     *  @return Tool display name.
     */
    [[nodiscard]] virtual std::string name() const = 0;

    /*! @brief Returns the current tool state.
     *  @return Current ToolState.
     */
    [[nodiscard]] ToolState state() const { return state_; }

    /**
     * @brief Sets the document this tool operates on.
     * @param document The active document.
     */
    void setDocument(std::shared_ptr<Document> document) { document_ = std::move(document); }

    /**
     * @brief Sets the command bus for issuing undoable commands.
     * @param commandBus The command bus instance.
     */
    void setCommandBus(CommandBus* commandBus) { commandBus_ = commandBus; }

    /**
     * @brief Called when mouse button is pressed.
     * @param event The input event data.
     * @return True if the event was handled.
     */
    bool onMousePress(const ToolInputEvent& event);

    /**
     * @brief Called when mouse moves (during drag or hover).
     * @param event The input event data.
     * @return True if the event was handled.
     */
    bool onMouseMove(const ToolInputEvent& event);

    /**
     * @brief Called when mouse button is released.
     * @param event The input event data.
     * @return True if the event was handled.
     */
    bool onMouseRelease(const ToolInputEvent& event);

    /**
     * @brief Called when the tool is activated (selected by user).
     */
    virtual void onActivate() {}

    /**
     * @brief Called when the tool is deactivated (another tool selected).
     */
    virtual void onDeactivate() { reset(); }

    /**
     * @brief Resets the tool to idle state, canceling any in-progress operation.
     */
    void reset();

  protected:
    /*! @brief Called when transitioning from Idle to Active.
     *  @param event The triggering input event.
     */
    virtual void beginStroke(const ToolInputEvent& event) = 0;

    /*! @brief Called during Active state for each mouse move.
     *  @param event The input event with current position.
     */
    virtual void continueStroke(const ToolInputEvent& event) = 0;

    /*! @brief Called when transitioning from Active to Commit.
     *  @param event The final input event.
     */
    virtual void endStroke(const ToolInputEvent& event) = 0;

    /*! @brief Called when operation is canceled before commit. */
    virtual void cancelStroke() {}

    /*! @brief Returns the active document.
     *  @return Shared pointer to the document.
     */
    [[nodiscard]] std::shared_ptr<Document> document() const { return document_; }

    /*! @brief Returns the command bus.
     *  @return Pointer to the command bus.
     */
    [[nodiscard]] CommandBus* commandBus() const { return commandBus_; }

  protected:
    //! @brief Current state of the tool.
    ToolState state_ = ToolState::Idle;
    //! @brief Active document the tool operates on.
    std::shared_ptr<Document> document_;
    //! @brief Command bus for issuing commands.
    CommandBus* commandBus_ = nullptr;
};

}  // namespace gimp

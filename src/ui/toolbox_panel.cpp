/**
 * @file toolbox_panel.cpp
 * @brief Implementation of ToolboxPanel widget.
 * @author Laurent Jiang
 * @date 2026-01-22
 */

#include "ui/toolbox_panel.h"

#include "core/event_bus.h"
#include "core/events.h"
#include "core/tool_registry.h"
#include "ui/theme.h"
#include "ui/tool_button.h"

#include <QPalette>

namespace gimp {

namespace {
constexpr int kGridColumns = 3;
}  // namespace

ToolboxPanel::ToolboxPanel(QWidget* parent)
    : QWidget(parent), buttonGroup_(new QButtonGroup(this))
{
    setupUi();
    populateTools();

    buttonGroup_->setExclusive(true);

    toolSwitchSub_ = EventBus::instance().subscribe<ToolSwitchRequestEvent>(
        [this](const ToolSwitchRequestEvent& evt) { setActiveTool(evt.targetToolId); });
}

ToolboxPanel::~ToolboxPanel()
{
    EventBus::instance().unsubscribe(toolSwitchSub_);
}

void ToolboxPanel::setupUi()
{
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Theme::toQColor(Theme::kPanelBackground));
    setPalette(pal);
    setAutoFillBackground(true);

    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(4, 4, 4, 4);
    mainLayout_->setSpacing(0);

    toolGrid_ = new QGridLayout();
    toolGrid_->setSpacing(1);
    toolGrid_->setContentsMargins(0, 0, 0, 0);

    mainLayout_->addLayout(toolGrid_);
    mainLayout_->addStretch();

    setMinimumWidth(80);
}

void ToolboxPanel::populateTools()
{
    const auto& registry = ToolRegistry::instance();
    auto primaryTools = registry.getPrimaryTools();

    int buttonId = 0;
    int row = 0;
    int col = 0;

    for (const auto& tool : primaryTools) {
        auto* button = new ToolButton(tool, this);

        // If tool has a group, add sub-tools
        if (!tool.groupId.empty()) {
            auto groupTools = registry.getToolsByGroup(tool.groupId);
            if (groupTools.size() > 1) {
                button->setSubTools(groupTools);
            }

            // Map all tools in group to this button
            for (const auto& groupTool : groupTools) {
                toolToGroupMap_[groupTool.id] = tool.id;
            }
        }

        toolButtons_[tool.id] = button;
        buttonGroup_->addButton(button, buttonId);

        connect(button, &ToolButton::toolActivated, this, &ToolboxPanel::onToolActivated);

        toolGrid_->addWidget(button, row, col);

        ++col;
        if (col >= kGridColumns) {
            col = 0;
            ++row;
        }

        if (tool.id == registry.getActiveTool()) {
            button->setChecked(true);
            activeToolId_ = tool.id;
        }

        ++buttonId;
    }
}

void ToolboxPanel::setActiveTool(const std::string& toolId)
{
    const std::string previousTool = activeToolId_;
    activeToolId_ = toolId;

    // Find the button for this tool (may be in a group)
    auto it = toolToGroupMap_.find(toolId);
    std::string buttonId = (it != toolToGroupMap_.end()) ? it->second : toolId;

    auto buttonIt = toolButtons_.find(buttonId);
    if (buttonIt != toolButtons_.end()) {
        buttonIt->second->setChecked(true);
        buttonIt->second->setCurrentTool(toolId);
    }

    ToolRegistry::instance().setActiveTool(toolId);
    // NOLINTNEXTLINE(modernize-use-designated-initializers)
    EventBus::instance().publish(ToolChangedEvent{previousTool, toolId});
}

void ToolboxPanel::onToolActivated(const std::string& toolId)
{
    const std::string previousTool = activeToolId_;
    activeToolId_ = toolId;

    // Uncheck other buttons
    for (auto& [id, button] : toolButtons_) {
        if (button->currentToolId() != toolId) {
            button->setChecked(false);
        } else {
            button->setChecked(true);
        }
    }

    ToolRegistry::instance().setActiveTool(toolId);
    // NOLINTNEXTLINE(modernize-use-designated-initializers)
    EventBus::instance().publish(ToolChangedEvent{previousTool, toolId});

    emit toolSelected(QString::fromStdString(toolId));
}

}  // namespace gimp

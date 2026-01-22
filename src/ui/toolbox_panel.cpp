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

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QScrollArea>

namespace gimp {

ToolboxPanel::ToolboxPanel(QWidget* parent)
    : QWidget(parent), buttonGroup_(new QButtonGroup(this))
{
    setupUi();
    populateTools();

    connect(buttonGroup_, &QButtonGroup::idClicked, this, &ToolboxPanel::onToolButtonClicked);
}

ToolboxPanel::~ToolboxPanel() = default;

void ToolboxPanel::setupUi()
{
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(4, 4, 4, 4);
    mainLayout_->setSpacing(8);

    auto* titleLabel = new QLabel("Toolbox", this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
    mainLayout_->addWidget(titleLabel);

    setMinimumWidth(80);
}

void ToolboxPanel::populateTools()
{
    const auto& registry = ToolRegistry::instance();
    auto tools = registry.getAllTools();

    std::unordered_map<std::string, QGridLayout*> categoryLayouts;
    std::vector<std::string> categoryOrder;

    for (const auto& tool : tools) {
        if (categoryLayouts.find(tool.category) == categoryLayouts.end()) {
            auto* groupBox = new QGroupBox(QString::fromStdString(tool.category), this);
            auto* gridLayout = new QGridLayout(groupBox);
            gridLayout->setSpacing(2);
            gridLayout->setContentsMargins(4, 8, 4, 4);
            categoryLayouts[tool.category] = gridLayout;
            categoryOrder.push_back(tool.category);
            mainLayout_->addWidget(groupBox);
        }
    }

    std::unordered_map<std::string, int> categoryToolCount;
    int buttonId = 0;

    for (const auto& tool : tools) {
        auto* button = new QToolButton(this);
        button->setText(QString::fromStdString(tool.name.substr(0, 2)));
        button->setToolTip(QString::fromStdString(
            tool.name + (tool.shortcut.empty() ? "" : " (" + tool.shortcut + ")")));
        button->setCheckable(true);
        button->setFixedSize(32, 32);
        button->setProperty("toolId", QString::fromStdString(tool.id));

        buttonGroup_->addButton(button, buttonId);

        auto* layout = categoryLayouts[tool.category];
        const int count = categoryToolCount[tool.category]++;
        const int row = count / 2;
        const int col = count % 2;
        layout->addWidget(button, row, col);

        if (tool.id == registry.getActiveTool()) {
            button->setChecked(true);
            activeToolId_ = tool.id;
        }

        ++buttonId;
    }

    mainLayout_->addStretch();
}

void ToolboxPanel::setActiveTool(const std::string& toolId)
{
    const std::string previousTool = activeToolId_;
    activeToolId_ = toolId;

    auto buttons = buttonGroup_->buttons();
    for (auto* button : buttons) {
        auto id = button->property("toolId").toString().toStdString();
        if (id == toolId) {
            button->setChecked(true);
            break;
        }
    }

    ToolRegistry::instance().setActiveTool(toolId);
    // NOLINTNEXTLINE(modernize-use-designated-initializers)
    EventBus::instance().publish(ToolChangedEvent{previousTool, toolId});
}

void ToolboxPanel::onToolButtonClicked(int id)
{
    auto* button = buttonGroup_->button(id);
    if (button != nullptr) {
        auto toolId = button->property("toolId").toString().toStdString();
        const std::string previousTool = activeToolId_;
        activeToolId_ = toolId;

        ToolRegistry::instance().setActiveTool(toolId);
        // NOLINTNEXTLINE(modernize-use-designated-initializers)
        EventBus::instance().publish(ToolChangedEvent{previousTool, toolId});

        emit toolSelected(QString::fromStdString(toolId));
    }
}

}  // namespace gimp

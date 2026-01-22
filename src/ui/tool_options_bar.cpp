/**
 * @file tool_options_bar.cpp
 * @brief Implementation of ToolOptionsBar widget.
 * @author Laurent Jiang
 * @date 2026-01-22
 */

#include "ui/tool_options_bar.h"

#include "core/events.h"
#include "core/tool_registry.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QSlider>
#include <QSpinBox>

namespace gimp {

ToolOptionsBar::ToolOptionsBar(QWidget* parent) : QWidget(parent)
{
    setupUi();

    toolChangedSub_ = EventBus::instance().subscribe<ToolChangedEvent>(
        [this](const ToolChangedEvent& event) { updateForTool(event.currentToolId); });

    updateForTool(ToolRegistry::instance().getActiveTool());
}

ToolOptionsBar::~ToolOptionsBar()
{
    EventBus::instance().unsubscribe(toolChangedSub_);
}

void ToolOptionsBar::setupUi()
{
    mainLayout_ = new QHBoxLayout(this);
    mainLayout_->setContentsMargins(8, 4, 8, 4);
    mainLayout_->setSpacing(12);

    toolNameLabel_ = new QLabel(this);
    toolNameLabel_->setStyleSheet("font-weight: bold;");
    mainLayout_->addWidget(toolNameLabel_);

    auto* separator = new QFrame(this);
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    mainLayout_->addWidget(separator);

    optionsContainer_ = new QWidget(this);
    auto* optionsLayout = new QHBoxLayout(optionsContainer_);
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->setSpacing(8);
    mainLayout_->addWidget(optionsContainer_);

    mainLayout_->addStretch();

    setMaximumHeight(40);
}

void ToolOptionsBar::clearOptions()
{
    auto* layout = optionsContainer_->layout();
    if (layout != nullptr) {
        // NOLINTNEXTLINE(misc-const-correctness)
        QLayoutItem* item = nullptr;
        while ((item = layout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
    }
}

void ToolOptionsBar::updateForTool(const std::string& toolId)
{
    clearOptions();

    const auto* tool = ToolRegistry::instance().getTool(toolId);
    if (tool == nullptr) {
        toolNameLabel_->setText("No Tool");
        return;
    }

    toolNameLabel_->setText(QString::fromStdString(tool->name));

    auto* layout = qobject_cast<QHBoxLayout*>(optionsContainer_->layout());
    if (layout == nullptr) {
        return;
    }

    if (tool->category == "Paint") {
        layout->addWidget(new QLabel("Size:", optionsContainer_));
        auto* sizeSpinner = new QSpinBox(optionsContainer_);
        sizeSpinner->setRange(1, 1000);
        sizeSpinner->setValue(20);
        sizeSpinner->setSuffix(" px");
        layout->addWidget(sizeSpinner);

        layout->addWidget(new QLabel("Opacity:", optionsContainer_));
        auto* opacitySlider = new QSlider(Qt::Horizontal, optionsContainer_);
        opacitySlider->setRange(0, 100);
        opacitySlider->setValue(100);
        opacitySlider->setMaximumWidth(100);
        layout->addWidget(opacitySlider);
        auto* opacityLabel = new QLabel("100%", optionsContainer_);
        opacityLabel->setMinimumWidth(35);
        layout->addWidget(opacityLabel);

        connect(opacitySlider, &QSlider::valueChanged,
                [opacityLabel](int value) { opacityLabel->setText(QString::number(value) + "%"); });

        layout->addWidget(new QLabel("Hardness:", optionsContainer_));
        auto* hardnessSlider = new QSlider(Qt::Horizontal, optionsContainer_);
        hardnessSlider->setRange(0, 100);
        hardnessSlider->setValue(100);
        hardnessSlider->setMaximumWidth(100);
        layout->addWidget(hardnessSlider);

    } else if (tool->category == "Selection") {
        auto* featherCheck = new QCheckBox("Feather edges", optionsContainer_);
        layout->addWidget(featherCheck);

        layout->addWidget(new QLabel("Radius:", optionsContainer_));
        auto* radiusSpinner = new QDoubleSpinBox(optionsContainer_);
        radiusSpinner->setRange(0, 100);
        radiusSpinner->setValue(0);
        radiusSpinner->setSuffix(" px");
        radiusSpinner->setEnabled(false);
        layout->addWidget(radiusSpinner);

        connect(featherCheck, &QCheckBox::toggled, radiusSpinner, &QWidget::setEnabled);

        auto* antialiasCheck = new QCheckBox("Antialiasing", optionsContainer_);
        antialiasCheck->setChecked(true);
        layout->addWidget(antialiasCheck);

    } else if (tool->category == "Transform") {
        if (toolId == "crop") {
            auto* aspectCheck = new QCheckBox("Fixed aspect ratio", optionsContainer_);
            layout->addWidget(aspectCheck);

            auto* ratioCombo = new QComboBox(optionsContainer_);
            ratioCombo->addItems({"Free", "1:1", "4:3", "16:9", "3:2"});
            ratioCombo->setEnabled(false);
            layout->addWidget(ratioCombo);

            connect(aspectCheck, &QCheckBox::toggled, ratioCombo, &QWidget::setEnabled);
        } else {
            auto* interpolationLabel = new QLabel("Interpolation:", optionsContainer_);
            layout->addWidget(interpolationLabel);

            auto* interpolationCombo = new QComboBox(optionsContainer_);
            interpolationCombo->addItems({"None", "Linear", "Cubic", "Sinc"});
            interpolationCombo->setCurrentIndex(2);
            layout->addWidget(interpolationCombo);
        }

    } else if (toolId == "text") {
        layout->addWidget(new QLabel("Font:", optionsContainer_));
        auto* fontCombo = new QComboBox(optionsContainer_);
        fontCombo->addItems({"Sans", "Serif", "Monospace"});
        layout->addWidget(fontCombo);

        layout->addWidget(new QLabel("Size:", optionsContainer_));
        auto* fontSizeSpinner = new QSpinBox(optionsContainer_);
        fontSizeSpinner->setRange(6, 200);
        fontSizeSpinner->setValue(24);
        fontSizeSpinner->setSuffix(" pt");
        layout->addWidget(fontSizeSpinner);

    } else if (toolId == "zoom") {
        layout->addWidget(new QLabel("Zoom Level:", optionsContainer_));
        auto* zoomCombo = new QComboBox(optionsContainer_);
        zoomCombo->addItems({"25%", "50%", "100%", "200%", "400%", "800%"});
        zoomCombo->setCurrentIndex(2);
        layout->addWidget(zoomCombo);

        auto* fitWindowBtn = new QCheckBox("Fit in window", optionsContainer_);
        layout->addWidget(fitWindowBtn);
    }
}

}  // namespace gimp

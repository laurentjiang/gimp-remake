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

void ToolOptionsBar::addSeparator(QHBoxLayout* layout)
{
    auto* sep = new QFrame(optionsContainer_);
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Sunken);
    layout->addWidget(sep);
}

void ToolOptionsBar::addSliderWithLabel(QHBoxLayout* layout,
                                        const QString& labelText,
                                        int minVal,
                                        int maxVal,
                                        int defaultVal)
{
    auto* label = new QLabel(labelText, optionsContainer_);
    auto* slider = new QSlider(Qt::Horizontal, optionsContainer_);
    slider->setRange(minVal, maxVal);
    slider->setValue(defaultVal);
    slider->setFixedWidth(80);
    auto* valueLabel = new QLabel(QString::number(defaultVal) + "%", optionsContainer_);
    valueLabel->setFixedWidth(40);

    layout->addWidget(label);
    layout->addWidget(slider);
    layout->addWidget(valueLabel);

    connect(slider, &QSlider::valueChanged, [valueLabel](int value) {
        valueLabel->setText(QString::number(value) + "%");
    });
}

void ToolOptionsBar::addSpinBox(QHBoxLayout* layout,
                                const QString& labelText,
                                int minVal,
                                int maxVal,
                                int defaultVal,
                                const QString& suffix)
{
    auto* label = new QLabel(labelText, optionsContainer_);
    auto* spinner = new QSpinBox(optionsContainer_);
    spinner->setRange(minVal, maxVal);
    spinner->setValue(defaultVal);
    spinner->setSuffix(suffix);
    layout->addWidget(label);
    layout->addWidget(spinner);
}

void ToolOptionsBar::addComboBox(QHBoxLayout* layout,
                                 const QString& labelText,
                                 const QStringList& items,
                                 int defaultIndex)
{
    auto* label = new QLabel(labelText, optionsContainer_);
    auto* combo = new QComboBox(optionsContainer_);
    combo->addItems(items);
    combo->setCurrentIndex(defaultIndex);
    layout->addWidget(label);
    layout->addWidget(combo);
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
        // Size option group
        addSpinBox(layout, "Size:", 1, 1000, 20, " px");
        addSeparator(layout);

        // Opacity option group
        addSliderWithLabel(layout, "Opacity:", 0, 100, 100);
        addSeparator(layout);

        // Hardness option group
        addSliderWithLabel(layout, "Hardness:", 0, 100, 100);

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
            addComboBox(layout, "Interpolation:", {"None", "Linear", "Cubic", "Sinc"}, 2);
        }

    } else if (toolId == "text") {
        addComboBox(layout, "Font:", {"Sans", "Serif", "Monospace"}, 0);
        addSpinBox(layout, "Size:", 6, 200, 24, " pt");

    } else if (toolId == "zoom") {
        addComboBox(layout, "Zoom Level:", {"25%", "50%", "100%", "200%", "400%", "800%"}, 2);

        auto* fitWindowBtn = new QCheckBox("Fit in window", optionsContainer_);
        layout->addWidget(fitWindowBtn);
    }
}

}  // namespace gimp

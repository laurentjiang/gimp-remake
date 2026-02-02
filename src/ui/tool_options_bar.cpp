/**
 * @file tool_options_bar.cpp
 * @brief Implementation of ToolOptionsBar widget.
 * @author Laurent Jiang
 * @date 2026-01-22
 */

#include "ui/tool_options_bar.h"

#include "core/events.h"
#include "core/tool_factory.h"
#include "core/tool_registry.h"
#include "core/tools/brush_tool.h"
#include "core/tools/fill_tool.h"

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

    if (toolId == "bucket_fill") {
        // Tolerance slider for bucket fill tool (0-255)
        auto* label = new QLabel("Tolerance:", optionsContainer_);
        auto* slider = new QSlider(Qt::Horizontal, optionsContainer_);
        slider->setRange(0, 255);
        slider->setValue(0);
        slider->setFixedWidth(100);
        auto* valueLabel = new QLabel("0", optionsContainer_);
        valueLabel->setFixedWidth(30);

        layout->addWidget(label);
        layout->addWidget(slider);
        layout->addWidget(valueLabel);

        connect(slider, &QSlider::valueChanged, [valueLabel](int value) {
            valueLabel->setText(QString::number(value));
            auto* activeTool = ToolFactory::instance().activeTool();
            if (auto* fillTool = dynamic_cast<FillTool*>(activeTool)) {
                fillTool->setTolerance(value);
            }
        });

    } else if (tool->category == "Paint") {
        // Size spinner
        auto* sizeLabel = new QLabel("Size:", optionsContainer_);
        auto* sizeSpinner = new QSpinBox(optionsContainer_);
        sizeSpinner->setRange(1, 1000);
        sizeSpinner->setSuffix(" px");
        layout->addWidget(sizeLabel);
        layout->addWidget(sizeSpinner);

        // Get current brush size from active tool
        auto* activeTool = ToolFactory::instance().activeTool();
        int currentSize = activeTool ? activeTool->brushSize() : 20;
        sizeSpinner->setValue(currentSize > 0 ? currentSize : 20);

        connect(sizeSpinner, QOverload<int>::of(&QSpinBox::valueChanged), [](int value) {
            auto* tool = ToolFactory::instance().activeTool();
            if (tool) {
                tool->setBrushSize(value);
            }
        });

        addSeparator(layout);

        // Opacity slider (0-100%)
        auto* opacityLabel = new QLabel("Opacity:", optionsContainer_);
        auto* opacitySlider = new QSlider(Qt::Horizontal, optionsContainer_);
        opacitySlider->setRange(0, 100);
        opacitySlider->setFixedWidth(80);
        auto* opacityValueLabel = new QLabel("100%", optionsContainer_);
        opacityValueLabel->setFixedWidth(40);

        // Get current opacity from BrushTool if applicable
        float currentOpacity = 1.0F;
        if (auto* brushTool = dynamic_cast<BrushTool*>(activeTool)) {
            currentOpacity = brushTool->opacity();
        }
        opacitySlider->setValue(static_cast<int>(currentOpacity * 100));
        opacityValueLabel->setText(QString::number(static_cast<int>(currentOpacity * 100)) + "%");

        layout->addWidget(opacityLabel);
        layout->addWidget(opacitySlider);
        layout->addWidget(opacityValueLabel);

        connect(opacitySlider, &QSlider::valueChanged, [opacityValueLabel](int value) {
            opacityValueLabel->setText(QString::number(value) + "%");
            auto* tool = ToolFactory::instance().activeTool();
            if (auto* brushTool = dynamic_cast<BrushTool*>(tool)) {
                brushTool->setOpacity(static_cast<float>(value) / 100.0F);
            }
        });

        addSeparator(layout);

        // Hardness slider (0-100%)
        auto* hardnessLabel = new QLabel("Hardness:", optionsContainer_);
        auto* hardnessSlider = new QSlider(Qt::Horizontal, optionsContainer_);
        hardnessSlider->setRange(0, 100);
        hardnessSlider->setFixedWidth(80);
        auto* hardnessValueLabel = new QLabel("50%", optionsContainer_);
        hardnessValueLabel->setFixedWidth(40);

        // Get current hardness from BrushTool if applicable
        float currentHardness = 0.5F;
        if (auto* brushTool = dynamic_cast<BrushTool*>(activeTool)) {
            currentHardness = brushTool->hardness();
        }
        hardnessSlider->setValue(static_cast<int>(currentHardness * 100));
        hardnessValueLabel->setText(QString::number(static_cast<int>(currentHardness * 100)) + "%");

        layout->addWidget(hardnessLabel);
        layout->addWidget(hardnessSlider);
        layout->addWidget(hardnessValueLabel);

        connect(hardnessSlider, &QSlider::valueChanged, [hardnessValueLabel](int value) {
            hardnessValueLabel->setText(QString::number(value) + "%");
            auto* tool = ToolFactory::instance().activeTool();
            if (auto* brushTool = dynamic_cast<BrushTool*>(tool)) {
                brushTool->setHardness(static_cast<float>(value) / 100.0F);
            }
        });

        // Velocity dynamics checkbox (only for BrushTool)
        if (toolId == "paintbrush") {
            addSeparator(layout);

            auto* dynamicsCheck = new QCheckBox("Velocity dynamics", optionsContainer_);
            dynamicsCheck->setToolTip("Simulate pressure from mouse speed (fast = light, slow = heavy)");
            layout->addWidget(dynamicsCheck);

            // Get current state from BrushTool
            if (auto* brushTool = dynamic_cast<BrushTool*>(activeTool)) {
                dynamicsCheck->setChecked(brushTool->velocityDynamics());
            }

            connect(dynamicsCheck, &QCheckBox::toggled, [](bool checked) {
                auto* tool = ToolFactory::instance().activeTool();
                if (auto* brushTool = dynamic_cast<BrushTool*>(tool)) {
                    brushTool->setVelocityDynamics(checked);
                }
            });
        }

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

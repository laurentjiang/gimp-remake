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
#include "ui/spin_slider.h"

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
    optionsContainer_->setStyleSheet(
        "QCheckBox::indicator { width: 14px; height: 14px; }"
        "QCheckBox::indicator:unchecked { background-color: #3c3c3c; border: 1px solid #555; "
        "border-radius: 2px; }"
        "QCheckBox::indicator:checked { background-color: #555; border: 1px solid #777; "
        "border-radius: 2px; image: url(:/icons/check.svg); }"
        "QCheckBox::indicator:checked:hover { background-color: #666; }"
        "QCheckBox::indicator:unchecked:hover { background-color: #4a4a4a; }");
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
        auto* toleranceSlider = new SpinSlider(optionsContainer_);
        toleranceSlider->setRange(0, 255);
        toleranceSlider->setLabel("Tolerance");
        toleranceSlider->setFixedWidth(140);
        toleranceSlider->setValue(0);
        layout->addWidget(toleranceSlider);

        connect(toleranceSlider, &SpinSlider::valueChanged, [](double value) {
            auto* activeTool = ToolFactory::instance().activeTool();
            if (auto* fillTool = dynamic_cast<FillTool*>(activeTool)) {
                fillTool->setTolerance(static_cast<int>(value));
            }
        });

    } else if (tool->category == "Paint") {
        auto* activeTool = ToolFactory::instance().activeTool();

        // Size SpinSlider
        auto* sizeSlider = new SpinSlider(optionsContainer_);
        sizeSlider->setRange(1, 1000);
        sizeSlider->setSuffix(" px");
        sizeSlider->setLabel("Size");
        sizeSlider->setFixedWidth(140);
        int currentSize = activeTool ? activeTool->brushSize() : 20;
        sizeSlider->setValue(currentSize > 0 ? currentSize : 20);
        layout->addWidget(sizeSlider);

        connect(sizeSlider, &SpinSlider::valueChanged, [](double value) {
            auto* tool = ToolFactory::instance().activeTool();
            if (tool) {
                tool->setBrushSize(static_cast<int>(value));
            }
        });

        // Opacity SpinSlider
        auto* opacitySlider = new SpinSlider(optionsContainer_);
        opacitySlider->setRange(0, 100);
        opacitySlider->setSuffix("%");
        opacitySlider->setLabel("Opacity");
        opacitySlider->setFixedWidth(140);
        float currentOpacity = 1.0F;
        if (auto* brushTool = dynamic_cast<BrushTool*>(activeTool)) {
            currentOpacity = brushTool->opacity();
        }
        opacitySlider->setValue(static_cast<double>(currentOpacity * 100));
        layout->addWidget(opacitySlider);

        connect(opacitySlider, &SpinSlider::valueChanged, [](double value) {
            auto* tool = ToolFactory::instance().activeTool();
            if (auto* brushTool = dynamic_cast<BrushTool*>(tool)) {
                brushTool->setOpacity(static_cast<float>(value) / 100.0F);
            }
        });

        // Hardness SpinSlider
        auto* hardnessSlider = new SpinSlider(optionsContainer_);
        hardnessSlider->setRange(0, 100);
        hardnessSlider->setSuffix("%");
        hardnessSlider->setLabel("Hardness");
        hardnessSlider->setFixedWidth(140);
        float currentHardness = 0.5F;
        if (auto* brushTool = dynamic_cast<BrushTool*>(activeTool)) {
            currentHardness = brushTool->hardness();
        }
        hardnessSlider->setValue(static_cast<double>(currentHardness * 100));
        layout->addWidget(hardnessSlider);

        connect(hardnessSlider, &SpinSlider::valueChanged, [](double value) {
            auto* tool = ToolFactory::instance().activeTool();
            if (auto* brushTool = dynamic_cast<BrushTool*>(tool)) {
                brushTool->setHardness(static_cast<float>(value) / 100.0F);
            }
        });

        // Velocity dynamics checkbox (only for BrushTool)
        if (toolId == "paintbrush") {
            addSeparator(layout);

            auto* dynamicsCheck = new QCheckBox("Velocity dynamics", optionsContainer_);
            dynamicsCheck->setToolTip(
                "Simulate pressure from mouse speed (fast = light, slow = heavy)");
            layout->addWidget(dynamicsCheck);

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

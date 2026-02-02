/**
 * @file tool_options_panel.cpp
 * @brief Implementation of ToolOptionsPanel.
 * @author Laurent Jiang
 * @date 2026-01-30
 */

#include "ui/tool_options_panel.h"

#include "ui/spin_slider.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

namespace gimp {

ToolOptionsPanel::ToolOptionsPanel(QWidget* parent) : QWidget(parent)
{
    setStyleSheet(
        "ToolOptionsPanel { background-color: #2b2b2b; }"
        "QLabel { color: #ffffff; }"
        "QSlider::groove:horizontal { background: #444; height: 8px; }"
        "QSlider::handle:horizontal { background: #0078d4; width: 18px; margin: -5px 0; }");

    auto* containerLayout = new QVBoxLayout(this);
    containerLayout->setContentsMargins(8, 8, 8, 8);
    containerLayout->setSpacing(4);

    // Tool name header
    m_toolNameLabel = new QLabel("No Tool Selected", this);
    m_toolNameLabel->setStyleSheet("font-weight: bold; font-size: 12px; margin-bottom: 8px;");
    containerLayout->addWidget(m_toolNameLabel);

    // Scroll area for options
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto* optionsWidget = new QWidget();
    m_mainLayout = new QVBoxLayout(optionsWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(8);
    m_mainLayout->setAlignment(Qt::AlignTop);

    scrollArea->setWidget(optionsWidget);
    containerLayout->addWidget(scrollArea, 1);  // Stretch factor 1 to fill space

    setLayout(containerLayout);
}

void ToolOptionsPanel::setTool(Tool* tool)
{
    m_currentTool = tool;
    m_toolOptions = dynamic_cast<ToolOptions*>(tool);

    clearOptions();

    if (!m_currentTool) {
        m_toolNameLabel->setText("No Tool Selected");
        return;
    }

    m_toolNameLabel->setText(QString::fromStdString(m_currentTool->name()));

    if (m_toolOptions) {
        populateOptions();
    }
}

void ToolOptionsPanel::clearOptions()
{
    // Remove all option widgets from layout
    while (m_mainLayout->count() > 0) {
        auto* item = m_mainLayout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    m_optionWidgets.clear();
    m_optionLabels.clear();
}

void ToolOptionsPanel::populateOptions()
{
    if (!m_toolOptions) {
        return;
    }

    auto options = m_toolOptions->getOptions();

    for (const auto& option : options) {
        // For sliders, the SpinSlider has its own label; skip creating separate label
        if (option.type != ToolOption::Type::Slider) {
            auto* label = new QLabel(QString::fromStdString(option.label), this);
            label->setStyleSheet("color: #cccccc; font-size: 11px;");
            m_mainLayout->addWidget(label);
            m_optionLabels[option.id] = label;
        }

        // Create option widget
        createOptionWidget(option);
    }

    // Add stretch at the end to push all widgets to the top
    m_mainLayout->addStretch();
}

void ToolOptionsPanel::createOptionWidget(const ToolOption& option)
{
    QWidget* widget = nullptr;

    switch (option.type) {
        case ToolOption::Type::Slider: {
            auto* spinSlider = new SpinSlider(this);
            spinSlider->setLabel(QString::fromStdString(option.label));
            spinSlider->setRange(option.minValue, option.maxValue);
            spinSlider->setSingleStep(option.step);
            spinSlider->setDecimals(option.step < 1.0F ? 1 : 0);

            // Set current value
            if (std::holds_alternative<int>(option.value)) {
                spinSlider->setValue(std::get<int>(option.value));
            } else if (std::holds_alternative<float>(option.value)) {
                spinSlider->setValue(static_cast<double>(std::get<float>(option.value)));
            }

            // Determine suffix based on option
            if (option.id.find("size") != std::string::npos) {
                spinSlider->setSuffix(" px");
            } else if (option.id.find("opacity") != std::string::npos ||
                       option.id.find("hardness") != std::string::npos) {
                spinSlider->setSuffix("%");
            }

            connect(spinSlider, &SpinSlider::valueChanged, this, [this, option](double value) {
                if (m_toolOptions) {
                    m_toolOptions->setOptionValue(option.id, static_cast<int>(value));
                }
            });

            widget = spinSlider;
            break;
        }

        case ToolOption::Type::Dropdown: {
            auto* comboBox = new QComboBox(this);
            for (const auto& choice : option.choices) {
                comboBox->addItem(QString::fromStdString(choice));
            }
            comboBox->setCurrentIndex(option.selectedIndex);

            connect(comboBox,
                    static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                    this,
                    [this, option](int index) {
                        if (m_toolOptions && index >= 0 &&
                            static_cast<std::size_t>(index) < option.choices.size()) {
                            m_toolOptions->setOptionValue(option.id, option.choices[index]);
                        }
                    });

            widget = comboBox;
            break;
        }

        case ToolOption::Type::Checkbox: {
            auto* checkBox = new QCheckBox(this);
            if (std::holds_alternative<bool>(option.value)) {
                checkBox->setChecked(std::get<bool>(option.value));
            }

            connect(checkBox,
                    &QCheckBox::checkStateChanged,
                    this,
                    [this, option](Qt::CheckState state) {
                        if (m_toolOptions) {
                            m_toolOptions->setOptionValue(option.id, state == Qt::Checked);
                        }
                    });

            widget = checkBox;
            break;
        }

        case ToolOption::Type::ColorPicker: {
            // Placeholder: just show current value
            auto* label = new QLabel(this);
            label->setText("Color Picker (TODO)");
            widget = label;
            break;
        }
    }

    if (widget) {
        m_mainLayout->addWidget(widget);
        m_optionWidgets[option.id] = widget;
    }
}

}  // namespace gimp

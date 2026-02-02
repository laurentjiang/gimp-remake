/**
 * @file tool_options_panel.cpp
 * @brief Implementation of ToolOptionsPanel.
 * @author Laurent Jiang
 * @date 2026-01-30
 */

#include "ui/tool_options_panel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QScrollArea>

namespace gimp {

ToolOptionsPanel::ToolOptionsPanel(QWidget* parent) : QWidget(parent)
{
    setStyleSheet(
        "ToolOptionsPanel { background-color: #2b2b2b; }"
        "QLabel { color: #ffffff; }"
        "QSlider::groove:horizontal { background: #444; height: 8px; }"
        "QSlider::handle:horizontal { background: #0078d4; width: 18px; margin: -5px 0; }"
    );

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

    scrollArea->setWidget(optionsWidget);
    containerLayout->addWidget(scrollArea);

    // Spacer at bottom
    containerLayout->addStretch();

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
        // Create label
        auto* label = new QLabel(QString::fromStdString(option.label), this);
        label->setStyleSheet("color: #cccccc; font-size: 11px;");
        m_mainLayout->addWidget(label);
        m_optionLabels[option.id] = label;

        // Create option widget
        createOptionWidget(option);
    }
}

void ToolOptionsPanel::createOptionWidget(const ToolOption& option)
{
    QWidget* widget = nullptr;

    switch (option.type) {
        case ToolOption::Type::Slider: {
            auto* slider = new QSlider(Qt::Horizontal, this);
            slider->setMinimum(static_cast<int>(option.minValue));
            slider->setMaximum(static_cast<int>(option.maxValue));
            slider->setSingleStep(static_cast<int>(option.step));

            // Set current value
            if (std::holds_alternative<int>(option.value)) {
                slider->setValue(std::get<int>(option.value));
            } else if (std::holds_alternative<float>(option.value)) {
                slider->setValue(static_cast<int>(std::get<float>(option.value)));
            }

            // Create layout with slider and value label
            auto* hLayout = new QHBoxLayout();
            hLayout->addWidget(slider);

            auto* valueLabel = new QLabel(QString::number(slider->value()), this);
            valueLabel->setMaximumWidth(40);
            valueLabel->setStyleSheet("color: #0078d4;");
            hLayout->addWidget(valueLabel);

            // Connect slider changes
            connect(slider, &QSlider::valueChanged, this, [this, valueLabel, option](int value) {
                valueLabel->setText(QString::number(value));
                if (m_toolOptions) {
                    m_toolOptions->setOptionValue(option.id, value);
                }
            });

            auto* container = new QWidget();
            container->setLayout(hLayout);
            widget = container;
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
                        if (m_toolOptions && index >= 0 && index < static_cast<int>(option.choices.size())) {
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

            connect(checkBox, &QCheckBox::checkStateChanged, this, [this, option](Qt::CheckState state) {
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

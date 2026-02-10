/**
 * @file layers_panel.cpp
 * @brief Implementation of LayersPanel widget.
 * @author Laurent Jiang
 * @date 2026-01-22
 */

#include "ui/layers_panel.h"

#include "core/document.h"
#include "core/events.h"
#include "core/layer.h"
#include "core/layer_stack.h"

#include <QHBoxLayout>
#include <QLabel>

#include <ranges>

namespace gimp {

LayersPanel::LayersPanel(QWidget* parent) : QWidget(parent)
{
    setupUi();

    stackChangedSub_ = EventBus::instance().subscribe<LayerStackChangedEvent>(
        [this](const LayerStackChangedEvent& /*event*/) { refreshLayerList(); });
}

LayersPanel::~LayersPanel()
{
    EventBus::instance().unsubscribe(stackChangedSub_);
}

void LayersPanel::setupUi()
{
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(4, 4, 4, 4);
    mainLayout_->setSpacing(4);

    auto* titleLabel = new QLabel("Layers", this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
    mainLayout_->addWidget(titleLabel);

    layerList_ = new QListWidget(this);
    layerList_->setSelectionMode(QAbstractItemView::SingleSelection);
    layerList_->setDragDropMode(QAbstractItemView::InternalMove);
    mainLayout_->addWidget(layerList_);

    // Opacity slider
    auto* opacityLayout = new QHBoxLayout();
    opacityLayout->setSpacing(4);
    opacityLabel_ = new QLabel("Opacity: 100%", this);
    opacityLabel_->setFixedWidth(85);
    opacityLayout->addWidget(opacityLabel_);

    opacitySlider_ = new QSlider(Qt::Horizontal, this);
    opacitySlider_->setRange(0, 100);
    opacitySlider_->setValue(100);
    opacitySlider_->setToolTip("Layer opacity");
    opacityLayout->addWidget(opacitySlider_);
    mainLayout_->addLayout(opacityLayout);

    connect(
        layerList_, &QListWidget::itemSelectionChanged, this, &LayersPanel::onItemSelectionChanged);
    connect(layerList_, &QListWidget::itemClicked, this, &LayersPanel::onItemClicked);
    connect(opacitySlider_, &QSlider::valueChanged, this, &LayersPanel::onOpacityChanged);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(2);

    addButton_ = new QPushButton("+", this);
    addButton_->setToolTip("Add new layer");
    addButton_->setFixedSize(28, 28);
    buttonLayout->addWidget(addButton_);

    removeButton_ = new QPushButton("-", this);
    removeButton_->setToolTip("Remove selected layer");
    removeButton_->setFixedSize(28, 28);
    buttonLayout->addWidget(removeButton_);

    moveUpButton_ = new QPushButton("↑", this);
    moveUpButton_->setToolTip("Move layer up");
    moveUpButton_->setFixedSize(28, 28);
    buttonLayout->addWidget(moveUpButton_);

    moveDownButton_ = new QPushButton("↓", this);
    moveDownButton_->setToolTip("Move layer down");
    moveDownButton_->setFixedSize(28, 28);
    buttonLayout->addWidget(moveDownButton_);

    buttonLayout->addStretch();
    mainLayout_->addLayout(buttonLayout);

    connect(addButton_, &QPushButton::clicked, this, &LayersPanel::onAddLayerClicked);
    connect(removeButton_, &QPushButton::clicked, this, &LayersPanel::onRemoveLayerClicked);
    connect(moveUpButton_, &QPushButton::clicked, this, &LayersPanel::onMoveUpClicked);
    connect(moveDownButton_, &QPushButton::clicked, this, &LayersPanel::onMoveDownClicked);

    setMinimumWidth(150);
}

void LayersPanel::setDocument(std::shared_ptr<Document> document)
{
    document_ = std::move(document);
    refreshLayerList();
}

void LayersPanel::refreshLayerList()
{
    layerList_->clear();

    if (!document_) {
        return;
    }

    const auto& layers = document_->layers();
    for (const auto& layer : std::views::reverse(layers)) {
        auto* item = new QListWidgetItem(layerList_);
        updateLayerItem(item, layer);
        item->setData(Qt::UserRole, QVariant::fromValue(reinterpret_cast<quintptr>(layer.get())));
    }
}

void LayersPanel::updateLayerItem(QListWidgetItem* item, const std::shared_ptr<Layer>& layer)
{
    QString text = QString::fromStdString(layer->name());

    if (layer->opacity() < 1.0F) {
        text += QString(" (%1%)").arg(static_cast<int>(layer->opacity() * 100));
    }

    item->setText(text);

    if (layer->visible()) {
        item->setIcon(QIcon(":/icons/eye-visible.svg"));
        item->setForeground(palette().text().color());
    } else {
        item->setIcon(QIcon(":/icons/eye-hidden.svg"));
        item->setForeground(Qt::gray);
    }
}

void LayersPanel::onItemSelectionChanged()
{
    auto items = layerList_->selectedItems();
    if (items.isEmpty() || !document_) {
        return;
    }

    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    auto* rawPtr = reinterpret_cast<Layer*>(items.first()->data(Qt::UserRole).value<quintptr>());

    const auto& layers = document_->layers();
    for (std::size_t i = 0; i < layers.count(); ++i) {
        if (layers[i].get() == rawPtr) {
            // NOLINTNEXTLINE(modernize-use-designated-initializers)
            EventBus::instance().publish(LayerSelectionChangedEvent{nullptr, layers[i], i});
            emit layerSelected(layers[i]);

            // Update opacity slider to match selected layer
            int opacityPercent = static_cast<int>(layers[i]->opacity() * 100.0F);
            opacitySlider_->blockSignals(true);
            opacitySlider_->setValue(opacityPercent);
            opacitySlider_->blockSignals(false);
            opacityLabel_->setText(QString("Opacity: %1%").arg(opacityPercent));
            break;
        }
    }
}

void LayersPanel::onOpacityChanged(int value)
{
    auto items = layerList_->selectedItems();
    if (items.isEmpty() || !document_) {
        return;
    }

    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    auto* rawPtr = reinterpret_cast<Layer*>(items.first()->data(Qt::UserRole).value<quintptr>());

    const auto& layers = document_->layers();
    for (std::size_t i = 0; i < layers.count(); ++i) {
        if (layers[i].get() == rawPtr) {
            layers[i]->setOpacity(static_cast<float>(value) / 100.0F);
            updateLayerItem(items.first(), layers[i]);
            opacityLabel_->setText(QString("Opacity: %1%").arg(value));
            // NOLINTNEXTLINE(modernize-use-designated-initializers)
            EventBus::instance().publish(LayerPropertyChangedEvent{layers[i], "opacity"});
            break;
        }
    }
}

void LayersPanel::onItemClicked(QListWidgetItem* item)
{
    if (!item || !document_) {
        return;
    }

    // Check if click was in the icon area (first ~24 pixels)
    // We can detect this by checking the mouse position relative to the item rect
    QPoint clickPos = layerList_->mapFromGlobal(QCursor::pos());
    QRect itemRect = layerList_->visualItemRect(item);
    int iconWidth = 24;  // Approximate icon width

    if (clickPos.x() < itemRect.x() + iconWidth) {
        // Click was on the visibility icon - toggle visibility
        // NOLINTNEXTLINE(performance-no-int-to-ptr)
        auto* rawPtr = reinterpret_cast<Layer*>(item->data(Qt::UserRole).value<quintptr>());

        const auto& layers = document_->layers();
        for (std::size_t i = 0; i < layers.count(); ++i) {
            if (layers[i].get() == rawPtr) {
                layers[i]->setVisible(!layers[i]->visible());
                updateLayerItem(item, layers[i]);
                // NOLINTNEXTLINE(modernize-use-designated-initializers)
                EventBus::instance().publish(
                    LayerPropertyChangedEvent{layers[i], "visible"});
                break;
            }
        }
    }
}

void LayersPanel::onAddLayerClicked()
{
    if (document_) {
        auto layer = document_->addLayer();
        EventBus::instance().publish(
            // NOLINTNEXTLINE(modernize-use-designated-initializers)
            LayerStackChangedEvent{LayerStackChangedEvent::Action::Added, layer});
        refreshLayerList();
    }
    emit addLayerRequested();
}

void LayersPanel::onRemoveLayerClicked()
{
    auto items = layerList_->selectedItems();
    if (items.isEmpty() || !document_) {
        return;
    }

    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    auto* rawPtr = reinterpret_cast<Layer*>(items.first()->data(Qt::UserRole).value<quintptr>());

    const auto& layers = document_->layers();
    for (std::size_t i = 0; i < layers.count(); ++i) {
        if (layers[i].get() == rawPtr) {
            auto layer = layers[i];
            document_->removeLayer(layer);
            EventBus::instance().publish(
                // NOLINTNEXTLINE(modernize-use-designated-initializers)
                LayerStackChangedEvent{LayerStackChangedEvent::Action::Removed, layer});
            refreshLayerList();
            break;
        }
    }
    emit removeLayerRequested();
}

void LayersPanel::onMoveUpClicked()
{
    auto items = layerList_->selectedItems();
    if (items.isEmpty() || !document_) {
        return;
    }

    const int row = layerList_->row(items.first());
    if (row > 0) {
        // UI shows layers in reverse order (top of stack at top of list)
        const std::size_t fromIndex =
            document_->layers().count() - 1 - static_cast<std::size_t>(row);
        const std::size_t toIndex = fromIndex + 1;

        auto layer = document_->layers()[fromIndex];
        if (document_->layers().moveLayer(fromIndex, toIndex)) {
            EventBus::instance().publish(
                LayerStackChangedEvent{LayerStackChangedEvent::Action::Reordered, layer});
            refreshLayerList();

            // Re-select the moved layer (it's now at row - 1)
            if (row - 1 >= 0 && row - 1 < layerList_->count()) {
                layerList_->setCurrentRow(row - 1);
            }
        }
    }
}

void LayersPanel::onMoveDownClicked()
{
    auto items = layerList_->selectedItems();
    if (items.isEmpty() || !document_) {
        return;
    }

    const int row = layerList_->row(items.first());
    if (row < layerList_->count() - 1) {
        // UI shows layers in reverse order (top of stack at top of list)
        const std::size_t fromIndex =
            document_->layers().count() - 1 - static_cast<std::size_t>(row);
        const std::size_t toIndex = fromIndex - 1;

        auto layer = document_->layers()[fromIndex];
        if (document_->layers().moveLayer(fromIndex, toIndex)) {
            EventBus::instance().publish(
                LayerStackChangedEvent{LayerStackChangedEvent::Action::Reordered, layer});
            refreshLayerList();

            // Re-select the moved layer (it's now at row + 1)
            if (row + 1 < layerList_->count()) {
                layerList_->setCurrentRow(row + 1);
            }
        }
    }
}

}  // namespace gimp

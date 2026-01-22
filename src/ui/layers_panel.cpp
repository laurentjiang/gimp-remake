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

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>

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

    connect(
        layerList_, &QListWidget::itemSelectionChanged, this, &LayersPanel::onItemSelectionChanged);

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
    for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
        auto* item = new QListWidgetItem(layerList_);
        updateLayerItem(item, *it);
        item->setData(Qt::UserRole, QVariant::fromValue(reinterpret_cast<quintptr>(it->get())));
    }
}

void LayersPanel::updateLayerItem(QListWidgetItem* item, const std::shared_ptr<Layer>& layer)
{
    QString text = QString::fromStdString(layer->name());

    if (!layer->visible()) {
        text = "[H] " + text;
        item->setForeground(Qt::gray);
    } else {
        text = "[V] " + text;
        item->setForeground(palette().text().color());
    }

    if (layer->opacity() < 1.0F) {
        text += QString(" (%1%)").arg(static_cast<int>(layer->opacity() * 100));
    }

    item->setText(text);

    auto* style = QApplication::style();
    if (layer->visible()) {
        item->setIcon(style->standardIcon(QStyle::SP_DialogApplyButton));
    } else {
        item->setIcon(style->standardIcon(QStyle::SP_DialogCancelButton));
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
            break;
        }
    }
}

void LayersPanel::onAddLayerClicked()
{
    if (document_) {
        auto layer = document_->add_layer();
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
            document_->remove_layer(layer);
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
        const std::size_t fromIndex =
            document_->layers().count() - 1 - static_cast<std::size_t>(row);
        const std::size_t toIndex = fromIndex + 1;
        // TODO(layers): Wire to LayerStack::move_layer when available
        static_cast<void>(toIndex);
        refreshLayerList();
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
        const std::size_t fromIndex =
            document_->layers().count() - 1 - static_cast<std::size_t>(row);
        const std::size_t toIndex = fromIndex - 1;
        // TODO(layers): Wire to LayerStack::move_layer when available
        static_cast<void>(toIndex);
        refreshLayerList();
    }
}

}  // namespace gimp

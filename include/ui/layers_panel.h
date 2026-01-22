/**
 * @file layers_panel.h
 * @brief Layers panel widget displaying the document layer stack.
 * @author Laurent Jiang
 * @date 2026-01-22
 */

#pragma once

#include "core/event_bus.h"

#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <memory>

namespace gimp {

class Document;
class Layer;

/**
 * @brief Panel displaying layers from the current document.
 *
 * Shows a list view of all layers with visibility toggles, opacity indicators,
 * and blend mode information. Subscribes to layer stack change events.
 */
class LayersPanel : public QWidget {
    Q_OBJECT

  public:
    explicit LayersPanel(QWidget* parent = nullptr);
    ~LayersPanel() override;

    void setDocument(std::shared_ptr<Document> document);

  signals:
    void layerSelected(std::shared_ptr<Layer> layer);
    void addLayerRequested();
    void removeLayerRequested();

  private slots:
    void onItemSelectionChanged();
    void onAddLayerClicked();
    void onRemoveLayerClicked();
    void onMoveUpClicked();
    void onMoveDownClicked();

  private:
    void setupUi();
    void refreshLayerList();
    void updateLayerItem(QListWidgetItem* item, const std::shared_ptr<Layer>& layer);

    QVBoxLayout* mainLayout_ = nullptr;
    QListWidget* layerList_ = nullptr;
    QPushButton* addButton_ = nullptr;
    QPushButton* removeButton_ = nullptr;
    QPushButton* moveUpButton_ = nullptr;
    QPushButton* moveDownButton_ = nullptr;

    std::shared_ptr<Document> document_;
    EventBus::SubscriptionId stackChangedSub_ = 0;
};

}  // namespace gimp

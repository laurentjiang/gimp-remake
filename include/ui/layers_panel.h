/**
 * @file layers_panel.h
 * @brief Layers panel widget displaying the document layer stack.
 * @author Laurent Jiang
 * @date 2026-01-22
 */

#pragma once

#include "core/event_bus.h"

#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
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
    /*! @brief Constructs the layers panel.
     *  @param parent Optional parent widget.
     */
    explicit LayersPanel(QWidget* parent = nullptr);
    ~LayersPanel() override;

    /*! @brief Sets the document to display layers for.
     *  @param document The active document.
     */
    void setDocument(std::shared_ptr<Document> document);

  signals:
    /*! @brief Emitted when a layer is selected.
     *  @param layer The selected layer.
     */
    void layerSelected(std::shared_ptr<Layer> layer);
    /*! @brief Emitted when the user requests to add a layer. */
    void addLayerRequested();
    /*! @brief Emitted when the user requests to remove a layer. */
    void removeLayerRequested();

  private slots:
    void onItemSelectionChanged();
    void onItemClicked(QListWidgetItem* item);
    void onItemDoubleClicked(QListWidgetItem* item);
    void onItemChanged(QListWidgetItem* item);
    void onOpacityChanged(int value);
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
    QSlider* opacitySlider_ = nullptr;
    QLabel* opacityLabel_ = nullptr;
    QPushButton* addButton_ = nullptr;
    QPushButton* removeButton_ = nullptr;
    QPushButton* moveUpButton_ = nullptr;
    QPushButton* moveDownButton_ = nullptr;

    std::shared_ptr<Document> document_;
    EventBus::SubscriptionId stackChangedSub_ = 0;
    bool isEditing_ = false;  ///< Track if an item is being edited.
};

}  // namespace gimp

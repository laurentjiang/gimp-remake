/**
 * @file resize_command.h
 * @brief Command to resize the document canvas.
 * @author Aless Tosi
 * @date 2026-02-09
 */

#pragma once

#include "core/command.h"
#include "core/selection_manager.h"

#include <QPainterPath>
#include <QPoint>

#include <cstdint>
#include <memory>
#include <vector>

namespace gimp {

class Document;
class Layer;

/**
 * @brief Command to resize the canvas while preserving content.
 */
class CanvasResizeCommand : public Command {
	public:
		/**
		 * @brief Constructs a canvas resize command.
		 * @param document Target document.
		 * @param width New canvas width in pixels.
		 * @param height New canvas height in pixels.
		 * @param anchorX Normalized horizontal anchor (0.0 = left, 1.0 = right).
		 * @param anchorY Normalized vertical anchor (0.0 = top, 1.0 = bottom).
		 */
		CanvasResizeCommand(std::shared_ptr<Document> document,
												int width,
												int height,
												float anchorX,
												float anchorY);

		~CanvasResizeCommand() override = default;

		/**
		 * @brief Applies the resize operation.
		 */
		void apply() override;

		/**
		 * @brief Undoes the resize operation.
		 */
		void undo() override;

	private:
		struct LayerSnapshot {
				std::shared_ptr<Layer> layer;
				std::vector<std::uint8_t> data;
		};

		void captureBeforeState();
		void restoreBeforeLayers();
		void restoreSelection(const QPainterPath& path, SelectionType type, const char* source);
		[[nodiscard]] QPainterPath buildAfterSelection() const;
		[[nodiscard]] QPoint computeOffset() const;

		std::shared_ptr<Document> document_;
		int targetWidth_ = 0;
		int targetHeight_ = 0;
		float anchorX_ = 0.5F;
		float anchorY_ = 0.5F;
		bool captured_ = false;

		int beforeWidth_ = 0;
		int beforeHeight_ = 0;
		std::vector<LayerSnapshot> beforeLayers_;

		QPainterPath beforeSelection_;
		SelectionType beforeSelectionType_ = SelectionType::Unknown;
		QPainterPath afterSelection_;
		SelectionType afterSelectionType_ = SelectionType::Unknown;
};

}  // namespace gimp
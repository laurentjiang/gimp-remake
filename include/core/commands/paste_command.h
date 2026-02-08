/**
 * @file paste_command.h
 * @brief Command stub to paste clipboard data into the document.
 * @author Aless Tosi
 * @date 2026-02-04
 */

#pragma once

#include "core/command.h"

#include <QImage>

#include <cstdint>
#include <memory>
#include <vector>

namespace gimp {

class Document;
class Layer;

/**
 * @brief Command to paste an image into a layer with undo support.
 */
class PasteCommand : public Command {
	public:
		PasteCommand(std::shared_ptr<Document> document, int x, int y, const QImage& image);

		void apply() override;
		void undo() override;

	private:
		void captureBeforeState();
		void captureAfterState();
		void updateState(const std::vector<std::uint8_t>& state);
		void writeImageToLayer();

		std::shared_ptr<Document> document_;
		std::shared_ptr<Layer> layer_;
		int regionX_ = 0;
		int regionY_ = 0;
		int regionWidth_ = 0;
		int regionHeight_ = 0;
		bool captured_ = false;
		bool createdLayer_ = false;

		std::vector<std::uint8_t> beforeState_;
		std::vector<std::uint8_t> afterState_;
		std::vector<std::uint8_t> imageData_;
};

}  // namespace gimp


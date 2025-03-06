/* ScummVM - ACK Engine Detection Header
 *
 * Defines the interface for detecting ACK games.
 * Integrated into ScummVM.
 */

#ifndef ACK_DETECTION_H
#define ACK_DETECTION_H

#include "engines/advancedDetector.h"

namespace Ack {

// Debug channel definitions.
enum {
	kDebugGeneral = 1 << 0,
	kDebugGraphics = 1 << 1,
	kDebugIO = 1 << 2
};

// Structure for ACK game descriptions.
struct AckGameDescription {
	ADGameDescription desc;
	int gameType;
};

const char *getGameId(const AckGameDescription *gd);

} // End of namespace Ack

#endif // ACK_DETECTION_H

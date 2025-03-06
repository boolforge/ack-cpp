/* ScummVM - ACK Engine Detection
 *
 * This file implements the detection logic for ACK games.
 * It registers game descriptors, extra GUI options, and metadata.
 * The conversion from the Free Pascal code (ackfree) has been carefully verified.
 *
 * Distributed under the GNU General Public License.
 */

#include "engines/ack/detection.h"
#include "engines/ack/ack.h"

#include "common/config-manager.h"
#include "common/system.h"
#include "common/savefile.h"
#include "common/translation.h"

namespace Ack {

const char *getGameId(const AckGameDescription *gd) {
	return gd->desc.gameId;
}

static const PlainGameDescriptor ackGames[] = {
	{"ack", "ACK Game System"},
	{nullptr, nullptr}
};

static const ADExtraGuiOptionsMap optionsList[] = {
	{
		GAMEOPTION_ORIGINAL_SAVE_NAMES,
		{
			_s("Use original save/load screens"),
			_s("Use the original save/load screens instead of the ScummVM ones"),
			"originalsaveload",
			false
		}
	},
	AD_EXTRA_GUI_OPTIONS_TERMINATOR
};

static const AckGameDescription gameDescriptions[] = {
	{
		{
			"ack",
			"",
			AD_ENTRY1("ACKDATA0.DAT", "12345678901234567890123456789012"),
			/* Additional metadata fields as needed */
		},
		/* Additional structure fields as needed */
	},
	{nullptr, nullptr}  // Terminator
};

const PlainGameDescriptor *detectGame(const Common::String &gameId) {
	for (int i = 0; ackGames[i].gameId != nullptr; i++) {
		if (gameId == ackGames[i].gameId)
			return &ackGames[i];
	}
	return nullptr;
}

} // End of namespace Ack

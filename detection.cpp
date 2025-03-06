/* ScummVM - ACK Engine Detection
 *
 * This file defines the detection logic for the ACK engine.
 * Registers game descriptors, extra GUI options, and metadata.
 * Integrated into ScummVM.
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
			Common::EN_ANY,
			Common::kPlatformDOS,
			ADGF_NO_FLAGS,
			GUIO0()
		},
		0
	},
	AD_TABLE_END_MARKER
};

class AckMetaEngineDetection : public AdvancedMetaEngineDetection {
public:
	AckMetaEngineDetection()
		: AdvancedMetaEngineDetection(gameDescriptions, sizeof(AckGameDescription), ackGames, optionsList) {
		_maxScanDepth = 3;
	}
	const char *getEngineId() const override {
		return "ack";
	}
	const char *getName() const override {
		return "ACK";
	}
	const char *getOriginalCopyright() const override {
		return "ACK (c) 1992-1994 David A. Blosser";
	}
};

} // End of namespace Ack

REGISTER_PLUGIN_STATIC(ACK_DETECTION, PLUGIN_TYPE_ENGINE_DETECTION, Ack::AckMetaEngineDetection);

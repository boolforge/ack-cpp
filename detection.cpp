/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "ack/detection.h"
#include "ack/ack.h"

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

// Detection entries
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
    AckMetaEngineDetection() : AdvancedMetaEngineDetection(gameDescriptions, sizeof(AckGameDescription), ackGames, optionsList) {
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

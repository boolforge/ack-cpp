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

#ifndef ACK_DETECTION_H
#define ACK_DETECTION_H

#include "engines/advancedDetector.h"

namespace Ack {

enum {
    kDebugGeneral = 1 << 0,
    kDebugGraphics = 1 << 1,
    kDebugIO = 1 << 2
};

struct AckGameDescription {
    ADGameDescription desc;
    int gameType;
};

const char *getGameId(const AckGameDescription *gd);

} // End of namespace Ack

#endif // ACK_DETECTION_H

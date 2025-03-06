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

#include "ack/ack.h"
#include "ack/detection.h"

#include "engines/advancedDetector.h"
#include "common/system.h"
#include "common/savefile.h"
#include "common/translation.h"

namespace Ack {

class AckMetaEngine : public AdvancedMetaEngine {
public:
    const char *getName() const override {
        return "ack";
    }

    bool hasFeature(MetaEngineFeature f) const override {
        return 
            (f == kSupportsListSaves) ||
            (f == kSupportsLoadingDuringStartup) ||
            (f == kSupportsDeleteSave) ||
            (f == kSavesSupportMetaInfo) ||
            (f == kSavesSupportThumbnail) ||
            (f == kSavesSupportCreationDate) ||
            (f == kSavesSupportPlayTime);
    }

    Common::Error createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const override {
        const AckGameDescription *gd = (const AckGameDescription *)desc;
        *engine = new AckEngine(syst, gd);
        return Common::kNoError;
    }

    SaveStateList listSaves(const char *target) const override {
        Common::SaveFileManager *saveFileMan = g_system->getSaveFileManager();
        Common::StringArray filenames;
        Common::String saveDesc;
        Common::String pattern = Common::String::format("%s.??", target);
        
        filenames = saveFileMan->listSavefiles(pattern);
        
        SaveStateList saveList;
        for (Common::StringArray::const_iterator file = filenames.begin(); file != filenames.end(); ++file) {
            // Obtain the last 2 digits of the filename, since they correspond to the save slot
            int slotNum = atoi(file->c_str() + file->size() - 2);
            
            if (slotNum >= 0 && slotNum <= 99) {
                Common::InSaveFile *in = saveFileMan->openForLoading(*file);
                if (in) {
                    saveDesc = in->readString();
                    saveList.push_back(SaveStateDescriptor(slotNum, saveDesc));
                    delete in;
                }
            }
        }
        
        Common::sort(saveList.begin(), saveList.end(), SaveStateDescriptorSlotComparator());
        return saveList;
    }

    int getMaximumSaveSlot() const override {
        return 99;
    }

    void removeSaveState(const char *target, int slot) const override {
        Common::String filename = Common::String::format("%s.%02d", target, slot);
        g_system->getSaveFileManager()->removeSavefile(filename);
    }

    SaveStateDescriptor querySaveMetaInfos(const char *target, int slot) const override {
        Common::String filename = Common::String::format("%s.%02d", target, slot);
        Common::InSaveFile *in = g_system->getSaveFileManager()->openForLoading(filename);
        
        if (!in)
            return SaveStateDescriptor();
            
        SaveStateDescriptor desc(slot, "");
        
        // Read save description
        desc.setDescription(in->readString());
        
        // Read creation date/time
        int day = in->readSint16LE();
        int month = in->readSint16LE();
        int year = in->readSint16LE();
        
        desc.setSaveDate(year, month, day);
        
        int hour = in->readSint16LE();
        int minutes = in->readSint16LE();
        
        desc.setSaveTime(hour, minutes);
        
        // Read thumbnail
        if (in->readByte() == 1) {
            int thumbWidth = in->readUint16LE();
            int thumbHeight = in->readUint16LE();
            int thumbSize = thumbWidth * thumbHeight * 4;
            
            byte *thumbData = (byte *)malloc(thumbSize);
            in->read(thumbData, thumbSize);
            
            desc.setThumbnail(Graphics::Surface::createFromData(thumbData, thumbWidth, thumbHeight, Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0)));
        }
        
        delete in;
        
        return desc;
    }
};

} // End of namespace Ack

#if PLUGIN_ENABLED_DYNAMIC(ACK)
    REGISTER_PLUGIN_DYNAMIC(ACK, PLUGIN_TYPE_ENGINE, Ack::AckMetaEngine);
#else
    REGISTER_PLUGIN_STATIC(ACK, PLUGIN_TYPE_ENGINE, Ack::AckMetaEngine);
#endif

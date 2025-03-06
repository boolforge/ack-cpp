/* ScummVM - ACK Meta Engine
 *
 * Implements the meta engine interface for the ACK engine.
 * Handles save state listing, deletion, and extra metadata.
 * Integrated into ScummVM.
 */

#include "engines/ack/ack.h"
#include "engines/ack/detection.h"

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
		// Support listing saves, loading during startup, delete, meta info, thumbnail and creation date.
		return (f == kSupportsListSaves) ||
		       (f == kSupportsLoadingDuringStartup) ||
		       (f == kSupportsDeleteSave) ||
		       (f == kSavesSupportMetaInfo) ||
		       (f == kSavesSupportThumbnail) ||
		       (f == kSavesSupportCreationDate) ||
		       (f == kSavesSupportPlayTime);
	}

	Common::Error createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const override {
		const AckGameDescription *gd = reinterpret_cast<const AckGameDescription *>(desc);
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
		desc.setDescription(in->readString());
		int day = in->readSint16LE();
		int month = in->readSint16LE();
		int year = in->readSint16LE();
		desc.setSaveDate(year, month, day);
		int hour = in->readSint16LE();
		int minutes = in->readSint16LE();
		desc.setSaveTime(hour, minutes);
		if (in->readByte() == 1) {
			int thumbWidth = in->readUint16LE();

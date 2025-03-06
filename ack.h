/* ScummVM - ACK Graphic Adventure Engine Header
 *
 * This header defines structures, constants, and the main engine class for the ACK engine.
 * Integrated into ScummVM.
 */

#ifndef ACK_H
#define ACK_H

#include "common/scummsys.h"
#include "common/system.h"
#include "common/error.h"
#include "common/fs.h"
#include "common/events.h"
#include "common/file.h"
#include "common/savefile.h"
#include "common/str.h"
#include "common/array.h"
#include "common/memstream.h"
#include "common/rect.h"
#include "common/textconsole.h"

#include "engines/util.h"
#include "engines/engine.h"

#include "graphics/surface.h"
#include "graphics/palette.h"

namespace Ack {

// Fixed-size string structure.
typedef Common::String IttyString;

// Structure for swapping information (from original).
struct SwapInfoRec {
	Common::String execFile;
	Common::String execParam;
	byte data[11];
};

// 16x16 graphic tile structure.
struct Grap256Unit {
	byte data[17][17];
};

typedef Grap256Unit GrapArray256;

// Palette record structure.
struct PaletteRec {
	byte r, g, b;
};

// Master record for game configuration.
struct MasterRec {
	byte textColors[10];
	int ackVersion;
	byte phaseColors[3][5][4];
};

// Main ACK engine class.
class AckEngine : public Engine {
public:
	AckEngine(OSystem *syst, const struct AckGameDescription *gd);
	~AckEngine() override;

	Common::Error run() override;

private:
	bool _quitTime;
	SwapInfoRec *_swapInfo;
	char _menuCmd;
	Common::String _daughter;
	Common::String _ds, _dc, _hres;
	GrapArray256 *_icons;
	int16 _whatOpt;
	int16 _oldWhatOpt;
	int _i, _i1, _i2;
	Common::String _systemDir;
	bool _passwordOk;
	Common::String _registration;
	Common::String _regno;
	MasterRec _ack;
	SwapInfoRec *_p4ts;
	bool _checking;
	byte *_block;
	Common::String _bgiDir;
	bool _disableMouse;
	Grap256Unit *_graphic;
	Common::String _advName;
	Common::String _lastCfgLoad;
	int _doserror;
	int _dosexitcode;

	// Mouse state.
	bool _mouseOn;
	bool _mouseActive;
	int _mouseX, _mouseY;
	bool _mouseClicked;

	// Screen memory.
	Graphics::Surface *_surface;
	byte *_screenBuffer;
	int _scrnl;
	int _scrnh[40];
	bool _spaceMono;

	// Constants.
	static const int kBlockSize = 10;
	static const int kGrapsSize = 10;
	static const int kPalette = 0;
	static const int kAckVersion = 20;
	static const int kScreenWidth = 320;
	static const int kScreenHeight = 200;
	static const int kMaxIcons = 100;

	// Methods.
	void initVars();
	void freeResources();
	Common::Error allocateResources();

	void initGameState();
	void processParameters();
	void loadBmpPalette(int version, const Common::String &name, const Common::String &sysdir);
	void menuSkinBmp();
	void updateScreen();
	Common::String version(byte v);
	void loadConfig();
	void loadIcons(const Common::String &fn);
	void saveIcons(const Common::String &fn);
	void putIcon(int xb, int yy, int bb);
	void showOption(byte x, byte n, int16 mo);
	void redisplay();
	void checkRegistration();
	void displayText(int x, int y, int color, const Common::String &text);
	void clearScreen();
	void mainMenuLoop();
	bool handleEvents();
	void checkMouseClick();
	void checkMouseMenuRegions();
	bool mouseIn(int x1, int y1, int x2, int y2);
	void processMenuCommand();
	void handleAdventureSelection();
	void handleAdventurePlay();
	bool loadAdventure(const Common::String &name);
	char convertKeyCode(Common::KeyCode keycode);
	void initMouse();
	void showMouse();
	void hideMouse();
	void trackMouse();
	void closeMouse();
	void loadFont();
	void loadGraps();
	Common::String getParameter(int idx);

	// Manager references.
	GraphicsManager *_graphicsManager;
	SoundManager *_soundManager;
	ScriptManager *_scriptManager;
};

} // End of namespace Ack

#endif // ACK_H

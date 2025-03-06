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

// Record structure definitions originally from Pascal

/**
 * @brief Fixed-size string converted to Common::String
 * Pascal used string[N] to denote strings with maximum length N
 */
typedef Common::String IttyString;

/**
 * @brief Record for swapping information (originally swapinfo_rec)
 * Used to pass parameters to child processes in the original DOS implementation
 */
struct SwapInfoRec {
    Common::String execFile;    // Originally string[12]
    Common::String execParam;   // Originally string[40]
    byte data[11];             // Originally array[1..10] of byte, we keep index 0 unused
};

/**
 * @brief Graphic unit representing a 16x16 tile
 * This corresponds to the original grap256unit type
 */
struct Grap256Unit {
    byte data[17][17];  // Originally indexed from 1 to 16
};

typedef Grap256Unit GrapArray256;

/**
 * @brief Palette record structure 
 * Originally paletterec which represented RGB color components
 */
struct PaletteRec {
    byte r, g, b;
};

/**
 * @brief Master record containing game configuration (originally masterrec)
 */
struct MasterRec {
    byte textColors[10];       // Color indices for various text elements
    int ackVersion;            // Version of the ACK system
    byte phaseColors[3][5][4]; // Color cycling information
    // Additional fields would be added as needed
};

/**
 * @brief Screen region detection structure
 */
struct ScreenRegion {
    int x1, y1, x2, y2;
    int option;
    char trigger;
};

/**
 * @brief Main ACK engine class
 */
class AckEngine : public Engine {
public:
    AckEngine(OSystem *syst, const ADGameDescription *gd);
    ~AckEngine() override;

    Common::Error run() override;

private:
    // Original Pascal globals converted to class members
    bool _quitTime;
    SwapInfoRec *_swapInfo;
    char _menuCmd;
    Common::String _daughter;  // Parameter to give to child processes
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
    
    // Mouse state
    bool _mouseOn;
    bool _mouseActive;
    int _mouseX, _mouseY;
    bool _mouseClicked;
    
    // Screen memory
    Graphics::Surface *_surface;
    byte *_screenBuffer;
    int _scrnl;
    int _scrnh[40];
    bool _spaceMono;
    
    // Constants
    static const int kBlockSize = 10;
    static const int kGrapsSize = 10;
    static const int kPalette = 0;
    static const int kAckVersion = 20;  // Example version
    
    // Methods converted from Pascal procedures
    void menuSkinBmp();
    void clearKeyboardBuffer();
    IttyString version(byte v);
    void loadConfig();
    void loadIcons(const Common::String &fn);
    void saveIcons(const Common::String &fn);
    void putIcon(int xb, int yy, int bb);
    void showOption(byte x, byte n, int16 mo);
    void redisplay();
    void checkRegistration();
    bool keypressed();
    char readkey();
    void loadBmpPalette(int version, const Common::String &advname, const Common::String &sysdir);
    void loadFont();
    void loadGraps();
    void say(int x, int y, int color, const Common::String &text);
    void clearScreen();
    void initMouse();
    void showMouse();
    void hideMouse();
    void closeMouse();
    void trackMouse();
    bool mouseIn(int x1, int y1, int x2, int y2);
    int16 checkMouse(int x1, int y1, int x2, int y2, int16 curOpt, int16 newOpt, char &cmd, char trigger);
    void help();
    bool exec2(const Common::String &program, const Common::String &params);
    void checkExec(int errorcode, int code);
    Common::String moduleNum(int num);
    byte lo(byte col);

    // Additional ScummVM-specific methods
    void initGraphics();
    void initGameState();
    void processEvents();
    bool handleEvents();
    void drawScreen();
    void mainMenuLoop();
    Common::String getParameterByIndex(int idx);
    int getParameterCount();
};

} // End of namespace Ack

#endif // ACK_H

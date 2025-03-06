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
#include "ack/graphics.h"

#include "common/config-manager.h"
#include "common/debug.h"
#include "common/debug-channels.h"
#include "common/file.h"
#include "common/substream.h"
#include "common/translation.h"

namespace Ack {

AckEngine::AckEngine(OSystem *syst, const ADGameDescription *gd) : Engine(syst) {
    // Initialize member variables
    _quitTime = false;
    _swapInfo = nullptr;
    _menuCmd = 0;
    _icons = nullptr;
    _whatOpt = 1;
    _oldWhatOpt = 1;
    _i = _i1 = _i2 = 0;
    _passwordOk = true;
    _registration = "registered";  // Default to registered version
    _p4ts = nullptr;
    _checking = false;
    _block = nullptr;
    _disableMouse = true;
    _graphic = nullptr;
    _advName = "NONAME";
    _lastCfgLoad = "NONAME";
    _doserror = 0;
    _dosexitcode = 0;
    _mouseOn = false;
    _mouseActive = false;
    _mouseX = _mouseY = 0;
    _mouseClicked = false;
    _surface = nullptr;
    _screenBuffer = nullptr;
    _scrnl = 0;
    _spaceMono = false;
    
    // Initialize screen height array (scrnh)
    for (int i = 0; i < 40; i++) {
        _scrnh[i] = i * 320;  // Assuming 320 pixels per line
    }
    
    // Debug channels
    DebugMan.addDebugChannel(kDebugGeneral, "general", "General debugging info");
    DebugMan.addDebugChannel(kDebugGraphics, "graphics", "Graphics operations");
    DebugMan.addDebugChannel(kDebugIO, "io", "Input/Output operations");
    
    // Get game data directory from ScummVM
    _systemDir = ConfMan.get("path");
    if (_systemDir.lastChar() != '\\' && _systemDir.lastChar() != '/')
        _systemDir += "/";
    
    debug(1, "AckEngine initialized with system directory: %s", _systemDir.c_str());
}

AckEngine::~AckEngine() {
    // Free allocated memory
    delete _swapInfo;
    delete[] _icons;
    free(_block);
    delete[] _graphic;
    if (_surface) {
        _surface->free();
        delete _surface;
    }
    free(_screenBuffer);
}

Common::Error AckEngine::run() {
    // Initialize graphics mode
    initGraphics(320, 200);
    _surface = new Graphics::Surface();
    _surface->create(320, 200, Graphics::PixelFormat::createFormatCLUT8());
    _screenBuffer = (byte *)malloc(320 * 200);
    memset(_screenBuffer, 0, 320 * 200);
    
    // Allocate memory for game resources
    _block = (byte *)malloc((kBlockSize + 1) * sizeof(void*));
    _icons = new GrapArray256[33];
    _graphic = new Grap256Unit[kGrapsSize + 1 + 4];
    _swapInfo = new SwapInfoRec();
    
    // Initialize game state
    initGameState();
    
    // Enter main game loop
    mainMenuLoop();
    
    return Common::kNoError;
}

void AckEngine::initGameState() {
    debug(1, "Initializing game state");
    
    // Setup equivalent of Pascal startup code
    _advName = "ACKDATA1";
    loadFont();
    say(30, 60, 0, "Loading...");
    loadBmpPalette(kAckVersion, "PALETTE2", _systemDir);
    
    _lastCfgLoad = "NONAME";
    _advName = "NONAME";
    
    // Load icons
    loadIcons("ACKDATA1.ICO");
    _i = 0;
    _graphic[_i + 241] = _icons[_i + 23];
    _graphic[_i + 242] = _icons[_i + 9];
    _graphic[_i + 243] = _icons[_i + 24];
    _graphic[_i + 244] = _icons[_i + 11];
    
    clearScreen();
    
    // Process command line parameters (adapt for ScummVM)
    Common::String paramStr = getParameterByIndex(0);
    if (!paramStr.empty() && paramStr[0] != '-') {
        say(10, 60, 0, "Loading Adventure...");
        _advName = paramStr;
        _ds = "N ";
        _daughter = _advName + _dc + paramStr + " " + _ds + Common::String::format("%d", kPalette);
        if (!_spaceMono)
            _daughter += " F";
            
        // In ScummVM, we'll handle adventure loading directly rather than executing ACK01.EXE
        if (loadAdventure(_advName)) {
            _i1 = 2;
            if (_graphic[_i1].data[1][1] != 255) {
                _i = _dosexitcode;
                _i1 = 1;
                _hres = "";
                for (_i = 1; _i <= _graphic[_i1].data[1][1]; _i++) {
                    _hres += (char)_graphic[_i1].data[_i+1][1];
                }
                _i1 = 2;
                if (_graphic[_i1].data[1][1] == 1)
                    _passwordOk = false;
                if (_graphic[_i1].data[1][1] == 2)
                    _passwordOk = true;
                _advName = _hres;
                if (_advName != "NONAME") {
                    loadFont();
                    loadGraps();
                } else {
                    _advName = _systemDir + "ACKDATA1";
                    loadFont();
                    _advName = "NONAME";
                }
            }
        }
    }
    
    initMouse();
    redisplay();
    _whatOpt = 1;
}

// Converted from procedure menuskinbmp
void AckEngine::menuSkinBmp() {
    // In original: loads a bitmap for menu skin
    debug(kDebugGraphics, "Loading menu skin bitmap");
    
    Common::File bmpFile;
    byte header[54];
    byte line[320];
    int i, i2;
    
    Common::String name = _systemDir + "\\ACKDATA0.DAT";
    if (bmpFile.open(name)) {
        // Read bitmap header
        bmpFile.read(header, 54);
        i2 = header[50]; // In Pascal this was header[51]
        if (i2 == 0)
            i2 = 256;
            
        // Skip palette data
        bmpFile.seek(54 + (i2 * 4), SEEK_SET);
        
        // Read bitmap data directly into screen buffer
        for (i = 0; i <= 32; i++) {
            bmpFile.read(line, 320);
            if (i < 200) {
                // Copy line to screen buffer or surface
                memcpy(_screenBuffer + i * 320, line, 320);
            }
        }
        
        bmpFile.close();
        
        // Update screen
        memcpy(_surface->getPixels(), _screenBuffer, 320 * 200);
        g_system->copyRectToScreen(_surface->getPixels(), _surface->pitch, 0, 0, 320, 200);
        g_system->updateScreen();
    }
}

// Converted from function version
IttyString AckEngine::version(byte v) {
    return Common::String::format("V%d.%d", v / 10, v % 10);
}

// Converted from procedure loadconfig
void AckEngine::loadConfig() {
    debug(kDebugIO, "Loading configuration for adventure: %s", _advName.c_str());
    
    Common::String masterFile = _advName + "MASTER.DAT"; // Assuming MASTERFILE constant
    Common::File ackFile;
    
    if (!ackFile.open(masterFile)) {
        _advName = "NONAME";
        // In Pascal: chdir(systemdir);
        return;
    }
    
    // Read master record
    ackFile.read(&_ack, sizeof(MasterRec));
    ackFile.close();
    
    // In the original, would load palette from .PAL file or fallback
    loadBmpPalette(_ack.ackVersion, _advName, _systemDir);
}

// Converted from procedure loadicons
void AckEngine::loadIcons(const Common::String &fn) {
    debug(kDebugIO, "Loading icons from: %s", fn.c_str());
    
    Common::File iconFile;
    if (!iconFile.open(_systemDir + fn))
        return;
        
    // Read icons (originally grap256unit structures)
    for (int i = 1; i <= 31; i++) {
        if (iconFile.eos())
            break;
            
        iconFile.read(&_icons[i], sizeof(Grap256Unit));
    }
    
    iconFile.close();
}

// Converted from procedure saveicons
void AckEngine::saveIcons(const Common::String &fn) {
    debug(kDebugIO, "Saving icons to: %s", fn.c_str());
    
    loadIcons(fn);
    
    Common::OutSaveFile *iconFile = g_system->getSaveFileManager()->openForSaving(_systemDir + fn);
    if (!iconFile)
        return;
        
    for (int i = 1; i <= 31; i++) {
        iconFile->write(&_icons[i], sizeof(Grap256Unit));
    }
    
    iconFile->finalize();
    delete iconFile;
}

// Converted from procedure puticon
void AckEngine::putIcon(int xb, int yy, int bb) {
    if ((_mouseOn) && (_mouseActive))
        hideMouse();
        
    // Copy 16x16 icon to screen at specified position
    for (int i = 1; i <= 16; i++) {
        memcpy(_screenBuffer + (xb * 4) + _scrnh[yy + i], &_icons[bb].data[i][1], 16);
    }
    
    // Update screen region
    int x = xb * 4;
    g_system->copyRectToScreen(_screenBuffer + x + _scrnh[yy + 1], 320, x, yy + 1, 16, 16);
    g_system->updateScreen();
}

// Converted from procedure showoption
void AckEngine::showOption(byte x, byte n, int16 mo) {
    if (!((x == 1) || (x == 12) || 
         ((_registration == "none") && (x == 11)) || 
         ((x == 2) && (_advName != "NONAME")) || 
         ((_advName != "NONAME") && _passwordOk)))
    {
        n = n + mo;
    }
    
    switch (x) {
    case 1:
        say(8, 57, n, "SELECT/CREATE");
        say(8, 65, n, " ADVENTURE");
        break;
    case 2:
        say(48, 57, n, "PLAY ADVENTURE");
        break;
    case 3:
        say(8, 81, n, "CONFIGURE");
        say(8, 89, n, " ADVENTURE");
        break;
    case 4:
        say(48, 81, n, "IMPORT FILES,");
        say(48, 89, n, "EXPORT REPORTS");
        break;
    case 5:
        say(8, 105, n, "EDIT FONT");
        break;
    case 6:
        say(48, 105, n, "EDIT GRAPHIC");
        say(48, 113, n, " TILES");
        break;
    case 7:
        say(8, 129, n, "EDIT OBJECTS,");
        say(8, 137, n, "ITEMS, TERRAIN");
        break;
    case 8:
        say(48, 129, n, "EDIT MESSAGES");
        say(48, 137, n, "AND DIALOGUE");
        break;
    case 9:
        say(8, 153, n, "EDIT MAPS AND");
        say(8, 161, n, "REGIONS");
        break;
    case 10:
        say(48, 153, n, "EDIT PEOPLE");
        say(48, 161, n, "AND CREATURES");
        break;
    case 11:
        if (_registration == "none") {
            say(8, 177, n, "ORDERING");
            say(8, 185, n, "INFORMATION");
        } else {
            say(8, 177, n, "EDIT MACROS");
            say(8, 185, n, "(ADVANCED)");
        }
        break;
    case 12:
        say(48, 177, n, "QUIT");
        say(48, 185, n, "EXIT TO DOS");
        break;
    }
}

// Converted from procedure redisplay
void AckEngine::redisplay() {
    double ymlt;
    
    // In original: if u_graph.MONO then ymlt:=(3/2) else ymlt:=1;
    ymlt = 1.0;
    
    if (_advName != "NONAME") {
        loadConfig();
        // Set text colors (originally global vars from ack.textcolors)
        // For ScummVM, these would be managed differently
    }
    
    loadIcons("ACKDATA1.ICO");
    for (int i = 1; i <= 31; i++) {
        for (int i1 = 1; i1 <= 16; i1++) {
            for (int i2 = 1; i2 <= 16; i2++) {
                if (_icons[i].data[i1][i2] == 222)
                    _icons[i].data[i1][i2] = lo(0); // Originally TEXTC0
            }
        }
    }
    
    clearScreen();
    
    // Load menu skin bitmap
    menuSkinBmp();
    
    if (_advName != "NONAME") {
        say(11, 34, 1, "CURRENT ADVENTURE: " + _advName);
        if (_ack.ackVersion != kAckVersion)
            say(15, 42, 1, "(CREATED WITH ACK " + version(_ack.ackVersion) + ")");
    } else {
        say(20, 40, 1, "No Adventure loaded.");
    }
    
    // Place icons on screen
    putIcon(3, 57, 1);
    putIcon(43, 57, 2);
    putIcon(3, 81, 3);
    putIcon(43, 81, 4);
    putIcon(3, 105, 5);
    putIcon(43, 105, 6);
    putIcon(3, 129, 7);
    putIcon(43, 129, 8);
    putIcon(3, 153, 9);
    putIcon(43, 153, 10);
    
    if (_registration == "none")
        putIcon(3, 177, 11);
    else
        putIcon(3, 177, 28);
        
    putIcon(43, 177, 12);
    
    // Show menu options
    for (int i = 1; i <= 12; i++)
        showOption(i, 0, 1);
}

// Converted from procedure checkregistration
void AckEngine::checkRegistration() {
    // In ScummVM we'll simplify this and always mark as registered
    _registration = "REGISTERED";
}

// Main menu loop converted from Pascal main program
void AckEngine::mainMenuLoop() {
    debug(1, "Entering main menu loop");
    
    do {
        _checking = true;
        
        // String operations for daughter parameter
        _dc = Common::String::format(" %s", _ds.c_str());
        _dc += Common::String::format(" %s CH", _ds.c_str());
        _dc += Common::String::format("%s ", _ds.c_str());
        
        _quitTime = false;
        _ds = "N ";
        _daughter = _advName + _dc + _ds + Common::String::format("%d", kPalette);
        if (!_spaceMono)
            _daughter += " F";
            
        showOption(_whatOpt, 6, -2);
        _menuCmd = 1;  // Using 1 as placeholder for #1
        _oldWhatOpt = _whatOpt;
        
        // Input loop
        do {
            // Handle ScummVM events including keyboard and mouse
            if (!handleEvents())
                return;  // Exit if quit event detected
                
            if (_mouseOn)
                trackMouse();
                
            // Check if mouse is in menu areas
            if (mouseIn(4, 57, 33, 73)) {
                _whatOpt = 1;
                _menuCmd = '\r';
            }
            
            if (_mouseOn) {
                // Check all menu option regions
                checkMouseMenuRegions();
            }
            
        } while (_menuCmd == 1);
        
        showOption(_oldWhatOpt, 0, 1);
        
        // Process menu command
        processMenuCommand();
        
    } while (!_quitTime);
}

// ScummVM event handling
bool AckEngine::handleEvents() {
    Common::Event event;
    
    while (g_system->getEventManager()->pollEvent(event)) {
        switch (event.type) {
        case Common::EVENT_QUIT:
            return false;
            
        case Common::EVENT_KEYDOWN:
            // Convert ScummVM keycode to ACK key code
            _menuCmd = convertKeyCode(event.kbd.keycode);
            break;
            
        case Common::EVENT_MOUSEMOVE:
            _mouseX = event.mouse.x;
            _mouseY = event.mouse.y;
            break;
            
        case Common::EVENT_LBUTTONDOWN:
            _mouseClicked = true;
            // Check if click is in menu area
            checkMouseClick();
            break;
            
        default:
            break;
        }
    }
    
    return true;
}

// Check if mouse is in a specific region
bool AckEngine::mouseIn(int x1, int y1, int x2, int y2) {
    return (_mouseX >= x1 && _mouseX <= x2 && 
            _mouseY >= y1 && _mouseY <= y2);
}

// Execute a program (replacement for exec2 in Pascal)
bool AckEngine::exec2(const Common::String &program, const Common::String &params) {
    debug(1, "Would execute: %s with params: %s", program.c_str(), params.c_str());
    
    // In ScummVM, we'll implement the functionality directly rather than executing
    // external programs. Each "program" (ACK01.EXE, etc.) would become a method call.
    
    if (program.contains("ACK01.EXE")) {
        // Handle adventure selection/creation
        return handleAdventureSelection(params);
    } else if (program.contains(moduleNum(2))) {
        // Handle adventure play
        return handleAdventurePlay(params);
    } else if (program.contains(moduleNum(3))) {
        // Handle adventure configuration
        return handleAdventureConfig(params);
    }
    
    return true;
}

// Utility methods
Common::String AckEngine::moduleNum(int num) {
    // Convert module number to filename
    return Common::String::format("ACK%02d.EXE", num);
}

// ScummVM-specific helper for parameter parsing
Common::String AckEngine::getParameterByIndex(int idx) {
    if (ConfMan.hasKey(Common::String::format("arg_%d", idx)))
        return ConfMan.get(Common::String::format("arg_%d", idx));
    return "";
}

int AckEngine::getParameterCount() {
    int count = 0;
    while (ConfMan.hasKey(Common::String::format("arg_%d", count)))
        count++;
    return count;
}

// Stub methods that would need to be implemented
void AckEngine::say(int x, int y, int color, const Common::String &text) {
    debug(kDebugGraphics, "Say at (%d,%d) color %d: %s", x, y, color, text.c_str());
    // Would render text to screen
}

void AckEngine::clearScreen() {
    memset(_screenBuffer, 0, 320 * 200);
    memset(_surface->getPixels(), 0, 320 * 200);
    g_system->fillScreen(0);
    g_system->updateScreen();
}

void AckEngine::loadFont() {
    debug(kDebugIO, "Loading font");
    // Would load font data
}

void AckEngine::loadGraps() {
    debug(kDebugIO, "Loading graphics");
    // Would load graphic tiles
}

void AckEngine::loadBmpPalette(int version, const Common::String &advname, const Common::String &sysdir) {
    debug(kDebugGraphics, "Loading palette for %s (v%d)", advname.c_str(), version);
    // Would load and set palette
}

// Implementations of additional methods would follow...

} // End of namespace Ack

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
#include "ack/resource.h"

#include "common/config-manager.h"
#include "common/debug.h"
#include "common/debug-channels.h"
#include "common/file.h"
#include "common/substream.h"
#include "common/translation.h"
#include "common/events.h"

namespace Ack {

AckEngine::AckEngine(OSystem *syst, const AckGameDescription *gd) : Engine(syst), _gameDescription(gd) {
    // Initialize debugging channels
    DebugMan.addDebugChannel(kDebugGeneral, "general", "General debugging info");
    DebugMan.addDebugChannel(kDebugGraphics, "graphics", "Graphics operations");
    DebugMan.addDebugChannel(kDebugIO, "io", "Input/Output operations");
    DebugMan.addDebugChannel(kDebugSound, "sound", "Sound operations");
    DebugMan.addDebugChannel(kDebugScript, "script", "Script operations");

    // Get game data directory from ScummVM
    _systemDir = ConfMan.get("path");
    if (_systemDir.lastChar() != '\\' && _systemDir.lastChar() != '/')
        _systemDir += '/';

    // Initialize member variables
    initVars();
    
    // Resource manager init
    _resourceManager = new ResourceManager(this);
    
    debug(1, "AckEngine initialized with system directory: %s", _systemDir.c_str());
}

AckEngine::~AckEngine() {
    // Clean up resources in reverse order of initialization
    freeResources();
    
    delete _resourceManager;
    delete _graphicsManager;
    delete _soundManager;
    delete _scriptManager;
}

void AckEngine::initVars() {
    _quitTime = false;
    _menuCmd = 0;
    _whatOpt = 1;
    _oldWhatOpt = 1;
    _passwordOk = true;
    _registration = "registered";  // Default to registered version
    _checking = false;
    _disableMouse = true;
    _advName = "NONAME";
    _lastCfgLoad = "NONAME";
    _doserror = 0;
    _dosexitcode = 0;
    
    // Input state
    _mouseOn = false;
    _mouseActive = false;
    _mouseX = _mouseY = 0;
    _mouseClicked = false;
    _keyboardInput = false;
    _lastKeyPressed = 0;
    
    // Graphics resources
    _icons = nullptr;
    _graphic = nullptr;
    _surface = nullptr;
    _screenBuffer = nullptr;
    _scrnl = 0;
    _spaceMono = false;
    
    // Memory resources
    _swapInfo = nullptr;
    _p4ts = nullptr;
    _block = nullptr;
    
    // Temp vars
    _i = _i1 = _i2 = 0;
    
    // Initialize screen height array (scrnh)
    for (int i = 0; i < 40; i++) {
        _scrnh[i] = i * 320;  // Assuming 320 pixels per line
    }
}

void AckEngine::freeResources() {
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
    // Set up graphics system
    initGraphics(kScreenWidth, kScreenHeight);
    
    // Create graphics manager
    _graphicsManager = new GraphicsManager(this);
    
    // Create sound manager
    _soundManager = new SoundManager(this);
    
    // Create script manager
    _scriptManager = new ScriptManager(this);
    
    // Allocate resources
    allocateResources();
    
    // Initialize game state
    initGameState();
    
    // Main game loop
    mainMenuLoop();
    
    return Common::kNoError;
}

void AckEngine::allocateResources() {
    _surface = new Graphics::Surface();
    _surface->create(kScreenWidth, kScreenHeight, Graphics::PixelFormat::createFormatCLUT8());
    _screenBuffer = (byte *)malloc(kScreenWidth * kScreenHeight);
    memset(_screenBuffer, 0, kScreenWidth * kScreenHeight);
    
    // Allocate memory for game resources
    _block = (byte *)malloc((kBlockSize + 1) * sizeof(void*));
    _icons = new GrapArray256[kMaxIcons + 1];
    memset(_icons, 0, sizeof(GrapArray256) * (kMaxIcons + 1));
    _graphic = new Grap256Unit[kGrapsSize + 1 + 4];
    memset(_graphic, 0, sizeof(Grap256Unit) * (kGrapsSize + 1 + 4));
    _swapInfo = new SwapInfoRec();
    memset(_swapInfo, 0, sizeof(SwapInfoRec));
}

void AckEngine::initGameState() {
    debug(1, "Initializing game state");
    
    // Setup equivalent of Pascal startup code
    _advName = "ACKDATA1";
    loadFont();
    displayText(kScreenWidth / 2 - 30, 60, 0, "Loading...");
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
    processParameters();
    
    initMouse();
    redisplay();
    _whatOpt = 1;
}

void AckEngine::processParameters() {
    Common::String paramStr = getParameter(0);
    if (!paramStr.empty() && paramStr[0] != '-') {
        displayText(10, 60, 0, "Loading Adventure...");
        _advName = paramStr;
        _ds = "N ";
        _daughter = _advName + " " + _ds + Common::String::format("%d", kPalette);
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
}

void AckEngine::loadBmpPalette(int version, const Common::String &name, const Common::String &sysdir) {
    debug(kDebugGraphics, "Loading palette for %s (v%d)", name.c_str(), version);
    
    // Try loading palette from adventure specific or system file
    Common::String palettePath = sysdir + name + ".PAL";
    Common::File paletteFile;
    
    if (!paletteFile.open(palettePath)) {
        // Try system palette
        palettePath = sysdir + "PALETTE.PAL";
        if (!paletteFile.open(palettePath)) {
            warning("Could not open palette file %s", palettePath.c_str());
            return;
        }
    }
    
    // Read palette data
    PaletteRec palette[256];
    for (int i = 0; i < 256; i++) {
        palette[i].r = paletteFile.readByte();
        palette[i].g = paletteFile.readByte();
        palette[i].b = paletteFile.readByte();
    }
    
    paletteFile.close();
    
    // Convert palette to ScummVM format
    byte palData[256 * 3];
    for (int i = 0; i < 256; i++) {
        palData[i * 3] = palette[i].r * 4; // Scale from 0-63 to 0-255
        palData[i * 3 + 1] = palette[i].g * 4;
        palData[i * 3 + 2] = palette[i].b * 4;
    }
    
    // Set palette
    _system->getPaletteManager()->setPalette(palData, 0, 256);
}

void AckEngine::menuSkinBmp() {
    debug(kDebugGraphics, "Loading menu skin bitmap");
    
    Common::File bmpFile;
    byte header[54];
    byte line[320];
    int i, i2;
    
    Common::String name = _systemDir + "ACKDATA0.DAT";
    if (bmpFile.open(name)) {
        // Read bitmap header
        bmpFile.read(header, 54);
        i2 = header[50]; // Palette size
        if (i2 == 0)
            i2 = 256;
            
        // Skip palette data
        bmpFile.seek(54 + (i2 * 4), SEEK_SET);
        
        // Read bitmap data directly into screen buffer
        for (i = 0; i < kScreenHeight; i++) {
            bmpFile.read(line, kScreenWidth);
            // Copy line to screen buffer or surface
            memcpy(_screenBuffer + (kScreenHeight - 1 - i) * kScreenWidth, line, kScreenWidth);
        }
        
        bmpFile.close();
        
        // Update screen
        updateScreen();
    }
}

void AckEngine::updateScreen() {
    if (_surface && _screenBuffer) {
        memcpy(_surface->getPixels(), _screenBuffer, kScreenWidth * kScreenHeight);
        _system->copyRectToScreen(_surface->getPixels(), _surface->pitch, 0, 0, kScreenWidth, kScreenHeight);
        _system->updateScreen();
    }
}

Common::String AckEngine::version(byte v) {
    return Common::String::format("V%d.%d", v / 10, v % 10);
}

void AckEngine::loadConfig() {
    debug(kDebugIO, "Loading configuration for adventure: %s", _advName.c_str());
    
    Common::String masterFile = _advName + "MASTER.DAT";
    Common::File ackFile;
    
    if (!ackFile.open(masterFile)) {
        _advName = "NONAME";
        return;
    }
    
    // Read master record
    ackFile.read(&_ack, sizeof(MasterRec));
    ackFile.close();
    
    // Load palette
    loadBmpPalette(_ack.ackVersion, _advName, _systemDir);
}

void AckEngine::loadIcons(const Common::String &fn) {
    debug(kDebugIO, "Loading icons from: %s", fn.c_str());
    
    Common::File iconFile;
    if (!iconFile.open(_systemDir + fn))
        return;
        
    // Read icons (originally grap256unit structures)
    for (int i = 1; i <= kMaxIcons; i++) {
        if (iconFile.eos())
            break;
            
        iconFile.read(&_icons[i], sizeof(Grap256Unit));
    }
    
    iconFile.close();
}

void AckEngine::saveIcons(const Common::String &fn) {
    debug(kDebugIO, "Saving icons to: %s", fn.c_str());
    
    loadIcons(fn);
    
    Common::OutSaveFile *iconFile = _system->getSaveFileManager()->openForSaving(_systemDir + fn);
    if (!iconFile)
        return;
        
    for (int i = 1; i <= kMaxIcons; i++) {
        iconFile->write(&_icons[i], sizeof(Grap256Unit));
    }
    
    iconFile->finalize();
    delete iconFile;
}

void AckEngine::putIcon(int xb, int yy, int bb) {
    if ((_mouseOn) && (_mouseActive))
        hideMouse();
        
    // Copy 16x16 icon to screen at specified position
    int x = xb * 4;
    for (int i = 1; i <= 16; i++) {
        memcpy(_screenBuffer + x + _scrnh[yy + i], &_icons[bb].data[i][1], 16);
    }
    
    // Update screen region
    _system->copyRectToScreen(_screenBuffer + x + _scrnh[yy + 1], kScreenWidth, x, yy + 1, 16, 16);
    _system->updateScreen();
}

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
        displayText(8, 57, n, "SELECT/CREATE");
        displayText(8, 65, n, " ADVENTURE");
        break;
    case 2:
        displayText(48, 57, n, "PLAY ADVENTURE");
        break;
    case 3:
        displayText(8, 81, n, "CONFIGURE");
        displayText(8, 89, n, " ADVENTURE");
        break;
    case 4:
        displayText(48, 81, n, "IMPORT FILES,");
        displayText(48, 89, n, "EXPORT REPORTS");
        break;
    case 5:
        displayText(8, 105, n, "EDIT FONT");
        break;
    case 6:
        displayText(48, 105, n, "EDIT GRAPHIC");
        displayText(48, 113, n, " TILES");
        break;
    case 7:
        displayText(8, 129, n, "EDIT OBJECTS,");
        displayText(8, 137, n, "ITEMS, TERRAIN");
        break;
    case 8:
        displayText(48, 129, n, "EDIT MESSAGES");
        displayText(48, 137, n, "AND DIALOGUE");
        break;
    case 9:
        displayText(8, 153, n, "EDIT MAPS AND");
        displayText(8, 161, n, "REGIONS");
        break;
    case 10:
        displayText(48, 153, n, "EDIT PEOPLE");
        displayText(48, 161, n, "AND CREATURES");
        break;
    case 11:
        if (_registration == "none") {
            displayText(8, 177, n, "ORDERING");
            displayText(8, 185, n, "INFORMATION");
        } else {
            displayText(8, 177, n, "EDIT MACROS");
            displayText(8, 185, n, "(ADVANCED)");
        }
        break;
    case 12:
        displayText(48, 177, n, "QUIT");
        displayText(48, 185, n, "EXIT TO DOS");
        break;
    }
}

void AckEngine::redisplay() {
    // In original: if u_graph.MONO then ymlt:=(3/2) else ymlt:=1;
    double ymlt = 1.0;
    
    if (_advName != "NONAME") {
        loadConfig();
        // Set text colors (original used global vars from ack.textcolors)
    }
    
    loadIcons("ACKDATA1.ICO");
    for (int i = 1; i <= kMaxIcons; i++) {
        for (int i1 = 1; i1 <= 16; i1++) {
            for (int i2 = 1; i2 <= 16; i2++) {
                if (_icons[i].data[i1][i2] == 222)
                    _icons[i].data[i1][i2] = 0; // Originally TEXTC0
            }
        }
    }
    
    clearScreen();
    
    // Load menu skin bitmap
    menuSkinBmp();
    
    if (_advName != "NONAME") {
        displayText(11, 34, 1, "CURRENT ADVENTURE: " + _advName);
        if (_ack.ackVersion != kAckVersion)
            displayText(15, 42, 1, "(CREATED WITH ACK " + version(_ack.ackVersion) + ")");
    } else {
        displayText(20, 40, 1, "No Adventure loaded.");
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

void AckEngine::checkRegistration() {
    // In ScummVM we'll simplify this and always mark as registered
    _registration = "REGISTERED";
}

void AckEngine::displayText(int x, int y, int color, const Common::String &text) {
    debug(kDebugGraphics, "Say at (%d,%d) color %d: %s", x, y, color, text.c_str());
    
    // Here would go logic to actually render text on the screen
    // using the loaded font and current palette
    _graphicsManager->drawText(x, y, color, text);
    
    // For now, just update the screen after text rendering
    updateScreen();
}

void AckEngine::clearScreen() {
    memset(_screenBuffer, 0, kScreenWidth * kScreenHeight);
    memset(_surface->getPixels(), 0, kScreenWidth * kScreenHeight);
    _system->fillScreen(0);
    _system->updateScreen();
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
        _daughter = _advName + " " + _ds + Common::String::format("%d", kPalette);
        if (!_spaceMono)
            _daughter += " F";
            
        showOption(_whatOpt, 6, -2);
        _menuCmd = 1;  // Using 1 as placeholder for initial state
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
                _menuCmd = '\r'; // Enter key
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

bool AckEngine::handleEvents() {
    Common::Event event;
    
    while (_eventMan->pollEvent(event)) {
        switch (event.type) {
        case Common::EVENT_QUIT:
            return false;
            
        case Common::EVENT_KEYDOWN:
            // Convert ScummVM keycode to ACK key code
            _lastKeyPressed = convertKeyCode(event.kbd.keycode);
            _keyboardInput = true;
            _menuCmd = _lastKeyPressed;
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

void AckEngine::checkMouseClick() {
    // Check menu regions and set appropriate commands
    // This would have a more complete implementation
    
    // Example: Check if click is in quit option area
    if (mouseIn(48, 177, 64, 185)) {
        _menuCmd = 'q';
    }
}

void AckEngine::checkMouseMenuRegions() {
    // Check all menu option areas
    // This would have a more complete implementation
    
    // Example: Update highlighted option when mouse is over menu item
    if (mouseIn(4, 57, 33, 73) && _oldWhatOpt != 1) {
        showOption(_oldWhatOpt, 0, 1);
        _oldWhatOpt = 1;
        _whatOpt = 1;
        showOption(_whatOpt, 6, -2);
    }
}

bool AckEngine::mouseIn(int x1, int y1, int x2, int y2) {
    return (_mouseX >= x1 && _mouseX <= x2 && 
            _mouseY >= y1 && _mouseY <= y2);
}

void AckEngine::processMenuCommand() {
    // Handle menu commands
    switch (_menuCmd) {
    case 'q':
    case 'Q':
        _quitTime = true;
        break;
        
    case '\r': // Enter key
        switch (_whatOpt) {
        case 1: // Select/Create Adventure
            handleAdventureSelection();
            break;
        case 2: // Play Adventure
            if (_advName != "NONAME")
                handleAdventurePlay();
            break;
        case 12: // Quit
            _quitTime = true;
            break;
        default:
            // Handle other options
            break;
        }
        break;
        
    default:
        // Handle other key commands
        break;
    }
}

void AckEngine::handleAdventureSelection() {
    debug(1, "Adventure Selection UI would be shown here");
    // This would show the adventure selection UI
    // In original, would have executed ACK01.EXE
}

void AckEngine::handleAdventurePlay() {
    debug(1, "Adventure Play would start here");
    // This would start the game
    // In original, would have executed ACK02.EXE
}

bool AckEngine::loadAdventure(const Common::String &name) {
    debug(1, "Loading adventure: %s", name.c_str());
    
    // Check if adventure exists
    if (!Common::File::exists(_systemDir + name + "MASTER.DAT")) {
        warning("Adventure %s not found", name.c_str());
        return false;
    }
    
    // Load adventure data
    loadConfig();
    
    return true;
}

// Utility methods
char AckEngine::convertKeyCode(Common::KeyCode keycode) {
    // Convert ScummVM keycode to ACK key code
    switch (keycode) {
    case Common::KEYCODE_RETURN:
        return '\r';
    case Common::KEYCODE_ESCAPE:
        return 27;
    case Common::KEYCODE_q:
        return 'q';
    case Common::KEYCODE_Q:
        return 'Q';
    default:
        if (keycode >= Common::KEYCODE_a && keycode <= Common::KEYCODE_z)
            return 'a' + (keycode - Common::KEYCODE_a);
        if (keycode >= Common::KEYCODE_A && keycode <= Common::KEYCODE_Z)
            return 'A' + (keycode - Common::KEYCODE_A);
        return 0;
    }
}

// Input handling methods
void AckEngine::initMouse() {
    _mouseOn = true;
    _mouseActive = true;
    _mouseX = kScreenWidth / 2;
    _mouseY = kScreenHeight / 2;
    _mouseClicked = false;
    
    showMouse();
}

void AckEngine::showMouse() {
    if (!_mouseOn) {
        _mouseOn = true;
        _system->showMouse(true);
    }
}

void AckEngine::hideMouse() {
    if (_mouseOn) {
        _mouseOn = false;
        _system->showMouse(false);
    }
}

void AckEngine::trackMouse() {
    // In original, would update software mouse cursor
    // In ScummVM, we use the system mouse cursor
}

void AckEngine::closeMouse() {
    hideMouse();
    _mouseActive = false;
}

// Resource loading
void AckEngine::loadFont() {
    debug(kDebugIO, "Loading font");
    // Would load font data from file
    _resourceManager->loadFont();
}

void AckEngine::loadGraps() {
    debug(kDebugIO, "Loading graphics");
    // Would load graphic tiles
    _resourceManager->loadGraphics();
}

Common::String AckEngine::getParameter(int idx) {
    if (ConfMan.hasKey(Common::String::format("arg_%d", idx)))
        return ConfMan.get(Common::String::format("arg_%d", idx));
    return "";
}

} // End of namespace Ack

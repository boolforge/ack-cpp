/* ScummVM - ACK Graphic Adventure Engine
 *
 * This engine is based on the original ACK engine source (ackfree in Free Pascal).
 * It has been integrated into ScummVM with corrections to resource management,
 * error handling, and string processing to match ScummVM conventions.
 *
 * Distributed under the terms of the GNU General Public License.
 */

#include "engines/ack/ack.h"
#include "engines/ack/detection.h"
#include "engines/ack/graphics.h"   // Adapted (if applicable) for ACK graphics management.
#include "engines/ack/resource.h"   // Resource management for ACK.

#include "common/config-manager.h"
#include "common/debug.h"
#include "common/debug-channels.h"
#include "common/file.h"
#include "common/translation.h"
#include "common/events.h"
#include "common/system.h"

namespace Ack {

AckEngine::AckEngine(OSystem *syst, const AckGameDescription *gd)
	: Engine(syst)
	, _gameDescription(gd)
{
	// Set up debug channels.
	DebugMan.addDebugChannel(kDebugGeneral, "general", "General debugging info");
	DebugMan.addDebugChannel(kDebugGraphics, "graphics", "Graphics operations");
	DebugMan.addDebugChannel(kDebugIO, "io", "I/O operations");
	DebugMan.addDebugChannel(kDebugSound, "sound", "Sound operations");
	DebugMan.addDebugChannel(kDebugScript, "script", "Script operations");

	// Locate the game data directory from ScummVM configuration.
	_systemDir = ConfMan.get("path");
	if (!_systemDir.empty() && _systemDir.lastChar() != '/' && _systemDir.lastChar() != '\\')
		_systemDir += '/';

	initVars();

	// Initialize resource manager object.
	_resourceManager = new ResourceManager(this);

	debug(1, "AckEngine initialized with system directory: %s", _systemDir.c_str());
}

AckEngine::~AckEngine() {
	// Clean up allocated resources.
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
	_registration = "registered"; // Default version.
	_checking = false;
	_disableMouse = true;
	_advName = "NONAME";
	_lastCfgLoad = "NONAME";
	_doserror = 0;
	_dosexitcode = 0;

	// Input states.
	_mouseOn = false;
	_mouseActive = false;
	_mouseX = _mouseY = 0;
	_mouseClicked = false;
	_keyboardInput = false;
	_lastKeyPressed = 0;

	// Pointers.
	_icons = nullptr;
	_graphic = nullptr;
	_surface = nullptr;
	_screenBuffer = nullptr;
	_scrnl = 0;
	_spaceMono = false;
	_swapInfo = nullptr;
	_p4ts = nullptr;
	_block = nullptr;

	_i = _i1 = _i2 = 0;

	// Initialize screen height mapping array.
	for (int i = 0; i < 40; i++) {
		_scrnh[i] = i * 320;
	}
}

void AckEngine::freeResources() {
	// Free the allocated resources with additional error checks.
	if (_swapInfo)
		delete _swapInfo;
	if (_icons)
		delete[] _icons;
	if (_block)
		free(_block);
	if (_graphic)
		delete[] _graphic;

	if (_surface) {
		_surface->free();
		delete _surface;
		_surface = nullptr;
	}

	if (_screenBuffer) {
		free(_screenBuffer);
		_screenBuffer = nullptr;
	}
}

Common::Error AckEngine::run() {
	// Set up graphics via ScummVM surface creation.
	initGraphics(kScreenWidth, kScreenHeight);

	// Create the manager objects.
	_graphicsManager = new GraphicsManager(this);
	_soundManager = new SoundManager(this);
	_scriptManager = new ScriptManager(this);

	// Allocate memory for engine resources.
	if (allocateResources() != Common::kNoError)
		return Common::kBadError;

	// Initialize game state (reading configuration, fonts, icons, etc.)
	initGameState();

	// Enter the main menu loop.
	mainMenuLoop();

	return Common::kNoError;
}

Common::Error AckEngine::allocateResources() {
	_surface = new Graphics::Surface();
	if (!_surface->create(kScreenWidth, kScreenHeight, Graphics::PixelFormat::createFormatCLUT8()))
		return Common::kBadError;

	_screenBuffer = (byte *)malloc(kScreenWidth * kScreenHeight);
	if (!_screenBuffer)
		return Common::kNoMemoryError;
	memset(_screenBuffer, 0, kScreenWidth * kScreenHeight);

	// Allocate memory for game assets.
	_block = (byte *)malloc((kBlockSize + 1) * sizeof(void *));
	if (!_block)
		return Common::kNoMemoryError;

	_icons = new GrapArray256[kMaxIcons + 1];
	memset(_icons, 0, sizeof(GrapArray256) * (kMaxIcons + 1));

	_graphic = new Grap256Unit[kGrapsSize + 1 + 4];
	memset(_graphic, 0, sizeof(Grap256Unit) * (kGrapsSize + 1 + 4));

	_swapInfo = new SwapInfoRec();
	memset(_swapInfo, 0, sizeof(SwapInfoRec));

	return Common::kNoError;
}

void AckEngine::initGameState() {
	debug(1, "Initializing game state");

	// Set default adventure file and prepare loading routine.
	_advName = "ACKDATA1";
	loadFont();
	displayText(kScreenWidth / 2 - 30, 60, 0, "Loading...");
	loadBmpPalette(kAckVersion, "PALETTE2", _systemDir);

	_lastCfgLoad = "NONAME";
	_advName = "NONAME";

	// Load icons with error checking.
	loadIcons("ACKDATA1.ICO");

	// Sample mapping for graphics registration.
	_i = 0;
	_graphic[_i + 241] = _icons[_i + 23];
	_graphic[_i + 242] = _icons[_i + 9];
	_graphic[_i + 243] = _icons[_i + 24];
	_graphic[_i + 244] = _icons[_i + 11];

	clearScreen();

	// Process command-line parameters.
	processParameters();

	// Initialize mouse and display main menu.
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

		// Load adventure data with extra verification (as in the Pascal code).
		if (loadAdventure(_advName)) {
			_i1 = 2;
			if (_graphic[_i1].data[1][1] != 255) {
				_i = _dosexitcode;
				_i1 = 1;
				_hres = "";
				for (_i = 1; _i <= _graphic[_i1].data[1][1]; _i++) {
					_hres += (char)_graphic[_i1].data[_i + 1][1];
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

	Common::String palettePath = sysdir + name + ".PAL";
	Common::File paletteFile;
	if (!paletteFile.open(palettePath)) {
		palettePath = sysdir + "PALETTE.PAL";
		if (!paletteFile.open(palettePath)) {
			warning("Could not open palette file %s", palettePath.c_str());
			return;
		}
	}

	PaletteRec palette[256];
	for (int i = 0; i < 256; i++) {
		palette[i].r = paletteFile.readByte();
		palette[i].g = paletteFile.readByte();
		palette[i].b = paletteFile.readByte();
	}
	paletteFile.close();

	byte palData[256 * 3];
	for (int i = 0; i < 256; i++) {
		palData[i * 3] = palette[i].r * 4;
		palData[i * 3 + 1] = palette[i].g * 4;
		palData[i * 3 + 2] = palette[i].b * 4;
	}

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
		bmpFile.read(header, 54);
		i2 = header[50];
		if (i2 == 0)
			i2 = 256;
		bmpFile.seek(54 + (i2 * 4), SEEK_SET);

		for (i = 0; i < kScreenHeight; i++) {
			bmpFile.read(line, kScreenWidth);
			memcpy(_screenBuffer + (kScreenHeight - 1 - i) * kScreenWidth, line, kScreenWidth);
		}

		bmpFile.close();
		updateScreen();
	}
}

void AckEngine::updateScreen() {
	if (_surface && _screenBuffer) {
		memcpy(_surface->getPixels(), _screenBuffer, kScreenWidth * kScreenHeight);
		_system->copyRectToScreen(_surface->getPixels(), _surface->pitch,
		                           0, 0, kScreenWidth, kScreenHeight);
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

	ackFile.read(&_ack, sizeof(MasterRec));
	ackFile.close();
	loadBmpPalette(_ack.ackVersion, _advName, _systemDir);
}

void AckEngine::loadIcons(const Common::String &fn) {
	debug(kDebugIO, "Loading icons from: %s", fn.c_str());

	Common::File iconFile;
	if (!iconFile.open(_systemDir + fn)) {
		warning("Could not open icons file: %s", (_systemDir + fn).c_str());
		return;
	}

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

	int x = xb * 4;
	for (int i = 1; i <= 16; i++) {
		memcpy(_screenBuffer + x + _scrnh[yy + i], &_icons[bb].data[i][1], 16);
	}
	_system->copyRectToScreen(_screenBuffer + x + _scrnh[yy + 1], kScreenWidth, x, yy + 1, 16, 16);
	_system->updateScreen();
}

void AckEngine::showOption(byte x, byte n, int16 mo) {
	// Ensure some options are highlighted per the original design.
	if (!((x == 1) || (x == 12) ||
	      ((_registration == "none") && (x == 11)) ||
	      ((x == 2) && (_advName != "NONAME")) ||
	      ((_advName != "NONAME") && _passwordOk)))
	{
		n += mo;
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
	// Load configuration and refresh UI icons.
	if (_advName != "NONAME")
		loadConfig();

	loadIcons("ACKDATA1.ICO");
	for (int i = 1; i <= kMaxIcons; i++) {
		for (int i1 = 1; i1 <= 16; i1++) {
			for (int i2 = 1; i2 <= 16; i2++) {
				if (_icons[i].data[i1][i2] == 222)
					_icons[i].data[i1][i2] = 0;
			}
		}
	}

	clearScreen();
	menuSkinBmp();

	if (_advName != "NONAME") {
		displayText(11, 34, 1, "CURRENT ADVENTURE: " + _advName);
		if (_ack.ackVersion != kAckVersion)
			displayText(15, 42, 1, "(CREATED WITH ACK " + version(_ack.ackVersion) + ")");
	} else {
		displayText(20, 40, 1, "No Adventure loaded.");
	}

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

	// Redraw all menu options.
	for (int i = 1; i <= 12; i++)
		showOption(i, 0, 1);
}

void AckEngine::checkRegistration() {
	_registration = "REGISTERED"; // In ScummVM we treat all registrations as valid.
}

void AckEngine::displayText(int x, int y, int color, const Common::String &text) {
	debug(kDebugGraphics, "Display text at (%d,%d) color %d: %s", x, y, color, text.c_str());
	_graphicsManager->drawText(x, y, color, text);
	updateScreen();
}

void AckEngine::clearScreen() {
	memset(_screenBuffer, 0, kScreenWidth * kScreenHeight);
	memset(_surface->getPixels(), 0, kScreenWidth * kScreenHeight);
	_system->fillScreen(0);
	_system->updateScreen();
}

void AckEngine::mainMenuLoop() {
	debug(1, "Entering main menu loop");

	do {
		_checking = true;
		_dc = Common::String::format(" %s", _ds.c_str());
		_dc += Common::String::format(" %s CH", _ds.c_str());
		_dc += Common::String::format("%s ", _ds.c_str());

		_quitTime = false;
		_ds = "N ";
		_daughter = _advName + " " + _ds + Common::String::format("%d", kPalette);
		if (!_spaceMono)
			_daughter += " F";

		showOption(_whatOpt, 6, -2);
		_menuCmd = 1;
		_oldWhatOpt = _whatOpt;

		do {
			if (!handleEvents())
				return;
			if (_mouseOn)
				trackMouse();
			if (mouseIn(4, 57, 33, 73)) {
				_whatOpt = 1;
				_menuCmd = '\r';
			}
			if (_mouseOn)
				checkMouseMenuRegions();
		} while (_menuCmd == 1);

		showOption(_oldWhatOpt, 0, 1);
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
			checkMouseClick();
			break;
		default:
			break;
		}
	}
	return true;
}

void AckEngine::checkMouseClick() {
	if (mouseIn(48, 177, 64, 185))
		_menuCmd = 'q';
}

void AckEngine::checkMouseMenuRegions() {
	if (mouseIn(4, 57, 33, 73) && _oldWhatOpt != 1) {
		showOption(_oldWhatOpt, 0, 1);
		_oldWhatOpt = 1;
		_whatOpt = 1;
		showOption(_whatOpt, 6, -2);
	}
}

bool AckEngine::mouseIn(int x1, int y1, int x2, int y2) {
	return (_mouseX >= x1 && _mouseX <= x2 && _mouseY >= y1 && _mouseY <= y2);
}

void AckEngine::processMenuCommand() {
	switch (_menuCmd) {
	case 'q':
	case 'Q':
		_quitTime = true;
		break;
	case '\r':
		switch (_whatOpt) {
		case 1:
			handleAdventureSelection();
			break;
		case 2:
			if (_advName != "NONAME")
				handleAdventurePlay();
			break;
		case 12:
			_quitTime = true;
			break;
		default:
			// Additional menu options could be handled here.
			break;
		}
		break;
	default:
		break;
	}
}

void AckEngine::handleAdventureSelection() {
	debug(1, "Adventure selection UI would be displayed here");
	// Implementation for adventure selection based on original Pascal code.
}

void AckEngine::handleAdventurePlay() {
	debug(1, "Starting adventure play...");
	// Launch game play functionality.
}

bool AckEngine::loadAdventure(const Common::String &name) {
	debug(1, "Loading adventure: %s", name.c_str());
	if (!Common::File::exists(_systemDir + name + "MASTER.DAT")) {
		warning("Adventure %s not found", name.c_str());
		return false;
	}
	loadConfig();
	return true;
}

char AckEngine::convertKeyCode(Common::KeyCode keycode) {
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
	// Implementation based on ScummVM system mouse handling.
}

void AckEngine::closeMouse() {
	hideMouse();
	_mouseActive = false;
}

void AckEngine::loadFont() {
	debug(kDebugIO, "Loading font");
	_resourceManager->loadFont();
}

void AckEngine::loadGraps() {
	debug(kDebugIO, "Loading graphics");
	_resourceManager->loadGraphics();
}

Common::String AckEngine::getParameter(int idx) {
	if (ConfMan.hasKey(Common::String::format("arg_%d", idx)))
		return ConfMan.get(Common::String::format("arg_%d", idx));
	return "";
}

} // End of namespace Ack

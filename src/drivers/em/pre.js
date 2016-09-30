/**
 * @license
 *
 * Copyright (C) 2015  Valtteri "tsone" Heikkila
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

// NOTE: Originally from: http://jsfiddle.net/vWx8V/
var KEY_CODE_TO_NAME = {
    8: "Backspace",
    9: "Tab",
    13: "Return",
    16: "Shift",
    17: "Ctrl",
    18: "Alt",
    19: "Pause/Break",
    20: "Caps Lock",
    27: "Esc",
    32: "Space",
    33: "Page Up",
    34: "Page Down",
    35: "End",
    36: "Home",
    37: "Left",
    38: "Up",
    39: "Right",
    40: "Down",
    45: "Insert",
    46: "Delete",
    48: "0",
    49: "1",
    50: "2",
    51: "3",
    52: "4",
    53: "5",
    54: "6",
    55: "7",
    56: "8",
    57: "9",
    65: "A",
    66: "B",
    67: "C",
    68: "D",
    69: "E",
    70: "F",
    71: "G",
    72: "H",
    73: "I",
    74: "J",
    75: "K",
    76: "L",
    77: "M",
    78: "N",
    79: "O",
    80: "P",
    81: "Q",
    82: "R",
    83: "S",
    84: "T",
    85: "U",
    86: "V",
    87: "W",
    88: "X",
    89: "Y",
    90: "Z",
    91: "Meta",
    93: "Right Click",
    96: "Numpad 0",
    97: "Numpad 1",
    98: "Numpad 2",
    99: "Numpad 3",
    100: "Numpad 4",
    101: "Numpad 5",
    102: "Numpad 6",
    103: "Numpad 7",
    104: "Numpad 8",
    105: "Numpad 9",
    106: "Numpad *",
    107: "Numpad +",
    109: "Numpad -",
    110: "Numpad .",
    111: "Numpad /",
    112: "F1",
    113: "F2",
    114: "F3",
    115: "F4",
    116: "F5",
    117: "F6",
    118: "F7",
    119: "F8",
    120: "F9",
    121: "F10",
    122: "F11",
    123: "F12",
    144: "Num Lock",
    145: "Scroll Lock",
    182: "My Computer",
    183: "My Calculator",
    186: ";",
    187: "=",
    188: ",",
    189: "-",
    190: ".",
    191: "/",
    192: "`",
    219: "[",
    220: "\\",
    221: "]",
    222: "'"
};

var FCEM = {
    games: [],
    soundEnabled: true,
    showControls: (function (show) {
        // var el = document.getElementById('controllersToggle');
        // return function(show) {
        //     el.checked = (show === undefined) ? !el.checked : show;
        // };
        return function (show) {
        };
    })(),
    toggleSound: (function () {
        // var el = document.getElementById('soundIcon');
        // return function() {
        //     FCEM.soundEnabled = !FCEM.soundEnabled;
        //     FCEM.silenceSound(!FCEM.soundEnabled);
        //     el.style.backgroundPosition = (FCEM.soundEnabled ? '-32' : '-80') + 'px -48px';
        // };
        return function () {
        }
    })(),
    setupFiles: function () {

        try {
            FS.mkdir('/fceux/sav');
        } catch (e) {
        }
        try {
            FS.mkdir('/fceux/rom');
        } catch (e) {
        }

        var gameFile = Module["gameFile"];
        var freezeFile = Module["freezeFile"];
        var extraFiles = Module["extraFiles"] || {};
        //todo: handle .fds
        FS.createPreloadedFile("/fceux/rom/", "boot.nes", gameFile, true, true);
        if (freezeFile) {
            FS.createPreloadedFile("/fceux/sav/", "boot.fc0", freezeFile, true, true);
        }
        if (extraFiles["battery"]) {
            FS.createPreloadedFile("/fceux/sav/", "boot.sav", extraFiles["battery"], true, true);
        }
        FCEM.updateGames();
        FCEM.startGame("/fceux/rom/boot.nes");
    },
    startGame: function (path) {
        Module.romName = path;
        Module.romReload = 1;
    },
    updateGames: function () {
        var games = [];
        var addGamesIn = function (path, deletable) {
            var files = findFiles(path);
            files.forEach(function (filename) {
                var split = PATH.splitPath(filename);
                var label = split[2].slice(0, -split[3].length).toUpperCase();
                var offset = 0;
                games.push({label: label, path: filename, offset: offset, deletable: deletable});
            });
        };
        addGamesIn('/fceux/rom/', false);
        FCEM.games = games;
    },
    _getLocalInputDefault: function (id, type) {
        var m = (type ? 'gp' : 'input') + id;
        if (localStorage[m] === undefined) {
            if (FCEC.inputs[id] === undefined) {
                localStorage[m] = '0'; // NOTE: fallback if the id is undefined
            } else {
                localStorage[m] = FCEC.inputs[id][type];
            }
        }
        return parseInt(localStorage[m]);
    },
    setLocalKey: function (id, key) {
        localStorage['input' + id] = key;
    },
    getLocalKey: function (id) {
        return FCEM._getLocalInputDefault(id, 0);
    },
    setLocalGamepad: function (id, binding) {
        localStorage['gp' + id] = binding;
    },
    getLocalGamepad: function (id) {
        return FCEM._getLocalInputDefault(id, 1);
    },
    clearInputBindings: function () {
        for (var id in FCEC.inputs) {
            // clear local bindings
            delete localStorage['input' + id];
            delete localStorage['gp' + id];
            // clear host bindings
            var key = FCEM.getLocalKey(id);
            FCEM.bindKey(0, key);
            FCEM.bindGamepad(id, 0);
        }
    },
    syncInputBindings: function () {
        for (var id in FCEC.inputs) {
            var key = FCEM.getLocalKey(id);
            FCEM.bindKey(id, key);
            var binding = FCEM.getLocalGamepad(id);
            FCEM.bindGamepad(id, binding);
        }
    },
    initInputBindings: function () {
        FCEM.syncInputBindings();
        FCEM.initKeyBind();
    },
    key2Name: function (key) {
        var keyName = (key & 0x0FF) ? KEY_CODE_TO_NAME[key & 0x0FF] : '(Unset)';
        if (keyName === undefined) keyName = '(Unknown)';
        var prefix = '';
        if (key & 0x100 && keyName !== 'Ctrl')  prefix += 'Ctrl+';
        if (key & 0x400 && keyName !== 'Alt')   prefix += 'Alt+';
        if (key & 0x200 && keyName !== 'Shift') prefix += 'Shift+';
        if (key & 0x800 && keyName !== 'Meta')  prefix += 'Meta+';
        return prefix + keyName;
    },
    gamepad2Name: function (binding) {
        var type = binding & 0x03;
        var pad = (binding & 0x0C) >> 2;
        var idx = (binding & 0xF0) >> 4;
        if (!type) return '(Unset)';
        var typeNames = ['Button', '-Axis', '+Axis'];
        return 'Gamepad ' + pad + ' ' + typeNames[type - 1] + ' ' + idx;
    },
    initKeyBind: function () {

    },
    clearBinding: function (keyBind) {
        var id = keyBind.dataset.id;
        var key = FCEM.getLocalKey(id);
        FCEM.bindKey(0, key);
        FCEM.setLocalKey(id, 0);
        FCEM.bindGamepad(id, 0);
        FCEM.setLocalGamepad(id, 0);
        FCEM.initKeyBind();
    },
    resetDefaultBindings: function () {
        FCEM.clearInputBindings();
        FCEM.initInputBindings();
    },
    setLocalController: function (id, val) {
        localStorage['ctrl' + id] = Number(val);
        FCEM.setController(id, val);
    },
    initControllers: function (force) {
        if (force || !localStorage.getItem('ctrlInit')) {
            for (var id in FCEC.controllers) {
                localStorage['ctrl' + id] = FCEC.controllers[id][0];
            }
            localStorage['ctrlInit'] = 'true';
        }
        for (var id in FCEC.controllers) {
            var val = Number(localStorage['ctrl' + id]);
            FCEM.setController(id, val);
        }
    }
};

Module.preRun.push(function () {
    ENV.SDL_EMSCRIPTEN_KEYBOARD_ELEMENT = Module.targetID;
    FS.mkdir('/fceux');
    FCEM.setupFiles();
    // HACK: Disable default fullscreen handlers. See Emscripten's library_browser.js
    // The handlers forces the canvas size by setting css style width and height with
    // "!important" flag. Workaround is to disable the default fullscreen handlers.
    // See Emscripten's updateCanvasDimensions() in library_browser.js for the faulty code.
    // Browser.fullScreenHandlersInstalled = true;
});
Module.postRun.push(function () {
    console.log("Post Run 1");
    FCEM.setController = Module.cwrap('FCEM_SetController', null, ['number', 'number']);
    FCEM.bindKey = Module.cwrap('FCEM_BindKey', null, ['number', 'number']);
    FCEM.bindGamepad = Module.cwrap('FCEM_BindGamepad', null, ['number', 'number']);
    FCEM.silenceSound = Module.cwrap('FCEM_SilenceSound', null, ['number']);
    FCEM.saveGameFn = Module.cwrap('FCEM_OnSaveGameInterval', null, []);
    //Might change to have these not be implicitly declared
    gamecip_freeze = Module.cwrap('gamecip_freeze', null, []);
    gamecip_unfreeze = Module.cwrap('gamecip_unfreeze', null, []);
    //ptrs
    gamecip_ram = Module.cwrap('gamecip_ram', 'number', []);
    gamecip_ntaram = Module.cwrap('gamecip_ntaram', 'number', []);
    gamecip_palram = Module.cwrap('gamecip_palram', 'number', []);
    gamecip_ppu = Module.cwrap('gamecip_ppu', 'number', []);
    gamecip_spram = Module.cwrap('gamecip_spram', 'number', []);
    gamecip_VPage = Module.cwrap('gamecip_VPage', 'number', ['number']);
    //gamecip_vnapage = Module.cwrap('gamecip_vnapage', 'number', ['number']);
    gamecip_chr = Module.cwrap('gamecip_chr','number',['number']);
    //not ptr
    gamecip_chrSize = Module.cwrap('gamecip_chrSize','number',['number']);
    //ptr
    gamecip_prg = Module.cwrap('gamecip_prg','number',['number']);
    //not ptr
    gamecip_prgSize = Module.cwrap('gamecip_prgSize','number',['number']);
    //ptr
    gamecip_chrRAM = Module.cwrap('gamecip_chrRAM','number',[]);
    //not ptr
    gamecip_chrRAMSize = Module.cwrap('gamecip_chrRAMSize','number',[]);
    // //ptr
    // gamecip_fds = Module.cwrap('gamecip_fdsRAM','number',[]);
    // //not ptr
    // gamecip_fdsSize = Module.cwrap('gamecip_fdsRAMSize','number',[]);
    // //ptr
    // gamecip_wram = Module.cwrap('gamecip_wRAM','number',[]);
    // //not ptr
    // gamecip_wramSize = Module.cwrap('gamecip_wRAMSize','number',[]);
    // //ptr
    // gamecip_exRAM = Module.cwrap('gamecip_exRAM','number',[]);

    Module["getAudioCaptureInfo"] = function() {
        return {
            context:FCEM.audioContext,
            capturedNode:FCEM.scriptProcessorNode
        };
    };

    Module['setMuted'] = function(b) {
        FCEM.soundEnabled = !b;
        FCEM.silenceSound(b ? 1 : 0);
    };

    // Setup configuration from localStorage.
    FCEM.resetDefaultBindings();
    FCEM.initControllers();
    FCEM.initInputBindings();
});
Module.print = function(c) { console.log(c); };
Module.printErr = function(e) { console.error(e); };
Module.canvas2D = Module.canvas;
Module.canvas2D.style.setProperty("width", "inherit", "important");
Module.canvas2D.style.setProperty("height", "inherit", "important");
Module.canvas3D = (function () {
    var targetElement = document.getElementById(Module.targetID);
    var canvas = document.createElement("canvas");
    canvas.width = Module.canvas2D.width;
    canvas.height = Module.canvas2D.height;
    canvas.style.setProperty("width", "inherit", "important");
    canvas.style.setProperty("height", "inherit", "important");
    targetElement.appendChild(canvas);
    return canvas;
})();

Module['quit'] = function() {
    Module.noExitRuntime = false;
    try { Module.exit(0,false); }
    catch(e) { }
    Module.canvas2D.remove();
    Module.canvas3D.remove();
};


Module['isMuted'] = function() {
    return !FCEM.soundEnabled;
};
//todo: these guys
Module['saveState'] = function(onSaved, onError) {
    try {
        gamecip_freeze();
        if(onSaved) {
            onSaved(FS.readFile("/fceux/sav/boot.fc0", {encoding:'binary'}));
        }
    } catch(e) {
        if(onError) {
            onError(e);
        }
    }
};

Module['saveExtraFiles'] = function(files, onSaved, onError) {
    try {
        FCEM.saveGameFn();
        if(onSaved) {
            var r = {};
            for(var i = 0; i < files.length; i++) {
                if(files[i] == "battery") {
                    r["battery"] = FS.readFile("/fceux/sav/boot.sav", {encoding:'binary'});
                } else if(files[i] == "state") {
                    r["state"] = FS.readFile("/fceux/sav/boot.fc0", {encoding:'binary'});
                }
            }
            onSaved(r);
        }
    } catch(e) {
        if(onError) { 
            onError(files, e); 
        }
    }
};

Module['listExtraFiles'] = function() {
    return ["battery", "state"];
};

Module['loadState'] = function(s, onLoaded, onError) {
    try {
        //load s in place of "state.frz"
        FS.writeFile("/fceux/sav/boot.fc0", s, {encoding:'binary'});
        gamecip_unfreeze();
        if(onLoaded) {
            onLoaded(s);
        }
    } catch(e) {
        if(onError) {
            onError(s, e);
        }
    }
};

var current = 0;

function findFiles(startPath) {
    var entries = [];

    function isRealDir(p) {
        return p !== '.' && p !== '..';
    }
    function toAbsolute(root) {
        return function (p) {
            return PATH.join2(root, p);
        }
    }

    try {
        return FS.readdir(startPath).filter(isRealDir).map(toAbsolute(startPath));
    } catch (e) {
        return [];
    }
}

function range(lo,hi) {
    var r = [];
    for(var i = lo; i < hi; i++) {
        r.push(i);
    }
    return r;
}

Module["getMemoryRegions"] = function() {
    return {
        RAM:{size:0x800},
        NTARAM:{size:0x800},
        PALRAM:{size:0x20},
        PPU:{size:4},
        SPRAM:{size:0x100},
        VPage:range(0,8).map(function(i) { return {size:0x800, index:i}; }),
        chr:range(0,32).map(function(i) { return {size:gamecip_chrSize(i), index:i}; }),
        prg:range(0,32).map(function(i) { return {size:gamecip_prgSize(i), index:i}; }),
        chrRAM:{size:gamecip_chrRAMSize()},
      //  fdsRAM:{size:gamecip_fdsRAMSize()},
      //  WRAM:{size:gamecip_WRAMSize()},
      //  ExRAM:{size:gamecip_ExRAMSize()}
        //TODO: later: vnapage
    }
}

function getMemoryRegion(path) {
    var regions = Module.getMemoryRegions();
    if(typeof path == "string" && regions[path]) {
        regions[path].name = path;
        return regions[path];
    }
    var r = regions;
    for(var i = 0; i < path.length; i++) {
        if(!r[path[i]]) {
            return null;
        }
        r = r[path[i]];
    }
    r.name = path[0] || path;
    return r;
}

//regionPath is either a region ID or a list of region IDs (numbers or strings).
Module["getBytes"] = function(regionPath, offset, count) {
    var region = getMemoryRegion(regionPath);
    var ptr;
    /*
      gamecip_vnapage = Module.cwrap('gamecip_vnapage', number, [number]);
    */
    switch(region.name) {
    case "RAM":
        ptr = gamecip_ram();
        break;
    case "NTARAM":
        ptr = gamecip_ntaram();
        break;
    case "PALRAM":
        ptr = gamecip_palram();
        break;
    case "PPU":
        ptr = gamecip_ppu();
        break;
    case "SPRAM":
        ptr = gamecip_spram();
        break;
    case "chr":
        ptr = gamecip_chr(region.index);
        break;
    case "prg":
        ptr = gamecip_prg(region.index);
        break;
    case "chrRAM":
        ptr = gamecip_chrRAM();
        break;
    // case "fdsRAM":
    //     ptr = gamecip_fds();
    //     break;
    // case "WRAM":
    //     ptr = gamecip_wram();
    //     break;
    // case "ExRAM":
    //     ptr = gamecip_exRAM();
    //     break;
    case "VPage":
        ptr = gamecip_VPage(region.index);
        break;
        //...
    default:
        return null;
    }
    return new Uint8Array(Module.HEAPU8.buffer, ptr, count);
}

Module["setBytes"] = function(regionID, offset, byteArray) {
    //danger!
    var mem = Module.getBytes(regionID, offset, byteArray.length);
    mem.set(byteArray);
}

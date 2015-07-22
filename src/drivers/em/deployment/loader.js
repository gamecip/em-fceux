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
var KEY_CODE_TO_NAME = {8:"Backspace",9:"Tab",13:"Return",16:"Shift",17:"Ctrl",18:"Alt",19:"Pause/Break",20:"Caps Lock",27:"Esc",32:"Space",33:"Page Up",34:"Page Down",35:"End",36:"Home",37:"Left",38:"Up",39:"Right",40:"Down",45:"Insert",46:"Delete",48:"0",49:"1",50:"2",51:"3",52:"4",53:"5",54:"6",55:"7",56:"8",57:"9",65:"A",66:"B",67:"C",68:"D",69:"E",70:"F",71:"G",72:"H",73:"I",74:"J",75:"K",76:"L",77:"M",78:"N",79:"O",80:"P",81:"Q",82:"R",83:"S",84:"T",85:"U",86:"V",87:"W",88:"X",89:"Y",90:"Z",91:"Meta",93:"Right Click",96:"Numpad 0",97:"Numpad 1",98:"Numpad 2",99:"Numpad 3",100:"Numpad 4",101:"Numpad 5",102:"Numpad 6",103:"Numpad 7",104:"Numpad 8",105:"Numpad 9",106:"Numpad *",107:"Numpad +",109:"Numpad -",110:"Numpad .",111:"Numpad /",112:"F1",113:"F2",114:"F3",115:"F4",116:"F5",117:"F6",118:"F7",119:"F8",120:"F9",121:"F10",122:"F11",123:"F12",144:"Num Lock",145:"Scroll Lock",182:"My Computer",183:"My Calculator",186:";",187:"=",188:",",189:"-",190:".",191:"/",192:"`",219:"[",220:"\\",221:"]",222:"'"};

var FCEM = {
games : [],
soundEnabled : true,
showStack : (function(show) {
	var el = document.getElementById('stackToggle');
	return function(show) {
		el.checked = (show === undefined) ? !el.checked : show;
	};
})(),
showControls : (function(show) {
	var el = document.getElementById('controllersToggle');
	return function(show) {
		el.checked = (show === undefined) ? !el.checked : show;
	};
})(),
toggleSound : (function() {
	var el = document.getElementById('soundIcon');
	return function() {
		FCEM.soundEnabled = !FCEM.soundEnabled;
		FCEM.silenceSound(!FCEM.soundEnabled);
		el.src = FCEM.soundEnabled ? 'img/sound_on.gif' : 'img/sound_off.gif';
	};
})(),
  onInitialSyncFromIDB : function(er) {
    assert(!er);
    try {
      FS.mkdir('/fceux/sav');
    } catch (e) {
    }
    try {
      FS.mkdir('/fceux/rom');
    } catch (e) {
    }
    // var savs = findFiles('/default/'); 
    // savs.forEach(function(x) { console.log('!!!! sav: ' + x); });
    FCEM.updateGames();
    FCEM.updateStack();
    FCEM.showStack(true);
    // Write savegame and synchronize IDBFS in intervals.
    setInterval(Module.cwrap('FCEM_OnSaveGameInterval'), 1000);
  },
  onDeleteGameSyncFromIDB : function(er) {
    assert(!er);
    FCEM.updateGames();
    FCEM.updateStack();
  },
  onSyncToIDB : function(er) {
    assert(!er);
  },
  onDOMLoaded : function() {
    FCEM.showStack(false);
    FCEM.showControls(false);
  },
  startGame : function(path) {
    Module.romName = path;
    Module.romReload = 1;
  },
  updateGames : function() {
    var games = [];
    var addGamesIn = function(path, deletable) {
      var files = findFiles(path); 
      files.forEach(function(filename) {
        var split = PATH.splitPath(filename);
        var label = split[2].slice(0, -split[3].length).toUpperCase();
        var offset = calcGameOffset();
        games.push({label:label, path:filename, offset:offset, deletable:deletable});
      });
    };

    addGamesIn('/default/', false);
    addGamesIn('/fceux/rom/', true);

    // sort in alphabetic order and assign as new games list
    games.sort(function(a, b) {
      return (a.label < b.label) ? -1 : ((a.label > b.label) ? 1 : 0);
    });
    FCEM.games = games;
  },
  updateStack : function() {
    var games = FCEM.games;
    var stackContainer = document.getElementById("stackContainer");
    var scrollPos = stackContainer.scrollTop;
    var list = document.getElementById("stack");
    var proto = document.getElementById("cartProto");
  
    while (list.firstChild != list.lastChild) {
      list.removeChild(list.lastChild);
    }
  
    for (var i = 0; i < games.length; i++) {
      var item = games[i];
      var el = proto.cloneNode(true);
  
      el.dataset.idx = i;
      el.style.backgroundPosition = item.offset + 'px 0px';
      list.appendChild(el);
  
      var label = el.firstChild.firstChild.firstChild;
      label.innerHTML = item.label;

      label.style.fontSize = '16px';
      label.style.lineHeight = '18px';

      var fontSize = 13;
      while (label.scrollHeight > 18 && fontSize >= 9) {
        label.style.fontSize = fontSize + 'px';
        fontSize -= 2;
      }
      if (label.scrollHeight > 18) {
        label.style.lineHeight = '9px';
      }

      if (!item.deletable) {
        el.firstChild.lastChild.hidden = true;
      }
    }
  
    stackContainer.scrollTop = scrollPos;
  },
  setLocalKey : function(id, key) {
    localStorage['input' + id] = key;
  },
  getLocalKey : function(id) {
    return parseInt(localStorage['input' + id]);
  },
  resetKeys : function() {
    for (var id in FCEC.inputs) {
      var key = FCEM.getLocalKey(id);
      FCEM.bindKey(0, key);
      var item = FCEC.inputs[id];
      FCEM.setLocalKey(id, item[0]);
    }
  },
  mapAllKeys : function() {
    for (var id in FCEC.inputs) {
      var key = FCEM.getLocalKey(id);
      FCEM.bindKey(id, key);
    }
  },
  setupKeys : function() {
    if (!localStorage.getItem('inputInit')) {
      FCEM.resetKeys();
      localStorage['inputInit'] = 'true';
    }
    FCEM.mapAllKeys();
    FCEM.initKeyBind();
  },
  key2Name : function (key) {
    var keyName = (key & 0x0FF) ? KEY_CODE_TO_NAME[key & 0x0FF] : '(Undefined)';
    if (keyName === undefined) keyName = '(Unknown)';
    var prefix = '';
    if (key & 0x100 && keyName !== 'Ctrl')  prefix += 'Ctrl+';
    if (key & 0x400 && keyName !== 'Alt')   prefix += 'Alt+';
    if (key & 0x200 && keyName !== 'Shift') prefix += 'Shift+';
    if (key & 0x800 && keyName !== 'Meta')  prefix += 'Meta+';
    return prefix + keyName;
  },
  initKeyBind : function() {
    var table = document.getElementById("keyBindTable");
    var proto = document.getElementById("keyBindProto");

    while (table.lastChild != table.firstChild) {
      table.removeChild(table.lastChild);
    }

    for (id in FCEC.inputs) {
      var item = FCEC.inputs[id];
      var key = FCEM.getLocalKey(id);
      var keyName = FCEM.key2Name(key);

      var el = proto.cloneNode(true);
      el.children[0].innerHTML = item[1];
      el.children[1].innerHTML = keyName;
      el.children[2].dataset.id = id;
      el.children[2].dataset.name = item[1];

      table.appendChild(el);
    }
  },
  keyBindUnset : function(keyBind) {
    var id = keyBind.dataset.id;
    var key = FCEM.getLocalKey(id);
    if (key) {
      FCEM.bindKey(0, key);
    }
    FCEM.setLocalKey(id, 0);
    FCEM.initKeyBind();
  },
  keyBindResetAll : function() {
    FCEM.resetKeys();
    FCEM.mapAllKeys();
    FCEM.initKeyBind();
  },
  setLocalController : function(id, val) {
    localStorage['ctrl' + id] = Number(val);
    FCEM.setController(id, val);
  },
  setupControllers : function(force) {
    if (force || !localStorage.getItem('ctrlInit')) {
      for (var id in FCEC.controllers) {
        localStorage['ctrl' + id] = FCEC.controllers[id][0];
      }
      localStorage['ctrlInit'] = 'true';
    }
    for (var id in FCEC.controllers) {
      var val = Number(localStorage['ctrl' + id]);
      FCEM.setController(id, val);
      FCEV.setControllerEl(id, val);
    }
  },
};

var FCEV = {
catchEnabled : false,
catchId : null,
catchKey : null,
keyBindToggle : (function() {
	var el = document.getElementById("keyBindDiv");
	return function() {
		el.style.display = (el.style.display == 'block') ? 'none' : 'block';
	};
})(),
catchStart : (function(keyBind) {
	var nameEl = document.getElementById("catchName");
	var keyEl = document.getElementById("catchKey");
	var catchDivEl = document.getElementById("catchDiv");
	return function(keyBind) {
		var id = keyBind.dataset.id;
		FCEV.catchId = id;

		nameEl.innerHTML = keyBind.dataset.name;
		var key = FCEM.getLocalKey(id);
		FCEV.catchKey = key;
		keyEl.innerHTML = FCEM.key2Name(key);

		catchDivEl.style.display = 'block';

		FCEV.catchEnabled = true;
	};
})(),
catchEnd : (function(save) {
	var catchDivEl = document.getElementById("catchDiv");
	return function(save) {
		FCEV.catchEnabled = false;

		if (save && FCEV.catchId) {
		  // Check/overwrite duplicate bindings
		  for (var id in FCEC.inputs) {
		    var key = FCEM.getLocalKey(id);
		    if (FCEV.catchKey == key) {
		      if (!confirm('Key ' + FCEM.key2Name(key) + ' already bound as ' + FCEC.inputs[id][1] + '. Overwrite?')) {
		        FCEV.catchEnabled = true; // Re-enable key catching
		        return;
		      }
		      FCEM.setLocalKey(id, 0);
		      FCEM.bindKey(0, key);
		    }
		  }

		  // Set new binding
		  FCEM.setLocalKey(FCEV.catchId, FCEV.catchKey);
		  FCEM.bindKey(FCEV.catchId, FCEV.catchKey);
		  FCEV.catchId = null;
		  FCEM.initKeyBind();
		}

		catchDivEl.style.display = 'none';
	};
})(),
setControllerEl : function(id, val) {
	var el = document.getElementById(FCEC.controllers[id][1]);
	if (!el) {
		return;
	}
	if (el.tagName == 'SELECT' || el.type == 'range') {
		el.value = val;
	} else if (el.type == 'checkbox') {
		el.checked = val;
	}
},
setProgress : (function(x) {
	var el = document.getElementById('progress');
	return function(x) {
		x = (x > 1) ? 1 : ((x < 0) ? 0 : x);
		el.style.width = 3 * ((42*x) |0) + 'px';
	};
})(),
};

window.onbeforeunload = function (ev) {
  return 'To prevent save game data loss, please let the game run at least one second after saving and before closing the window.';
};

var loaderEl = document.getElementById('loader');

var Module = {
  preRun: [function() {
    // Mount IndexedDB file system (IDBFS) to /fceux.
    FS.mkdir('/fceux');
    FS.mount(IDBFS, {}, '/fceux');
  }],
  postRun: [function() {
    FCEM.setController = Module.cwrap('FCEM_SetController', null, ['number', 'number']);
    FCEM.bindKey = Module.cwrap('FCEM_BindKey', null, ['number', 'number']);
    FCEM.silenceSound = Module.cwrap('FCEM_SilenceSound', null, ['number']);
    // HACK: Disable default fullscreen handlers. See Emscripten's library_browser.js
    // The handlers forces the canvas size by setting css style width and height with
    // "!important" flag. Workaround is to disable the default fullscreen handlers.
    // See Emscripten's updateCanvasDimensions() in library_browser.js for the faulty code.
    Browser.fullScreenHandlersInstalled = true;
    // Initial IDBFS sync.
    FS.syncfs(true, FCEM.onInitialSyncFromIDB);
    // Setup configuration from localStorage.
    FCEM.setupControllers();
    FCEM.setupKeys();
  }],
  print: function() {
    text = Array.prototype.slice.call(arguments).join(' ');
    console.log(text);
  },
  printErr: function(text) {
    text = Array.prototype.slice.call(arguments).join(' ');
    console.error(text);
  },
  canvas: (function() {
    var el = document.getElementById('canvas');
// TODO: tsone: handle context loss, see: http://www.khronos.org/registry/webgl/specs/latest/1.0/#5.15.2
    el.addEventListener("webglcontextlost", function(e) { alert('WebGL context lost. You will need to reload the page.'); e.preventDefault(); }, false);
    return el;
  })(),
  setStatus: function(text) {
    var dl = 'Downloading data...';
// TODO: tsone: add startswith() method?
    if (text.substring(0, dl.length) === dl) {
      var r = text.match(/\(([\d.]+)\/([\d.]+)\)/);
      var x = parseFloat(r[1]) / parseFloat(r[2]);
      FCEV.setProgress(0.75 + 0.25*x);
    }
    if (!text) {
      Module.canvas.hidden = false;
      loaderEl.hidden = true;
    }
  },
  totalDependencies: 0,
  monitorRunDependencies: function(left) {
    this.totalDependencies = Math.max(this.totalDependencies, left);
    Module.setStatus(left ? 'Preparing... (' + (this.totalDependencies-left) + '/' + this.totalDependencies + ')' : 'All downloads complete.');
  }
};
Module.setStatus('Loading...');
window.onerror = function(event) {
  Module.setStatus = function(text) {
    if (text) Module.printErr('[post-exception status] ' + text);
  };
};

var req = new XMLHttpRequest();
req.addEventListener('progress', function(event) {
	var x = event.loaded / FCEC.FCEUX_JS_SIZE;
	FCEV.setProgress(0.75 * x);
}, false);
req.addEventListener('load', function(event) {
	var e = event.target;
	var blob = new Blob([ e.responseText ]);
	var s = document.createElement('script');
	var url = URL.createObjectURL(blob);
	s.onload = s.onerror = function() {
		URL.revokeObjectURL(url);
	}
	s.src = url;
	document.documentElement.appendChild(s);
}, false);
req.open("GET", "fceux.js", true);
req.send();

function dragHandler(text) {
  return function(ev) {
    ev.stopPropagation();
    ev.preventDefault();
  };
}
function dropHandler(ev) {
  ev.stopPropagation();
  ev.preventDefault();
  var f = ev.dataTransfer.files[0];
  if (f && confirm('Do you want to run the game ' + f.name + ' and add it to stack?')) {
    var r = new FileReader();
    r.onload = function(e) { 
      var opts = {encoding:'binary'};
      var path = PATH.join2('/fceux/rom/', f.name);
      FS.writeFile(path, new Uint8Array(e.target.result), opts);
      FCEM.updateGames();
      FCEM.updateStack();
      FCEM.startGame(path);
    }
    r.readAsArrayBuffer(f);
  }
}

document.addEventListener('dragenter', dragHandler('enter'), false);
document.addEventListener('dragleave', dragHandler('leave'), false);
document.addEventListener('dragover', dragHandler('over'), false);
document.addEventListener('drop', dropHandler, false);

var current = 0;

function calcGameOffset() {
  return 3 * ((3*Math.random()) |0);
}

function askConfirmGame(ev, el, q) {
  var games = FCEM.games;
  ev.stopPropagation();
  ev.preventDefault();
  var idx = current + (el.dataset.idx |0);
  if ((idx >= 0) && (idx < games.length) && confirm(q + ' ' + games[idx].label + '?')) {
    return idx;
  } else {
    return -1;
  }
}

function askSelectGame(ev, el) {
  var idx = askConfirmGame(ev, el, 'Do you want to play');
  if (idx != -1) {
    FCEM.startGame(FCEM.games[idx].path);
    setTimeout(function() { FCEM.showStack(false); FCEM.showControls(false); }, 1000);
  }
  return false;
}

function askDeleteGame(ev, el) {
  var idx = askConfirmGame(ev, el, 'Do you want to delete');
  if (idx != -1) {
    idx = askConfirmGame(ev, el, 'ARE YOU REALLY SURE YOU WANT TO DELETE');
    if (idx != -1) {
      var item = FCEM.games.slice(idx, idx+1)[0];
      FS.unlink(item.path);
      FS.syncfs(FCEM.onDeleteGameSyncFromIDB);
    }
  }
  return false;
}

function findFiles(startPath) {
  var entries = [];

  function isRealDir(p) {
    return p !== '.' && p !== '..';
  };
  function toAbsolute(root) {
    return function(p) {
      return PATH.join2(root, p);
    }
  };

  try {
      return FS.readdir(startPath).filter(isRealDir).map(toAbsolute(startPath));
  } catch (e) {
      return [];
  }
// TODO: remove, this is for recursive search
/*
  while (check.length) {
    var path = check.pop();
    var stat;

    try {
      stat = FS.stat(path);
    } catch (e) {
      return [];
    }

    if (FS.isDir(stat.mode)) {
      check.push.apply(check, FS.readdir(path).filter(isRealDir).map(toAbsolute(path)));
    }

    entries.push(path);
  }

  return entries;
*/
}

document.addEventListener("keydown", function(e) {
  if (!FCEV.catchEnabled) {
    return;
  }
  var key = e.keyCode & 0x0FF;
  if (e.metaKey)  key |= 0x800;
  if (e.altKey)   key |= 0x400;
  if (e.shiftKey) key |= 0x200;
  if (e.ctrlKey)  key |= 0x100;

  var el = document.getElementById("catchKey");
  el.innerHTML = FCEM.key2Name(key);

  FCEV.catchKey = key;
});

document.addEventListener('DOMContentLoaded', FCEM.onDOMLoaded, false);

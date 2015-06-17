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
  catchEnabled : false,
  catchKey : null,
  catchId : null,
  games : [],
  stackToggleElem : null,
  controlsToggleElem : null,
  soundIconElem : null,
  soundEnabled : true,
  showStack : function(show) {
    FCEM.stackToggleElem.checked = (show === undefined) ? !FCEM.stackToggleElem.checked : show;
  },
  showControls : function(show) {
    FCEM.controlsToggleElem.checked = (show === undefined) ? !FCEM.controlsToggleElem.checked : show;
  },
  toggleSound : function() {
    FCEM.soundEnabled = !FCEM.soundEnabled;
    FCEM.setController(SOUND_ENABLED, FCEM.soundEnabled);
    FCEM.soundIconElem.src = FCEM.soundEnabled ? 'img/sound_on.gif' : 'img/sound_off.gif';
  },
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
    FCEM.setController = Module.cwrap('FCEM_SetController', null, ['number', 'number']);
    FCEM.bindKey = Module.cwrap('FCEM_BindKey', null, ['number', 'number']);
    // Write savegame and synchronize IDBFS in intervals.
    setInterval(Module.cwrap('FCEM_OnSaveGameInterval'), 1000);

    FCEM.setupKeys();
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
    FCEM.stackToggleElem = document.getElementById('stackToggle');
    FCEM.controlsToggleElem = document.getElementById('controlsToggle');
    FCEM.soundIconElem = document.getElementById('soundIcon');
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
      return a < b ? -1 : a > b ? 1 : 0;
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
      label.style.fontSize = '12px';
      label.style.lineHeight = '16px';
      label.innerHTML = item.label;
      if (label.scrollHeight > 18) {
        label.style.fontSize = '8px';
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
    for (var id in FCEM.inputs) {
      var key = FCEM.getLocalKey(id);
      FCEM.bindKey(0, key);
      var item = FCEM.inputs[id];
      FCEM.setLocalKey(id, item[0]);
    }
  },
  mapAllKeys : function() {
    for (var id in FCEM.inputs) {
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

    while (table.lastChild) {
      table.removeChild(table.lastChild);
    }

    for (id in FCEM.inputs) {
      var item = FCEM.inputs[id];
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
  catchStart : function(keyBind) {
    var id = keyBind.dataset.id;
    FCEM.catchId = id;
  
    var nameEl = document.getElementById("catchName");
    nameEl.innerHTML = keyBind.dataset.name;
    var keyEl = document.getElementById("catchKey");
    var key = FCEM.getLocalKey(id);
    FCEM.catchKey = key;
    keyEl.innerHTML = FCEM.key2Name(key);
  
    var catchDivEl = document.getElementById("catchDiv");
    catchDivEl.style.display = 'block';
  
    FCEM.catchEnabled = true;
  },
  catchEnd : function(save) {
    FCEM.catchEnabled = false;

    if (save && FCEM.catchId) {
      // Check/overwrite duplicate bindings
      for (var id in FCEM.inputs) {
        var key = FCEM.getLocalKey(id);
        if (FCEM.catchKey == key) {
          if (!confirm('Key ' + FCEM.key2Name(key) + ' already bound as ' + FCEM.inputs[id][1] + '. Overwrite?')) {
            FCEM.catchEnabled = true; // Re-enable key catching
            return;
          }
          FCEM.setLocalKey(id, 0);
          FCEM.bindKey(0, key);
        }
      }

      // Set new binding
      FCEM.setLocalKey(FCEM.catchId, FCEM.catchKey);
      FCEM.bindKey(FCEM.catchId, FCEM.catchKey);
      FCEM.catchId = null;
      FCEM.initKeyBind();
    }

    var catchDivEl = document.getElementById("catchDiv");
    catchDivEl.style.display = 'none';
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
  keyBindToggle : function() {
    var keyBindDivEl = document.getElementById("keyBindDiv");
    keyBindDivEl.style.display = (keyBindDivEl.style.display == 'block') ? 'none' : 'block';
  }
};

window.onbeforeunload = function (ev) {
  return 'To prevent save game data loss, please let the game run at least one second after saving and before closing the window.';
};

var loaderElem = document.getElementById('loader');

var Module = {
  preRun: [function() {
    // Mount IndexedDB file system (IDBFS) to /fceux.
    FS.mkdir('/fceux');
    FS.mount(IDBFS, {}, '/fceux');
    FS.syncfs(true, FCEM.onInitialSyncFromIDB);
  }],
  postRun: [],
  print: function() {
    text = Array.prototype.slice.call(arguments).join(' ');
    console.log(text);
  },
  printErr: function(text) {
    text = Array.prototype.slice.call(arguments).join(' ');
    console.error(text);
  },
  canvas: (function() {
    var canvasElement = document.getElementById('canvas');

    // As a default initial behavior, pop up an alert when webgl context is lost. To make your
    // application robust, you may want to override this behavior before shipping!
    // See http://www.khronos.org/registry/webgl/specs/latest/1.0/#5.15.2
    canvasElement.addEventListener("webglcontextlost", function(e) { alert('WebGL context lost. You will need to reload the page.'); e.preventDefault(); }, false);

    return canvasElement;
  })(),
  setStatus: function(text) {
    if (!Module.setStatus.last) Module.setStatus.last = { time: Date.now(), text: '' };
    if (text === Module.setStatus.text) return;
    var m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
    var now = Date.now();
    if (m && now - Date.now() < 30) return;
    if (m) {
      text = m[1];
    } else {
      if (!text) {
        Module.canvas.hidden = false;
        loaderElem.hidden = true;
      }
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

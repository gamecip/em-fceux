// NOTE: do not change values!
var BRIGHTNESS = 0;
var CONTRAST = 1;
var COLOR = 2;
var GAMMA = 3;
var GLOW = 4;
var SHARPNESS = 5;
var RGBPPU = 6;
var CRT_ENABLED = 7;
var SCANLINES = 8;
var CONVERGENCE = 9;
var NOISE = 10;
var SOUND_ENABLED = 11;
var FCEM = {
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
    FCEM.setControl(SOUND_ENABLED, FCEM.soundEnabled);
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
    FCEM.setControl = Module.cwrap('FCEM_setControl', null, ['number', 'number']);
    // Write savegame and synchronize IDBFS in intervals.
    setInterval(Module.cwrap('FCEM_onSaveGameInterval'), 1000);
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
  
      el.id = i;
      el.className = 'cart';
      el.style.backgroundPosition = item.offset + 'px 0px';
      list.appendChild(el);
  
      var label = el.firstChild.firstChild;
      label.style.fontSize = '12px';
      label.style.lineHeight = '16px';
      label.innerHTML = item.label;
      if (label.scrollHeight > 18) {
        label.style.fontSize = '8px';
        label.style.lineHeight = '9px';
      }

      if (!item.deletable) {
        el.lastChild.hidden = true;
      }
    }
  
    stackContainer.scrollTop = scrollPos;
  }
};

window.onbeforeunload = function (ev) {
  var msg = 'Do you want to close the FCEUX emulator and the current game? Unsaved game data will be lost.';
  (ev || window.event).returnValue = msg; 
  return msg;
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

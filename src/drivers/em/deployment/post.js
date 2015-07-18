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
var req = new XMLHttpRequest();
req.addEventListener('progress', function(event) {
	var x = event.loaded / FCEUX_JS_SIZE;
	FCEM.setProgress(0.75 * x);
}, false);
req.addEventListener('load', function(event) {
	var e = event.target;
	var s = document.createElement("script");
	s.innerHTML = e.responseText;
	document.documentElement.appendChild(s);

// TODO: tsone: add action when script is loaded?
//	s.addEventListener("load", function() {});
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
  if (!FCEM.catchEnabled) {
    return;
  }
  var key = e.keyCode & 0x0FF;
  if (e.metaKey)  key |= 0x800;
  if (e.altKey)   key |= 0x400;
  if (e.shiftKey) key |= 0x200;
  if (e.ctrlKey)  key |= 0x100;

  var el = document.getElementById("catchKey");
  el.innerHTML = FCEM.key2Name(key);

  FCEM.catchKey = key;
});

document.addEventListener('DOMContentLoaded', FCEM.onDOMLoaded, false);

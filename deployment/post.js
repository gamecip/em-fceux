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
  if (f && confirm('want to add game: ' + f.name + '?')) {
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
  var idx = current + (el.id |0);
  if ((idx >= 0) && (idx < games.length) && confirm(q + ' ' + games[idx].label + '?')) {
    return idx;
  } else {
    return -1;
  }
}

function askSelectGame(ev, el) {
  var idx = askConfirmGame(ev, el, 'want to start');
  if (idx != -1) {
    FCEM.startGame(FCEM.games[idx].path);
    setTimeout(function() { FCEM.showStack(false); FCEM.showControls(false); }, 1000);
  }
  return false;
}

function askDeleteGame(ev, el) {
  var idx = askConfirmGame(ev, el, 'want to delete');
  if (idx != -1) {
    var item = FCEM.games.slice(idx, idx+1)[0];
    FS.unlink(item.path);
    FS.syncfs(FCEM.onDeleteGameSyncFromIDB);
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

document.addEventListener('DOMContentLoaded', FCEM.onDOMLoaded, false);

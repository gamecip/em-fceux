# EM-FCEUX #

em-fceux is an Emscripten port of FCEUX 2.2.2 emulator.

Play it here: https://tsone.kapsi.fi/em-fceux/

Emscripten project site: http://emscripten.org

FCEUX project site: http://www.fceux.com/


### OVERVIEW ###

The goal of em-fceux is to enable FCEUX in the modern web browsers.
This means real-time frame rates and interactivity at around 60 fps,
high-quality visuals, support for audio, battery-backed save RAM, save
states and gamepads.

Additional goal is to emulate late 80's / early 90's console gaming
experience by providing NTSC signal and CRT TV emulation and a
"game stack" in the client/browser.


### FEATURES ###

em-fceux contains major modifications to make the FCEUX code more suitable
for Emscripten and web browsers. It also adds a new Emscripten driver that
uses OpenGL ES 2.0 for rendering. This enables the use of shaders to
emulate NTSC signal and analog CRT TV output.

New features:

* Optimizations to achieve around 60 fps interactivity in web browsers
* NTSC composite video emulation
* CRT TV emulation

Notable supported FCEUX features:

* Both NTSC and PAL system emulation
* Save states and battery-backed SRAM
* Speed throttling
* Audio
* Support for two game controllers
* Zapper support
* Support for gamepads/joysticks
* Custom keyboard bindings
* Support for .nes, .zip and .nsf file formats

Notable *unsupported* FCEUX features:

* New PPU emulation (old PPU emulation is used for performance)
* FDS disk system
* VS system
* Special peripherals: Family Keyboard, Mahjong controller etc.
* Screenshots and movie recording
* Cheats, TAS features and Lua scripting


### BUILD ###

em-fceux can be built under Linux or Unix. Windows is not currently supported.

First install Emscripten (tested on: 1.34.6, 1.32.0) by following instructions in:
http://emscripten.org/

Then you also need scons build tool, gzip and Python 2.7.x. Their installation
depends on the operating system.

Build the source with ./embuild.sh bash script in the em-fceux directory root.

```
# Release build:
./embuild.sh

# Debug build:
./embuild.sh RELEASE=0
```

After successful build, the src/drivers/em/site/ directory will contain
the em-fceux "binaries": fceux.js, fceux.js.mem, fceux.data and their respective
gzipped versions. To deploy and test, please follow the steps in the RUN / DEPLOY
section.


### BUILD SHADERS (OPTIONAL) ###

If the shaders in src/drivers/em/assets/shaders/ have been modified,
run ./buildshaders.sh in the em-fceux root. This requires a *glslopt* binary
(=glsl optimizer). Currently (28-Sep-2015) one way to get it is to build
it from the source as follows:

```
git clone https://github.com/aras-p/glsl-optimizer.git ../glsl-optimizer
cd ../glsl-optimizer
cmake . && make glslopt
```

Note, you must run ./buildshaders.sh before the ./embuild.sh script. This is because
the shaders are copied/embedded in the file system created by the latter script.


### RUN / DEPLOY ###

To make deployment build, run the ./embuildsite.sh in the em-fceux root.
This will create a deployable directory in src/drivers/em/deploy/
It contains all content for the em-fceux site.

To test em-fceux locally, run 'python -m SimpleHTTPServer' in the
src/drivers/em/deploy/ directory and navigate your web browser to
http://localhost:8000/

The ./embuildsite.sh script suffixes all static site content with git file hash
for explicit cache/version control.

Note, to take advantage of the pre-built .gz files, you must set HTTP server to
forward request to these files instead of the uncompressed ones and also to set
the transfer encoding headers properly. It's also good to have some cache control.
For Apache, please refer to an example .htaccess file in src/drivers/em/assets/
which does these things.


### SUMMARY ###

```
# Build a em-fceux and the site:
./embuild.sh && ./embuildsite.sh

# ...also build the shaders:
./buildshaders.sh && ./embuild.sh && ./embuildsite.sh
```

### CONTACT ###

Please submit bugs and feature requests in the issue tracker here:
https://bitbucket.org/tsone/em-fceux/issues/new

em-fceux is written and maintained by Valtteri "tsone" Heikkilä.
Send a bitbucket message to user "tsone".

FCEUX 2.2.2 emulator is written and maintained by the FCEUX project team:
http://www.fceux.com/web/contact.html


### TECHNICAL ###

em-fceux utilizes web browser client-side storage (Emscripten IndexedDB
file system) to store the games, save data and save states. None of these
are ever transmitted out from the client/browser.

Emulator settings and input bindings are stored in localStorage.
Web Audio API is used for the audio and Web Gamepad API for the gamepads.

em-fceux implements NTSC signal emulation by modeling the composite YIQ
output. The separation of YIQ luminance (Y) and chrominance (IQ/UV) is
achieved with a technique known as "1D comb filter" which reduces chroma
fringing compared to band or low-pass methods (and is also simple to
implement). Under the hood, YIQ to RGB conversion relies on a large lookup
table (texture) which is referenced in a fragment shader.


### LEGAL / OTHER ###

FCEUX and em-fceux are licensed under GNU GPL Version 2:
https://www.gnu.org/licenses/gpl-2.0.txt

em-fceux is based on the source code release of FCEUX 2.2.2 emulator: 
http://sourceforge.net/projects/fceultra/files/Source%20Code/2.2.2%20src/fceux-2.2.2.src.tar.gz/download

The built-in games (binaries, ROMs) in em-fceux are distributed with permission
from the game authors who retain all the rights to the games. The games are
not licensed with GNU GPL Version 2.
# EM-FCEUX #

em-fceux is an Emscripten port of FCEUX 2.2.2.

More about Emscripten: http://emscripten.org

More about FCEUX: http://www.fceux.com/


### OVERVIEW ###

The goal of em-fceux is to enable FCEUX in the modern web browsers.
Primarily this means real-time frame rates and interactivivity at 60 fps,
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

* Emscripten-specific optimizations for 60 fps interactivity in web browsers
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
* Support for .zip and .nsf file formats

Notable *unsupported* FCEUX features:

* FDS disk system
* VS system
* Special peripherals: Family Keyboard, Mahjong controller etc.
* Screenshots and movie recording
* Cheats, TAS features and Lua scripting


### BUILD ###

em-fceux is built under Linux or Unix. First install Emscripten 1.32.0
(tested) by following instructions in: http://emscripten.org/

You also need Python 2.7.x and the scons build tool. Their installation
depends on the operating system.

Build the source with ./embuild.sh script in the em-fceux directory root.

Release build: ./embuild.sh RELEASE=1

Debug build: ./embuild.sh RELEASE=0


### RUN / DEPLOY ###

After successfull build, the deployment/ directory will contain the
complete em-fceux "web app". To deploy, simply copy all the contents of
the deployment/ directory to a web server.

To test em-fceux locally, run 'python -m SimpleHTTPServer' in the deployment/
directory. Then navigate web browser to http://localhost:8000/

Please refer to em-fceux help (the '?' icon) for help.


### CONTACT ###

em-fceux is written by Valtteri "tsone" Heikkilä (tsone at bitbucket.org).

FCEUX 2.2.2 is written by the FCEUX project team: http://www.fceux.com/web/contact.html


### TECHNICAL ###

em-fceux utilizes web browser client-side storage (Emscripten IndexedDB
file system) to store the games, save data and save states. None of these
are ever transmitted out from the client/browser.

Emulator settings and keyboard bindings are stored in localStorage.
Web Audio API is used for the audio.

em-fceux implements NTSC signal emulation by modeling the composite YIQ
output. The separation of YIQ luminance (Y) and chrominance (IQ/UV) is
achieved with a technique known as "1D comb filter" which reduces chroma
fringing compared to band or low-pass methods (and is also simple to
implement). Under the hood, YIQ to RGB conversion relies on a large lookup
table (texture) which is referenced in a fragment shader.


### LEGAL / OTHER ###

FCEUX and em-fceux are licensed under GNU GPL Version 2:
https://www.gnu.org/licenses/gpl-2.0.txt

em-fceux is based on the source code release of FCEUX 2.2.2: 
http://sourceforge.net/projects/fceultra/files/Source%20Code/2.2.2%20src/fceux-2.2.2.src.tar.gz/download


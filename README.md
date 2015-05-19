# EM-FCEUX #

em-fceux is an Emscripten port of FCEUX 2.2.2.

More about Emscripten: http://emscripten.org

More about FCEUX: http://www.fceux.com/


### OVERVIEW ###

The goal of em-fceux is to enable FCEUX in the modern web browsers.
Primarily this means real-time frame rates and interactivivity at 60 fps,
high-quality fullscreen visuals, support for audio, battery-backed
save RAM, save states and gamepads.

Additional goal is to emulate late 80's / early 90's console gaming
experience by providing NTSC and CRT emulation and a "game stack"
in the client/browser.


### FEATURES ###

em-fceux contains major modifications to make the FCEUX code more suitable
for Emscripten and web browsers. It also adds a new Emscripten driver that
uses OpenGL ES 2.0 for rendering. The use of GPU allows the driver to emulate
NTSC signal and analog CRT output via shaders.

New features:

* Emscripten-specific optimizations for interactive 60 fps in web browsers
* NTSC composite video emulation
* CRT TV emulation

Notable supported FCEUX features:

* NTSC system emulation (USA, Japan)
* Audio support
* Keyboard input (for one game controller)
* Gamepad input
* Save states
* Speed throttling
* Support for .zip and .nsf file formats

Notable unsupported FCEUX features:

* PAL system emulation (Europe)
* FDS disk system
* VS system
* Special peripherals: Zapper, Family Keyboard, Mahjong controller etc.
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

Please refer to em-fceux help (the '?' icon) for the emulator usage,
keyboard mappings and the like.


### CONTACT ###

em-fceux is written by Valtteri "tsone" Heikkilä (tsone at bitbucket.org).

FCEUX 2.2.2 is written by the FCEUX project team: http://www.fceux.com/web/contact.html


### TECHNICAL ###

em-fceux utilizes web browser client-side storage (Emscripten IndexedDB
file system) to store the games, save data and save states. None of these
are ever transmitted out from the client/browser.

em-fceux implements NTSC emulation by modeling the composite YIQ signal
output. The separation of luminance (Y) and chrominance (IQ/UV) is achieved
with "1D comb filter" to reduce chroma fringing. The YIQ to RGB conversion
is done in a shader.


### LEGAL / OTHER ###

FCEUX and em-fceux are licensed under GNU GPL Version 2:
https://www.gnu.org/licenses/gpl-2.0.txt

em-fceux is based on the source code release of FCEUX 2.2.2: 
http://sourceforge.net/projects/fceultra/files/Source%20Code/2.2.2%20src/fceux-2.2.2.src.tar.gz/download
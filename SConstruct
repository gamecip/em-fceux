#
# SConstruct - build script for the emscripten/SDL port of fceux
#
# You can adjust the BoolVariables below to include/exclude features 
# at compile-time.  You may also use arguments to specify the parameters.
#   ie: scons RELEASE=1 GTK3=1
#
# Use "scons" to compile and "scons install" to install.
#

import os
import sys
import platform 

opts = Variables(None, ARGUMENTS)
opts.AddVariables( 
  BoolVariable('DEBUG',     'Build with debugging symbols', 1),
  BoolVariable('RELEASE',   'Set to 1 to build for release', 0),
  BoolVariable('FRAMESKIP', 'Enable frameskipping', 1),
  BoolVariable('OPENGL',    'Enable OpenGL support', 1),
  BoolVariable('LUA',       'Enable Lua support', 1),
  BoolVariable('GTK', 'Enable GTK2 GUI (SDL only)', 1),
  BoolVariable('GTK3', 'Enable GTK3 GUI (SDL only)', 0),
  BoolVariable('X11', 'Link with X11 (SDL only)', 1),
  BoolVariable('NEWPPU',    'Enable new PPU core', 1),
  BoolVariable('CREATE_AVI', 'Enable avi creation support (SDL only)', 1),
  BoolVariable('LOGO', 'Enable a logoscreen when creating avis (SDL only)', 1),
  BoolVariable('SYSTEM_LUA','Use system lua instead of static lua provided with fceux', 1),
  BoolVariable('SYSTEM_MINIZIP', 'Use system minizip instead of static minizip provided with fceux', 0),
  BoolVariable('LSB_FIRST', 'Least signficant byte first (non-PPC)', 1),
  BoolVariable('CLANG', 'Compile with llvm-clang instead of gcc', 0),
  BoolVariable('SDL2', 'Compile using SDL2 instead of SDL 1.2 (experimental/non-functional)', 0),
  BoolVariable('NOCHEAT',   'Completely disable all cheats', 0)
)
AddOption('--prefix', dest='prefix', type='string', nargs=1, action='store', metavar='DIR', help='installation prefix')

prefix = GetOption('prefix')
env = Environment(options = opts)

if 'EMSCRIPTEN_TOOL_PATH' in os.environ:
  env['EMSCRIPTEN'] = 1
  env['GTK'] = 0
  env['GTK3'] = 0
  env['X11'] = 0
  env['LUA'] = 0
  env['SDL2'] = 0
  env['SYSTEM_LUA'] = 0
  env['SYSTEM_MINIZIP'] = 0
  env['NEWPPU'] = 0 # Not strictly necessary, but faster.
  env['CREATE_AVI'] = 0
  env['LOGO'] = 0
  env['NOCHEAT'] = 1
  env.Tool('emscripten', toolpath=[os.environ['EMSCRIPTEN_TOOL_PATH']])
  env.Replace(PROGSUFFIX = [".js"])
  exportsList = [
    'main',
    'FCEM_OnSaveGameInterval',
    'FCEM_SetController',
    'FCEM_BindKey',
    'FCEM_SilenceSound'
  ]
  exports = '-s EXPORTED_FUNCTIONS=\'["_' + '","_'.join(exportsList) + '"]\''
  env.Append(LINKFLAGS = exports)
  env.Append(LINKFLAGS = '--preload-file src/drivers/em/assets/games/@/default/')
  env.Append(LINKFLAGS = '-s USE_SDL=2')
  env.Append(CCFLAGS = '-s USE_SDL=2')
else:
  env['EMSCRIPTEN'] = 0

if env['RELEASE']:
  env.Append(CPPDEFINES=["PUBLIC_RELEASE"])
  env['DEBUG'] = 0

# LSB_FIRST must be off for PPC to compile
if platform.system == "ppc":
  env['LSB_FIRST'] = 0

env['LIBS'] = []

# Default compiler flags:
env.Append(CCFLAGS = ['-Wall', '-Werror', '-Wno-write-strings', '-Wno-sign-compare', '-Wno-unused-local-typedefs'])

if os.environ.has_key('PLATFORM'):
  env.Replace(PLATFORM = os.environ['PLATFORM'])
if os.environ.has_key('CC'):
  env.Replace(CC = os.environ['CC'])
if os.environ.has_key('CXX'):
  env.Replace(CXX = os.environ['CXX'])
if os.environ.has_key('WINDRES'):
  env.Replace(WINDRES = os.environ['WINDRES'])
if os.environ.has_key('CFLAGS'):
  env.Append(CCFLAGS = os.environ['CFLAGS'].split())
if os.environ.has_key('CXXFLAGS'):
  env.Append(CXXFLAGS = os.environ['CXXFLAGS'].split())
if os.environ.has_key('CPPFLAGS'):
  env.Append(CPPFLAGS = os.environ['CPPFLAGS'].split())
if os.environ.has_key('LDFLAGS'):
  env.Append(LINKFLAGS = os.environ['LDFLAGS'].split())
if os.environ.has_key('PKG_CONFIG_PATH'):
  env['ENV']['PKG_CONFIG_PATH'] = os.environ['PKG_CONFIG_PATH']
if os.environ.has_key('PKG_CONFIG_LIBDIR'):
  env['ENV']['PKG_CONFIG_LIBDIR'] = os.environ['PKG_CONFIG_LIBDIR']

print "platform: ", env['PLATFORM']

# compile with clang
if env['CLANG']:
  env.Replace(CC='clang')
  env.Replace(CXX='clang++')

# special flags for cygwin
# we have to do this here so that the function and lib checks will go through mingw
if env['PLATFORM'] == 'cygwin':
  env.Append(CCFLAGS = " -mno-cygwin")
  env.Append(LINKFLAGS = " -mno-cygwin")
  env['LIBS'] = ['wsock32'];

if env['NOCHEAT']:
  env.Append(CPPDEFINES=["NOCHEAT"])

if env['PLATFORM'] == 'win32':
  env.Append(CPPPATH = [".", "drivers/win/", "drivers/common/", "drivers/", "drivers/win/zlib", "drivers/win/directx", "drivers/win/lua/include"])
  env.Append(CPPDEFINES = ["PSS_STYLE=2", "WIN32", "_USE_SHARED_MEMORY_", "NETWORK", "FCEUDEF_DEBUGGER", "NOMINMAX", "NEED_MINGW_HACKS", "_WIN32_IE=0x0600"])
  env.Append(LIBS = ["rpcrt4", "comctl32", "vfw32", "winmm", "ws2_32", "comdlg32", "ole32", "gdi32", "htmlhelp"])
else:
  conf = Configure(env)
  if env['DEBUG'] and not env['EMSCRIPTEN']:
    # If libdw is available, compile in backward-cpp support
    if conf.CheckLib('dw'):
      conf.env.Append(CCFLAGS = "-DBACKWARD_HAS_DW=1")
      conf.env.Append(LINKFLAGS = "-ldw")
  if conf.CheckFunc('asprintf'):
    conf.env.Append(CCFLAGS = "-DHAVE_ASPRINTF")
  if env['SYSTEM_MINIZIP']:
    assert conf.CheckLibWithHeader('minizip', 'minizip/unzip.h', 'C', 'unzOpen;', 1), "please install: libminizip"
    assert conf.CheckLibWithHeader('z', 'zlib.h', 'c', 'inflate;', 1), "please install: zlib"
    env.Append(CPPDEFINES=["_SYSTEM_MINIZIP"])
  elif env['EMSCRIPTEN']:
    env.Append(CPPPATH = ["drivers/win/zlib"])
  else:
    assert conf.CheckLibWithHeader('z', 'zlib.h', 'c', 'inflate;', 1), "please install: zlib"
  if env['SDL2']:
    if not conf.CheckLib('SDL2'):
      print 'Did not find libSDL2 or SDL2.lib, exiting!'
      Exit(1)
    env.Append(CPPDEFINES=["_SDL2"])
    env.ParseConfig('pkg-config sdl2 --cflags --libs')
  elif not env['EMSCRIPTEN']:
    if not conf.CheckLib('SDL'):
      print 'Did not find libSDL or SDL.lib, exiting!'
      Exit(1)
    env.ParseConfig('sdl-config --cflags --libs')
  if env['GTK']:
    if not conf.CheckLib('gtk-x11-2.0'):
      print 'Could not find libgtk-2.0, exiting!'
      Exit(1)
    # Add compiler and linker flags from pkg-config
    config_string = 'pkg-config --cflags --libs gtk+-2.0'
    if env['PLATFORM'] == 'darwin' and env['X11']:
      config_string = 'PKG_CONFIG_PATH=/opt/X11/lib/pkgconfig/ ' + config_string
    env.ParseConfig(config_string)
    env.Append(CPPDEFINES=["_GTK2"])
    env.Append(CCFLAGS = ["-D_GTK"])
  if env['GTK3']:
    # Add compiler and linker flags from pkg-config
    config_string = 'pkg-config --cflags --libs gtk+-3.0'
    if env['PLATFORM'] == 'darwin' and env['X11']:
      config_string = 'PKG_CONFIG_PATH=/opt/X11/lib/pkgconfig/ ' + config_string
    env.ParseConfig(config_string)
    env.Append(CPPDEFINES=["_GTK3"])
    env.Append(CCFLAGS = ["-D_GTK"])

  ### Lua platform defines
  ### Applies to all files even though only lua needs it, but should be ok
  if env['LUA']:
    env.Append(CPPDEFINES=["_S9XLUA_H"])
    if env['PLATFORM'] == 'darwin':
      # Define LUA_USE_MACOSX otherwise we can't bind external libs from lua
      env.Append(CCFLAGS = ["-DLUA_USE_MACOSX"])    
    if env['PLATFORM'] == 'posix':
      # If we're POSIX, we use LUA_USE_LINUX since that combines usual lua posix defines with dlfcn calls for dynamic library loading.
      # Should work on any *nix
      env.Append(CCFLAGS = ["-DLUA_USE_LINUX"])
    lua_available = False
    if conf.CheckLib('lua5.1'):
      env.Append(LINKFLAGS = ["-ldl", "-llua5.1"])
      env.Append(CCFLAGS = ["-I/usr/include/lua5.1"])
      lua_available = True
    elif conf.CheckLib('lua'):
      env.Append(LINKFLAGS = ["-ldl", "-llua"])
      env.Append(CCFLAGS = ["-I/usr/include/lua"])
      lua_available = True
    if lua_available == False:
      print 'Could not find liblua, exiting!'
      Exit(1)
  # "--as-needed" no longer available on OSX (probably BSD as well? TODO: test)
  if env['PLATFORM'] != 'darwin':
    env.Append(LINKFLAGS=['-Wl,--as-needed'])
  
  ### Search for gd if we're not in Windows
  if env['PLATFORM'] != 'win32' and env['PLATFORM'] != 'cygwin' and env['CREATE_AVI'] and env['LOGO']:
    gd = conf.CheckLib('gd', autoadd=1)
    if gd == 0:
      env['LOGO'] = 0
      print 'Did not find libgd, you won\'t be able to create a logo screen for your avis.'
   
  if env['OPENGL'] and (env['EMSCRIPTEN'] or conf.CheckLibWithHeader('GL', 'GL/gl.h', 'c', autoadd=1)):
    conf.env.Append(CCFLAGS = "-DOPENGL")
        
  conf.env.Append(CPPDEFINES = ['PSS_STYLE=1'])
  
  env = conf.Finish()

if sys.byteorder == 'little' or env['PLATFORM'] == 'win32':
  env.Append(CPPDEFINES = ['LSB_FIRST'])

if env['FRAMESKIP']:
  env.Append(CPPDEFINES = ['FRAMESKIP'])

print "base CPPDEFINES:",env['CPPDEFINES']
print "base CCFLAGS:",env['CCFLAGS']

if env['DEBUG']:
  env.Append(CPPDEFINES=["_DEBUG"], CCFLAGS = ['-g', '-O0'])
else:
  if env['EMSCRIPTEN']:
    common = ' --llvm-opts 3'
    common += ' --llvm-lto 3'
    common += ' -s NO_EXIT_RUNTIME=1 -s AGGRESSIVE_VARIABLE_ELIMINATION=1'
    common += ' -s DISABLE_EXCEPTION_CATCHING=1'
    common += ' -s ASSERTIONS=0'
    env.Append(CCFLAGS = '-flto -O3' + common)
    env.Append(LINKFLAGS = '-flto -O3 -strip-all' + common)
  else:
    env.Append(CCFLAGS = ['-O2'])
    env.Append(LINKFLAGS = ['-O2'])

if env['PLATFORM'] != 'win32' and env['PLATFORM'] != 'cygwin' and env['CREATE_AVI']:
  env.Append(CPPDEFINES=["CREATE_AVI"])
else:
  env['CREATE_AVI']=0;

Export('env')
fceux = SConscript('src/SConscript')
if env['EMSCRIPTEN']:
  target = str(fceux[0])
  target_dir = os.path.dirname(target)
  gzip_src = [
    target,
    target + '.mem',
    target[:-3] + '.data',
    os.path.join(target_dir, 'style.css'),
    os.path.join(target_dir, 'pre.js'),
    os.path.join(target_dir, 'post.js')
  ]
  for f in gzip_src:
    env.Command(f+'.gz', f, 'gzip --best -c $SOURCE > $TARGET')
else:
  env.Program(target="fceux-net-server", source=["fceux-server/server.cpp", "fceux-server/md5.cpp", "fceux-server/throttle.cpp"])
  
  # Installation rules
  if prefix == None:
    prefix = "/usr/local"
  
  exe_suffix = ''
  if env['PLATFORM'] == 'win32':
    exe_suffix = '.exe'
  
  fceux_src = 'src/fceux' + exe_suffix
  fceux_dst = 'bin/fceux' + exe_suffix
  
  fceux_net_server_src = 'fceux-net-server' + exe_suffix
  fceux_net_server_dst = 'bin/fceux-net-server' + exe_suffix
  
  auxlib_src = 'src/auxlib.lua'
  auxlib_dst = 'bin/auxlib.lua'
  auxlib_inst_dst = prefix + '/share/fceux/auxlib.lua'
  
  fceux_h_src = 'output/fceux.chm'
  fceux_h_dst = 'bin/fceux.chm'
  
  env.Command(fceux_h_dst, fceux_h_src, [Copy(fceux_h_dst, fceux_h_src)])
  env.Command(fceux_dst, fceux_src, [Copy(fceux_dst, fceux_src)])
  env.Command(fceux_net_server_dst, fceux_net_server_src, [Copy(fceux_net_server_dst, fceux_net_server_src)])
  env.Command(auxlib_dst, auxlib_src, [Copy(auxlib_dst, auxlib_src)])
  
  man_src = 'documentation/fceux.6'
  man_net_src = 'documentation/fceux-net-server.6'
  man_dst = prefix + '/share/man/man6/fceux.6'
  man_net_dst = prefix + '/share/man/man6/fceux-net-server.6'
  
  share_src = 'output/'
  share_dst = prefix + '/share/fceux/'
  
  image_src = 'fceux.png'
  image_dst = prefix + '/share/pixmaps'
  
  desktop_src = 'fceux.desktop'
  desktop_dst = prefix + '/share/applications/'
  
  env.Install(prefix + "/bin/", fceux)
  env.Install(prefix + "/bin/", "fceux-net-server")
  # TODO:  Where to put auxlib on "scons install?"
  env.Alias('install', env.Command(auxlib_inst_dst, auxlib_src, [Copy(auxlib_inst_dst, auxlib_src)]))
  env.Alias('install', env.Command(share_dst, share_src, [Copy(share_dst, share_src)]))
  env.Alias('install', env.Command(man_dst, man_src, [Copy(man_dst, man_src)]))
  env.Alias('install', env.Command(man_net_dst, man_net_src, [Copy(man_net_dst, man_net_src)]))
  env.Alias('install', env.Command(image_dst, image_src, [Copy(image_dst, image_src)]))
  env.Alias('install', env.Command(desktop_dst, desktop_src, [Copy(desktop_dst, desktop_src)]))
  env.Alias('install', (prefix + "/bin/"))

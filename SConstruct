# vim: filetype=python

## first we extend the module path to load our own modules
import subprocess
import sys, os
sys.path = ["tools/python"] + sys.path

import system
import glob
from build_tools import *
from nozon_tools import *

import SCons

from colorama import init
init()
from colorama import Fore, Back, Style


################################################################################
#   Operating System detection
################################################################################

if system.os() == 'darwin':
    ALLOWED_COMPILERS = ('gcc',)   # Do not remove this comma, it's magic
    arnold_default_api_lib = os.path.join('$ARNOLD', 'bin')
    glew_default_lib = os.path.join('$EXTERNAL_PATH', 'glew-1.10.0', 'lib', 'libGLEW.a')
    glew_default_include = os.path.join('$EXTERNAL_PATH', 'glew-1.10.0', 'include')
elif system.os() == 'linux':
    ALLOWED_COMPILERS = ('gcc',)   # Do not remove this comma, it's magic
    # linux conventions would be to actually use lib for dynamic libraries!
    arnold_default_api_lib = os.path.join('$ARNOLD', 'bin')
    glew_default_lib = '/usr/lib64/libGLEW.a'
    glew_default_include = '/usr/include'
elif system.os() == 'windows':
    ALLOWED_COMPILERS = ('msvc', 'icc')
    arnold_default_api_lib = os.path.join('$ARNOLD', 'lib')
    glew_default_lib = os.path.join('$EXTERNAL_PATH', 'glew-1.10.0', 'lib', 'glew32s.lib')
    glew_default_include = os.path.join('$EXTERNAL_PATH', 'glew-1.10.0', 'include')
else:
    print "Unknown operating system: %s" % system.os()
    Exit(1)

################################################################################
#   Build system options
################################################################################

vars = Variables('custom.py')
vars.AddVariables(
    ## basic options
    EnumVariable('MODE'       , 'Set compiler configuration', 'debug'             , allowed_values=('opt', 'debug', 'profile')),
    EnumVariable('WARN_LEVEL' , 'Set warning level'         , 'strict'            , allowed_values=('strict', 'warn-only', 'none')),
    EnumVariable('COMPILER'   , 'Set compiler to use'       , ALLOWED_COMPILERS[0], allowed_values=ALLOWED_COMPILERS),
    ('COMPILER_VERSION'       , 'Version of compiler to use', ''),
    BoolVariable('MULTIPROCESS','Enable multiprocessing in the testsuite', True),
    BoolVariable('SHOW_CMDS'  , 'Display the actual command lines used for building', False),
    PathVariable('LINK', 'Linker to use', None),
    PathVariable('SHCC', 'Path to C++ (gcc) compiler used', None),
    PathVariable('SHCXX', 'Path to C++ (gcc) compiler used for generating shared-library objects', None),
    ('FTP'            , 'Path of the FTP to upload the package'        , ''),
    ('FTP_SUBDIR'     , 'Subdirectory on the FTP to place the package' , ''),
    ('FTP_USER'       , 'Username for the FTP'                         , ''),
    ('FTP_PASS'       , 'Password for the FTP'                         , ''),
    ('PACKAGE_SUFFIX' , 'Suffix for the package names'                 , ''),
                  
    BoolVariable('COLOR_CMDS' , 'Display colored output messages when building', True),
    EnumVariable('SHOW_TEST_OUTPUT', 'Display the test log as it is being run', 'single', allowed_values=('always', 'never', 'single')),
    BoolVariable('UPDATE_REFERENCE', 'Update the reference log/image for the specified targets', False),
    ('TEST_THREADS' , 'Number of simultaneous tests to run', 4),
    ('TEST_PATTERN' , 'Glob pattern of tests to be run', 'test_*'),
    ('GCC_OPT_FLAGS', 'Optimization flags for gcc', '-O3 -funroll-loops'),
    BoolVariable('DISABLE_COMMON', 'Disable shaders found in the common repository', False),

    PathVariable('MAYA_ROOT',
                 'Directory where Maya is installed (defaults to $MAYA_LOCATION)', 
                 get_default_path('MAYA_LOCATION', '.')),
    PathVariable('MAYA_ROOT',
                 'Directory where Maya is installed (defaults to $MAYA_LOCATION)', 
                 get_default_path('MAYA_LOCATION', '.')),
    PathVariable('MTOA_ROOT', 
                 'Directory where MtoA is installed', 
                 get_default_path('MTOA_HOME', '.')), 
    PathVariable('MTOA_API_INCLUDES', 
                 'Where to find MtoA API includes', 
                 os.path.join('$MTOA_ROOT', 'include'), PathVariable.PathIsDir),                    
    PathVariable('MTOA_API_LIB', 
                 'Where to find MtoA API static libraries', 
                 os.path.join('$MTOA_ROOT', 'lib'), PathVariable.PathIsDir), 
    PathVariable('ILMBASE_INCLUDE_PATH',
                 'ILMBase include path',
                 '.'),    
    PathVariable('JSON_INCLUDE_PATH',
                 'Json include path',
                 '.'), 
    PathVariable('JSON_LIB',
                 'Json lib path',
                 '.'),                     
    PathVariable('PYSTRING_INCLUDE_PATH',
                 'Pystring include path',
                 '.'), 
    PathVariable('PYSTRING_LIB',
                 'Pystring lib path',
                 '.'),
    PathVariable('ILMBASE_LIBS',
                 'ILMBase static libs path',
                 '.'),        
    PathVariable('ILMBASE_BIN',
                 'ILMBase bin path',
                 '.'),          
    PathVariable('BOOST_INCLUDE_PATH',
                 'Boost include path',
                 '.'), 
    PathVariable('BOOST_LIBS',
                 'Boost static lib path',
                 '.'), 
    PathVariable('MAYA_INCLUDE_PATH',
                 'Directory where Maya SDK headers are installed',
                 '.'),
    PathVariable('ALEMBIC_INCLUDE_PATH', 
                 'Where to find Alembic includes', 
                 get_default_path('ALEMBIC_INCLUDE_PATH', '.')),
    PathVariable('ALEMBIC_LIBS', 
                 'Where to find Alembic static libraries', 
                 '.', PathVariable.PathIsDir),
    PathVariable('ARNOLD', 
                 'Where to find Arnold installation', 
                 get_default_path('ARNOLD_HOME', 'Arnold')),                   
    PathVariable('ARNOLD_API_INCLUDES', 
                 'Where to find Arnold API includes', 
                 os.path.join('$ARNOLD', 'include'), PathVariable.PathIsDir),
    PathVariable('ARNOLD_API_LIB', 
                 'Where to find Arnold API static libraries', 
                 arnold_default_api_lib, PathVariable.PathIsDir),
    PathVariable('ARNOLD_BINARIES', 
                 'Where to find Arnold API dynamic libraries and executables', 
                 os.path.join('$ARNOLD', 'bin'), PathVariable.PathIsDir),
    PathVariable('GLEW_INCLUDES', 
                 'Where to find GLEW includes', 
                 glew_default_include, PathVariable.PathIsDir),
    PathVariable('GLEW_LIB', 
                 'Where to find GLEW static library', 
                 glew_default_lib, PathVariable.PathIsFile),
    PathVariable('TARGET_MODULE_PATH', 
                 'Path used for installation of the mtoa module', 
                 '.', PathVariable.PathIsDirCreate),
    PathVariable('TARGET_PLUGIN_PATH', 
                 'Path used for installation of the mtoa plugin', 
                 os.path.join('$TARGET_MODULE_PATH', 'plug-ins'), PathVariable.PathIsDirCreate),
    PathVariable('TARGET_SCRIPTS_PATH', 
                 'Path used for installation of scripts', 
                 os.path.join('$TARGET_MODULE_PATH', 'scripts'), PathVariable.PathIsDirCreate),
    PathVariable('TARGET_PYTHON_PATH', 
                 'Path used for installation of Python scripts', 
                 os.path.join('$TARGET_MODULE_PATH', 'scripts'), PathVariable.PathIsDirCreate),            
    PathVariable('TARGET_ICONS_PATH', 
                 'Path used for installation of icons', 
                 os.path.join('$TARGET_MODULE_PATH', 'icons'), PathVariable.PathIsDirCreate),
    PathVariable('TARGET_SHADER_PATH', 
                 'Path used for installation of arnold shaders', 
                 os.path.join('$TARGET_MODULE_PATH', 'shaders'), PathVariable.PathIsDirCreate),
    PathVariable('TARGET_PROCEDURAL_PATH', 
                 'Path used for installation of arnold procedurals', 
                 os.path.join('$TARGET_MODULE_PATH', 'procedurals'), PathVariable.PathIsDirCreate),
    PathVariable('TARGET_EXTENSION_PATH', 
                 'Path used for installation of mtoa translator extensions', 
                 os.path.join('$TARGET_MODULE_PATH', 'extensions'), PathVariable.PathIsDirCreate),
    PathVariable('TARGET_DOC_PATH', 
                 'Path for documentation', 
                 os.path.join('$TARGET_MODULE_PATH', 'docs','api'), PathVariable.PathIsDirCreate),                  
    PathVariable('TARGET_BINARIES', 
                 'Path for libraries', 
                 os.path.join('$TARGET_MODULE_PATH', 'bin'), PathVariable.PathIsDirCreate),
    PathVariable('TOOLS_PATH',
                 'Where to find external tools required for sh',
                 '.', PathVariable.PathIsDir),
)

if system.os() == 'darwin':
    vars.Add(EnumVariable('SDK_VERSION', 'Version of the Mac OSX SDK to use', '10.7', allowed_values=('10.6', '10.7', '10.8', '10.9')))
    vars.Add(PathVariable('SDK_PATH', 'Root path to installed OSX SDKs', '/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs'))

if system.os() == 'windows':    
    # Ugly hack. Create a temporary environment, without loading any tool, so we can set the MSVC_ARCH
    # variable from the contents of the TARGET_ARCH variable. Then we can load tools.
    tmp_env = Environment(variables = vars, tools=[])
    tmp_env.Append(MSVC_ARCH = 'amd64')
    env = tmp_env.Clone(tools=['default'])
    # restore as the Clone overrides it
    env['TARGET_ARCH'] = 'x86_64'
else:
    env = Environment(variables = vars)

if env['TARGET_MODULE_PATH'] == '.':
    print "Please define TARGET_MODULE_PATH (Path used for installation of the mtoa plugin)"
    Exit(1)

env.Append(BUILDERS = {'MakeModule' : make_module})

env.AppendENVPath('PATH', env.subst(env['TOOLS_PATH']))

system.set_target_arch('x86_64')

# Configure colored output
color_green   = ''
color_red     = ''
color_bright  = ''
color_bgreen  = ''
color_bred    = ''
color_reset   = ''
if env['COLOR_CMDS']:
    color_green   = Fore.GREEN
    color_red     = Fore.RED
    color_bright  = Style.BRIGHT
    color_bgreen  = color_green + color_bright
    color_bred    = color_red   + color_bright
    color_reset   = Fore.RESET + Back.RESET + Style.RESET_ALL

#define shortcuts for the above paths, with substitution of environment variables
MAYA_ROOT = env.subst(env['MAYA_ROOT'])
MAYA_INCLUDE_PATH = env.subst(env['MAYA_INCLUDE_PATH'])

if env['MAYA_INCLUDE_PATH'] == '.':
	if system.os() == 'darwin':
	    MAYA_INCLUDE_PATH = os.path.join(MAYA_ROOT, '../../devkit/include')
	else:
	    MAYA_INCLUDE_PATH = os.path.join(MAYA_ROOT, 'include')

ARNOLD = env.subst(env['ARNOLD'])
ARNOLD_API_INCLUDES = env.subst(env['ARNOLD_API_INCLUDES'])
ARNOLD_API_LIB = env.subst(env['ARNOLD_API_LIB'])
ARNOLD_BINARIES = env.subst(env['ARNOLD_BINARIES'])
ALEMBIC_INCLUDE_PATH = env.subst(env['ALEMBIC_INCLUDE_PATH'])
BOOST_INCLUDE_PATH = env.subst(env['BOOST_INCLUDE_PATH'])
BOOST_LIBS = env.subst(env['BOOST_LIBS'])
JSON_INCLUDE_PATH = env.subst(env['JSON_INCLUDE_PATH'])
JSON_LIB = env.subst(env['JSON_LIB'])
PYSTRING_INCLUDE_PATH = env.subst(env['PYSTRING_INCLUDE_PATH'])
PYSTRING_LIB = env.subst(env['PYSTRING_LIB'])
ILMBASE_INCLUDE_PATH = env.subst(env['ILMBASE_INCLUDE_PATH'])
ILMBASE_BIN = env.subst(env['ILMBASE_BIN'])
ILMBASE_LIBS = env.subst(env['ILMBASE_LIBS'])
ALEMBIC_LIBS = env.subst(env['ALEMBIC_LIBS'])
TARGET_MODULE_PATH = env.subst(env['TARGET_MODULE_PATH']) 
TARGET_PLUGIN_PATH = env.subst(env['TARGET_PLUGIN_PATH'])  
TARGET_SCRIPTS_PATH = env.subst(env['TARGET_SCRIPTS_PATH']) 
TARGET_PYTHON_PATH = env.subst(env['TARGET_PYTHON_PATH']) 
TARGET_ICONS_PATH = env.subst(env['TARGET_ICONS_PATH'])  
TARGET_SHADER_PATH = env.subst(env['TARGET_SHADER_PATH']) 
TARGET_PROCEDURAL_PATH = env.subst(env['TARGET_PROCEDURAL_PATH'])
TARGET_EXTENSION_PATH = env.subst(env['TARGET_EXTENSION_PATH']) 
TARGET_DOC_PATH = env.subst(env['TARGET_DOC_PATH'])  
TARGET_BINARIES = env.subst(env['TARGET_BINARIES']) 
PACKAGE_SUFFIX = env.subst(env['PACKAGE_SUFFIX'])

# Get arnold and maya versions used for this build
arnold_version    = get_arnold_version(os.path.join(ARNOLD_API_INCLUDES, 'ai_version.h'))
maya_version      = get_maya_version(os.path.join(MAYA_INCLUDE_PATH, 'maya', 'MTypes.h'))
maya_version_base = maya_version[0:4]
if int(maya_version) >= 201450:
    env['ENABLE_XGEN'] = 1
if int(maya_version_base) >= 2014:
    env['ENABLE_VP2'] = 1

if int(maya_version_base) == 2012:
    env['MSVC_VERSION'] = '9.0'
elif (int(maya_version_base) == 2013) or (int(maya_version_base) == 2014):
    env['MSVC_VERSION'] = '10.0'
elif int(maya_version_base) >= 2015:
    env['MSVC_VERSION'] = '11.0'


if system.os() == 'linux':
    try:
        p = subprocess.Popen([env['COMPILER'] + env['COMPILER_VERSION'], '-dumpversion'], stdout=subprocess.PIPE)
        compiler_version, err = p.communicate()
        print 'Compiler       : %s' % (env['COMPILER'] + compiler_version[:-1])
    except:
        pass
elif system.os() == 'windows':
    print 'MSVC version   : %s' % (env['MSVC_VERSION'])
print 'SCons          : %s' % (SCons.__version__)
print ''

try:
   import json
except ImportError:
   import simplejson as json

try:
    json_data = open('dependencies.json')
    data = json.load(json_data)
    if data['arnold'] != arnold_version:
        print '''
        You are building with arnold %s instead
        of the officially supported version %s. 
        You might encounter bugs, build errors 
        or undefined behavior.
        ''' % (arnold_version, data['arnold'])
except:
    pass

################################
## COMPILER OPTIONS
################################

## Generic Windows stuff476
if system.os() == 'windows':
    # Embed manifest in executables and dynamic libraries
    env['LINKCOM'] = [env['LINKCOM'], 'mt.exe -nologo -manifest ${TARGET}.manifest -outputresource:$TARGET;1']
    env['SHLINKCOM'] = [env['SHLINKCOM'], 'mt.exe -nologo -manifest ${TARGET}.manifest -outputresource:$TARGET;2']

export_symbols = env['MODE'] in ['debug', 'profile']

if env['COMPILER'] == 'gcc':
    compiler_version = env['COMPILER_VERSION']
    if compiler_version != '':
        env['CC']  = 'gcc' + compiler_version
        env['CXX'] = 'g++' + compiler_version
    # env.Append(CXXFLAGS = Split('-fno-rtti'))

    if env['MODE'] == 'opt': 
        env.Append(CPPDEFINES = Split('NDEBUG')) 

    ## Hide all internal symbols (the ones without AI_API decoration)
    env.Append(CCFLAGS = Split('-fvisibility=hidden'))
    env.Append(CXXFLAGS = Split('-fvisibility=hidden'))
    env.Append(LINKFLAGS = Split('-fvisibility=hidden'))

    ## Hardcode '.' directory in RPATH in linux
    if system.os() == 'linux':
        env.Append(LINKFLAGS = Split('-z origin') )
        env.Append(RPATH = env.Literal(os.path.join('\\$$ORIGIN', '..', 'bin')))

    ## warning level
    if env['WARN_LEVEL'] == 'none':
        env.Append(CCFLAGS = Split('-w'))
    else:
        env.Append(CCFLAGS = Split('-Wall -Wsign-compare'))
        if env['WARN_LEVEL'] == 'strict':
            env.Append(CCFLAGS = Split('-Werror'))

    ## optimization flags
    if env['MODE'] == 'opt' or env['MODE'] == 'profile':
        env.Append(CCFLAGS = Split(env['GCC_OPT_FLAGS']))
    if env['MODE'] == 'debug' or env['MODE'] == 'profile':
        if system.os() == 'darwin': 
            env.Append(CCFLAGS = Split('-gstabs')) 
            env.Append(LINKFLAGS = Split('-gstabs')) 
        else: 
            env.Append(CCFLAGS = Split('-g -fno-omit-frame-pointer')) 
            env.Append(LINKFLAGS = Split('-g')) 
    if system.os() == 'linux' and env['MODE'] == 'profile':
        env.Append(CCFLAGS = Split('-pg'))
        env.Append(LINKFLAGS = Split('-pg'))

    if system.os() == 'darwin':
        ## tell gcc to compile a 64 bit binary
        env.Append(CCFLAGS = Split('-arch x86_64'))
        env.Append(LINKFLAGS = Split('-arch x86_64'))
        env.Append(CCFLAGS = env.Split('-mmacosx-version-min=10.7'))
        env.Append(LINKFLAGS = env.Split('-mmacosx-version-min=10.7'))
        env.Append(CCFLAGS = env.Split('-isysroot %s/MacOSX%s.sdk/' % (env['SDK_PATH'], env['SDK_VERSION'])))
        env.Append(LINKFLAGS = env.Split('-isysroot %s/MacOSX%s.sdk/' % (env['SDK_PATH'], env['SDK_VERSION'])))

elif env['COMPILER'] == 'msvc':
    MSVC_FLAGS  = " /W3"         # Warning level : 3
    MSVC_FLAGS += " /wd 4005"
    MSVC_FLAGS += " /EHsc"       # enable synchronous C++ exception handling model & 
                                # assume extern "C" functions do not throw exceptions
    MSVC_FLAGS += " /Gd"         # makes __cdecl the default calling convention 
    MSVC_FLAGS += " /fp:precise" # precise floating point model: results are predictable

    if env['WARN_LEVEL'] == 'strict':
        MSVC_FLAGS += " /WX"  # treats warnings as errors

    if export_symbols:
        MSVC_FLAGS += " /Zi"  # generates complete debug information

    LINK_FLAGS  = " /MANIFEST"

    if env['MODE'] in ['opt', 'profile']:
        MSVC_FLAGS += " /Ob2"    # enables inlining of ANY function (compiler discretion)
        MSVC_FLAGS += " /GL"     # enables whole program optimization
        MSVC_FLAGS += " /MD"     # uses multithreaded DLL runtime library
        MSVC_FLAGS += " /Ox"     # selects maximum optimization
      
        LINK_FLAGS += " /LTCG"   # enables link time code generation (needed by /GL)
    else:  ## Debug mode
        MSVC_FLAGS += " /Od"   # disables all optimizations
        MSVC_FLAGS += " /MDd"   # uses *NON-DEBUG* multithreaded DLL runtime library

        LINK_FLAGS += " /DEBUG"

    env.Append(CCFLAGS = Split(MSVC_FLAGS))
    env.Append(LINKFLAGS = Split(LINK_FLAGS))
   
    if env['MODE'] == 'opt':
        env.Append(CPPDEFINES = Split('NDEBUG'))
    # We cannot enable this define, as it will try to use symbols from the debug runtime library  
    # Re-acivated to allow memory tracking in MSVC
    if env['MODE'] == 'debug':
        env.Append(CPPDEFINES = Split('_DEBUG'))
        # for MSVC memory tracking 
        env.Append(CPPDEFINES = Split('_CRTDBG_MAP_ALLOC'))

    env.Append(CPPDEFINES = Split('_CRT_SECURE_NO_WARNINGS'))
elif env['COMPILER'] == 'icc':
    env.Tool('intelc', abi = 'intel64')

    ICC_FLAGS  = " /W3"            # displays remarks, warnings, and errors
    ICC_FLAGS += " /Qstd:c99"      # conforms to The ISO/IEC 9899:1999 International Standard
    ICC_FLAGS += " /EHsc"          # enable synchronous C++ exception handling model & 
                                  # assume extern "C" functions do not throw exceptions
    ICC_FLAGS += " /GS"            # generates code that detects some buffer overruns 
    ICC_FLAGS += " /Qprec"         # improves floating-point precision and consistency 
    ICC_FLAGS += " /Qvec-report0"  # disables diagnostic information reported by the vectorizer 

    if env['WARN_LEVEL'] == 'strict':
        ICC_FLAGS += " /WX"  # treats warnings as errors

    # Disables the following warnings:
    #
    #  424 : 
    #  537 : 
    #  991 : 
    # 1478 : 
    # 1572 : 
    # 1786 : 
    ICC_FLAGS += " /Qdiag-disable:424,537,991,1478,1572,1786" 

    XILINK_FLAGS  = " /LARGEADDRESSAWARE"

    if export_symbols:
        ICC_FLAGS    += " /debug:full"  # generates complete debug information
        ICC_FLAGS    += " /Zi"          # 
        XILINK_FLAGS += " /DEBUG"

    if env['MODE'] in ['opt', 'profile']:
        ICC_FLAGS += " /Ob2"    # enables inlining of ANY function (compiler discretion)
        ICC_FLAGS += " -Qipo"   # enables interprocedural optimization between files
        ICC_FLAGS += " /G7"     # optimize for latest Intel processors (deprecated)
        ICC_FLAGS += " /QaxW"   # optimize for Intel processors with SSE2 (deprecated)
        ICC_FLAGS += " /MD"     # uses multithreaded DLL runtime library
        ICC_FLAGS += " -O3"     # max optimization level

        if env['MODE'] == 'profile':
            ICC_FLAGS += " /FR"     # Enable browse information

            XILINK_FLAGS += " /FIXED:NO"

        XILINK_FLAGS += " /INCREMENTAL:NO"
    else:
        ICC_FLAGS += " /Od"   # disables all optimizations
        ICC_FLAGS += " /MDd"   # uses *NON-DEBUG* multithreaded DLL runtime library

        XILINK_FLAGS += " /INCREMENTAL"

    env.Append(CCFLAGS = Split(ICC_FLAGS))
    env.Append(LINKFLAGS = Split(XILINK_FLAGS))
   
    if env['MODE'] == 'opt':
        env.Append(CPPDEFINES = Split('NDEBUG'))
# We cannot enable this define, as it will try to use symbols from the debug runtime library  
#   if env['MODE'] == 'debug':
#      env.Append(CPPDEFINES = Split('_DEBUG'))
            
if env['MODE'] == 'debug':
    env.Append(CPPDEFINES = Split('ARNOLD_DEBUG'))

## platform related defines
if system.os() == 'windows':
    env.Append(CPPDEFINES = Split('_WINDOWS _WIN32 WIN32'))
    env.Append(CPPDEFINES = Split('_WIN64'))
elif system.os() == 'darwin':
    env.Append(CPPDEFINES = Split('_DARWIN OSMac_'))
elif system.os() == 'linux':
    env.Append(CPPDEFINES = Split('_LINUX'))

## Add path to Arnold API by default
env.Append(CPPPATH = [ARNOLD_API_INCLUDES,])
env.Append(LIBPATH = [ARNOLD_API_LIB, ARNOLD_BINARIES])
   
## configure base directory for temp files
BUILD_BASE_DIR = os.path.join('build', '%s_%s' % (system.os(), system.target_arch()), maya_version, '%s_%s' % (env['COMPILER'], env['MODE']))
env['BUILD_BASE_DIR'] = BUILD_BASE_DIR

if not env['SHOW_CMDS']:
    ## hide long compile lines from the user
    env['CCCOMSTR']     = color_bgreen + 'Compiling $SOURCE ...' + color_reset
    env['SHCCCOMSTR']   = color_bgreen + 'Compiling $SOURCE ...' + color_reset
    env['CXXCOMSTR']    = color_bgreen + 'Compiling $SOURCE ...' + color_reset
    env['SHCXXCOMSTR']  = color_bgreen + 'Compiling $SOURCE ...' + color_reset
    env['LINKCOMSTR']   = color_bred   + 'Linking $TARGET ...'   + color_reset
    env['SHLINKCOMSTR'] = color_bred   + 'Linking $TARGET ...'   + color_reset

################################
## BUILD TARGETS
################################

env['BUILDERS']['MakePackage'] = Builder(action = Action(make_package, "Preparing release package: '$TARGET'"))
env['ROOT_DIR'] = os.getcwd()

if system.os() == 'windows':
    #First the maya plugins. They all need maya & alembic libs.
    maya_env = env.Clone()
    maya_env.Append(CPPPATH = ['.'])
    maya_env.Append(CPPPATH = [MAYA_INCLUDE_PATH, ALEMBIC_INCLUDE_PATH, BOOST_INCLUDE_PATH, ILMBASE_INCLUDE_PATH])
    maya_env.Append(CPPDEFINES = Split('NT_PLUGIN REQUIRE_IOSTREAM'))
    maya_env.Append(LIBPATH = [os.path.join(MAYA_ROOT, 'lib'), ALEMBIC_LIBS, ILMBASE_LIBS])
    maya_env.Append(LIBS=Split('OpenGl32.lib Foundation.lib OpenMaya.lib OpenMayaRender.lib OpenMayaUI.lib OpenMayaAnim.lib OpenMayaFX.lib AlembicAbcGeom.lib AlembicAbcMaterial.lib AlembicAbcCoreOgawa.lib AlembicOgawa.lib AlembicAbc.lib AlembicAbcCoreHDF5.lib AlembicAbcCoreAbstract.lib AlembicAbcCoreFactory.lib AlembicAbcCollection.lib AlembicUtil.lib hdf5_hl.lib hdf5.lib libIex.lib libIlmImf.lib libHalf.lib zlibwapi.lib'))

    MAYA_PLUGINS = env.SConscript(os.path.join('maya', 'SConscript'),
                                            variant_dir = os.path.join(BUILD_BASE_DIR, 'plugins'),
                                            duplicate = 0,
                                            exports   = 'maya_env')


    # The translator. They only need MtoA & Arnold libs.
    mtoa_env = env.Clone()
    mtoa_env.Append(CPPPATH = ['.'])
    mtoa_env.Append(CPPPATH = [MAYA_INCLUDE_PATH, ARNOLD_API_INCLUDES, env["MTOA_API_INCLUDES"]])
    mtoa_env.Append(CPPDEFINES = Split('NT_PLUGIN REQUIRE_IOSTREAM'))
    mtoa_env.Append(LIBPATH = [os.path.join(MAYA_ROOT, 'lib'), ARNOLD_API_LIB, env["MTOA_API_LIB"]])
    mtoa_env.Append(LIBS=Split('Foundation.lib OpenMaya.lib OpenMayaRender.lib OpenMayaAnim.lib mtoa_api.lib ai.lib'))
   

    MTOA_TRANSLATORS = env.SConscript(os.path.join('mtoa', 'SConscript'),
                                                variant_dir = os.path.join(BUILD_BASE_DIR, 'extensions'),
                                                duplicate   = 0,
                                                exports     = 'mtoa_env')

    # The arnold procedural & plugin. They all need alembic.
    arnold_env = env.Clone()
    arnold_env.Append(CPPPATH = ['.'])
    arnold_env.Append(CPPPATH = [ALEMBIC_INCLUDE_PATH, BOOST_INCLUDE_PATH, ILMBASE_INCLUDE_PATH, ARNOLD_API_LIB])
    arnold_env.Append(CPPDEFINES = Split('NT_PLUGIN REQUIRE_IOSTREAM'))
    arnold_env.Append(LIBPATH = [ALEMBIC_LIBS, ILMBASE_LIBS, ARNOLD_API_LIB])
    arnold_env.Append(LIBS=Split('AlembicAbcGeom.lib AlembicAbcMaterial.lib AlembicAbcCoreOgawa.lib AlembicOgawa.lib AlembicAbc.lib AlembicAbcCoreHDF5.lib AlembicAbcCoreAbstract.lib AlembicAbcCoreFactory.lib AlembicAbcCollection.lib AlembicUtil.lib hdf5_hl.lib hdf5.lib libIex.lib libIlmImf.lib libHalf.lib zlibwapi.lib ai.lib'))

    ARNOLD_SHADERS = env.SConscript(os.path.join('arnold', 'shaders', 'SConscript'),
                                                variant_dir = os.path.join(BUILD_BASE_DIR, 'shaders', 'arnold'),
                                                duplicate   = 0,
                                                exports     = 'arnold_env')

    ARNOLD_PROCS = env.SConscript(os.path.join('arnold', 'procedurals', 'SConscript'),
                                                variant_dir = os.path.join(BUILD_BASE_DIR, 'procedurals', 'arnold'),
                                                duplicate   = 0,
                                                exports     = 'arnold_env')


else:
    maya_env = env.Clone()
    maya_env.Append(CPPPATH = ['.'])
    maya_env.Append(CPPDEFINES = Split('_BOOL REQUIRE_IOSTREAM'))

    if system.os() == 'linux':
        maya_env.Append(CPPPATH = [MAYA_INCLUDE_PATH])
        maya_env.Append(LIBS=Split('GL'))
        maya_env.Append(CPPDEFINES = Split('LINUX'))
        maya_env.Append(LIBPATH = [os.path.join(MAYA_ROOT, 'lib')])
    
    elif system.os() == 'darwin':
        # MAYA_LOCATION on osx includes Maya.app/Contents
        maya_env.Append(CPPPATH = [MAYA_INCLUDE_PATH])
        maya_env.Append(LIBPATH = [os.path.join(MAYA_ROOT, 'MacOS')])

    maya_env.Append(LIBS=Split('ai pthread Foundation OpenMaya OpenMayaRender OpenMayaUI OpenMayaAnim OpenMayaFX'))

    MTOA_API = env.SConscript(os.path.join('plugins', 'mtoa', 'SConscriptAPI'),
                              variant_dir = os.path.join(BUILD_BASE_DIR, 'api'),
                              duplicate   = 0,
                              exports     = 'maya_env')

    MTOA = env.SConscript(os.path.join('plugins', 'mtoa', 'SConscript'),
                          variant_dir = os.path.join(BUILD_BASE_DIR, 'mtoa'),
                          duplicate   = 0,
                          exports     = 'maya_env')

    MTOA_SHADERS = env.SConscript(os.path.join('shaders', 'src', 'SConscript'),
                                  variant_dir = os.path.join(BUILD_BASE_DIR, 'shaders'),
                                  duplicate   = 0,
                                  exports     = 'env')

    MTOA_PROCS = env.SConscript(os.path.join('procedurals', 'SConscript'),
                                variant_dir = os.path.join(BUILD_BASE_DIR, 'procedurals'),
                                duplicate   = 0,
                                exports     = 'env')
                                 
    def osx_hardcode_path(target, source, env):
        cmd = ""

        if target[0] == MTOA_API[0]:
            cmd = "install_name_tool -id @loader_path/../bin/libmtoa_api.dylib"
        elif target[0] == MTOA[0]:
            cmd = " install_name_tool -add_rpath @loader_path/../bin/"
        else:
	          cmd = "install_name_tool -id " + str(target[0]).split('/')[-1]
         
        if cmd :
            p = subprocess.Popen(cmd + " " + str(target[0]), shell=True)
            retcode = p.wait()

        return 0

    if system.os() == 'darwin':
        env.AddPostAction(MTOA_API[0],  Action(osx_hardcode_path, 'Adjusting paths in mtoa_api.dylib ...'))
        env.AddPostAction(MTOA, Action(osx_hardcode_path, 'Adjusting paths in mtoa.boundle ...'))
        env.AddPostAction(MTOA_SHADERS, Action(osx_hardcode_path, 'Adjusting paths in mtoa_shaders ...'))
        env.AddPostAction(MTOA_PROCS, Action(osx_hardcode_path, 'Adjusting paths in mtoa_procs ...'))

if system.os() == 'windows':
    # Rename plugins as .mll and install them in the target path
    for path in MAYA_PLUGINS:
        if str(path).endswith(".dll"):

            new = os.path.splitext(str(path))[0] + '.mll'
            env.Command(new, str(path), Copy("$TARGET", "$SOURCE"))
            env.Install(TARGET_PLUGIN_PATH, [new])
            
    nprocs = []
    for proc in ARNOLD_PROCS:
        if str(proc).endswith(".dll"):
            nprocs.append(proc)
    PROCS = nprocs
    env.Install(env['TARGET_PROCEDURAL_PATH'], PROCS)

    nshaders = []
    for shader in ARNOLD_SHADERS:
        if str(shader).endswith(".dll"):
            nshaders.append(shader)
    SHADERS = nshaders
    env.Install(env['TARGET_SHADER_PATH'], SHADERS)

    nexts = []
    for extension in MTOA_TRANSLATORS:
        if str(extension).endswith(".dll"):
            nexts.append(extension)
    EXTS = nexts
    env.Install(env['TARGET_EXTENSION_PATH'], EXTS)

# else:
#     env.Install(TARGET_PLUGIN_PATH, MTOA)
#     env.Install(TARGET_SHADER_PATH, MTOA_SHADERS)
#     env.Install(env['TARGET_PROCEDURAL_PATH'], MTOA_PROCS)
#     if system.os() == 'linux':
#         libs = glob.glob(os.path.join(ARNOLD_API_LIB, '*.so'))
#     else:
#         libs = glob.glob(os.path.join(ARNOLD_API_LIB, '*.dylib'))

# env.Install(env['TARGET_LIB_PATH'], libs)

dylibs = glob.glob(os.path.join(ILMBASE_BIN, '*%s' % get_library_extension()))
env.Install(env['TARGET_BINARIES'], dylibs)

# install scripts
scriptfiles = find_files_recursive(os.path.join('maya', 'scripts'), ['.py', '.mel', '.ui'])
env.InstallAs([os.path.join(TARGET_PYTHON_PATH, x) for x in scriptfiles],
            [os.path.join('maya', 'scripts', x) for x in scriptfiles])

# install icons
env.Install(TARGET_ICONS_PATH, glob.glob(os.path.join('icons', '*.xpm')))
env.Install(TARGET_ICONS_PATH, glob.glob(os.path.join('icons', '*.png')))

# # install docs
# env.Install(TARGET_DOC_PATH, glob.glob(os.path.join(BUILD_BASE_DIR, 'docs', 'api', 'html', '*.*')))
# env.Install(TARGET_MODULE_PATH, glob.glob(os.path.join('docs', 'readme.txt')))

env.MakeModule(TARGET_MODULE_PATH, os.path.join(BUILD_BASE_DIR, 'alembicHolder.mod'))
env.Install(TARGET_MODULE_PATH, os.path.join(BUILD_BASE_DIR, 'alembicHolder.mod'))

maya_base_version = maya_version[:4]

if maya_base_version == '2013':
    if int(maya_version[-2:]) >= 50:
        maya_base_version = '20135'

## Sets release package name based on MtoA version, architecture and compiler used.
##
package_name = "AlembicHolder-" + "-" + system.os() + "-" + maya_base_version + PACKAGE_SUFFIX

if env['MODE'] in ['debug', 'profile']:
    package_name += '-' + env['MODE']

package_name_inst = package_name

PACKAGE = env.MakePackage(package_name, MAYA_PLUGINS + MTOA_TRANSLATORS + ARNOLD_SHADERS + ARNOLD_PROCS)
#PACKAGE = env.MakePackage(package_name, MTOA + MTOA_API + MTOA_SHADERS)

import ftplib

def deploy(target, source, env):

    def ftp_send_binary_cb(block):
        print "\b#",

    local_package_name = str(source[0])
    
    server = env['FTP']

    ftp = ftplib.FTP(server)

    ftp.login(env['FTP_USER'], env['FTP_PASS'])

    directory = env['FTP_SUBDIR']

    directory_split = directory.split('/')

    for d in directory_split:
        try:
            ftp.cwd(d)
        except:
            ftp.mkd(d)
            ftp.cwd(d)    

    f = open(os.path.abspath(local_package_name), 'rb')
    print 'Sending "%s" to %s/%s...' % (source[0], server, directory)
    command = "STOR %s" % local_package_name
    try:
        ftp.storbinary(command, f, 81920, ftp_send_binary_cb)
    except:
        # Old python versions have no ftp callback
        ftp.storbinary(command, f, 81920)
    print

    f.close()
    ftp.close()

env['BUILDERS']['PackageDeploy']  = Builder(action = Action(deploy,  "Deploying release package: '$SOURCE'"))


## Specifies the files that will be included in the release package.
## List items have 2 or 3 elements, with 3 possible formats:
##
## (source_file, destination_path [, new_file_name])    Copies file to destination path, optionally renaming it
## (source_dir, destination_dir)                        Recursively copies the source directory as destination_dir
## (file_spec, destination_path)                        Copies a group of files specified by a glob expression
##
PACKAGE_FILES = [
[os.path.join(BUILD_BASE_DIR, 'alembicHolder.mod'), '.'],
[os.path.join('icons', '*.xpm'), 'icons'],
[os.path.join('icons', '*.png'), 'icons'],
[os.path.join(ILMBASE_BIN, '*%s' % get_library_extension()), 'bin'],
#[MTOA_SHADERS[0], 'shaders'],
]


for p in scriptfiles:
    (d, f) = os.path.split(p)
    PACKAGE_FILES += [
        [os.path.join('scripts', 'mtoa', p), os.path.join('scripts', 'mtoa', d)]
    ]

PACKAGE_FILES.append([os.path.join(BUILD_BASE_DIR, 'alembicProcedural' 'arnold', 'AlembicArnoldProcedural%s' % get_library_extension()), 'procedurals'])
PACKAGE_FILES.append([os.path.join(BUILD_BASE_DIR, 'extensions' 'abcShader', 'AlembicArnoldProcedural%s' % get_library_extension()), 'extensions'])
PACKAGE_FILES.append([os.path.join(BUILD_BASE_DIR, 'alembicProcedural' 'arnold', 'AlembicArnoldProcedural%s' % get_library_extension()), 'extensions'])
#PACKAGE_FILES.append([os.path.join(BUILD_BASE_DIR, 'alembicProcedural', 'AlembicArnoldProcedural%s' % get_library_extension()), 'procedurals'])


# if system.os() == 'windows':
#     PACKAGE_FILES += [
#         [MTOA[0], 'plug-ins', 'alembicHolder.mll'],
#     ]
# elif system.os() == 'linux':
#     PACKAGE_FILES += [
#         [MTOA[0], 'plug-ins'],
#         [os.path.join(ARNOLD_BINARIES, '*%s.*' % get_library_extension()), 'bin'],
#     ]
# elif system.os() == 'darwin':
#     PACKAGE_FILES += [
#        [MTOA[0], 'plug-ins'],
#     ]

env['PACKAGE_FILES'] = PACKAGE_FILES

# installer_name = ''
# if system.os() == "windows":
#     installer_name = 'MtoA-%s-%s%s.exe' % (MTOA_VERSION, maya_base_version, PACKAGE_SUFFIX)
# else:
#     installer_name = 'MtoA-%s-%s-%s%s.run' % (MTOA_VERSION, system.os(), maya_base_version, PACKAGE_SUFFIX)

# def create_installer(target, source, env):
#     import tempfile
#     import shutil
#     tempdir = tempfile.mkdtemp() # creating a temporary directory for the makeself.run to work
#     shutil.copyfile(os.path.abspath('installer/MtoAEULA.txt'), os.path.join(tempdir, 'MtoAEULA.txt'))
#     if system.os() == "windows":
#         import zipfile
#         shutil.copyfile(os.path.abspath('installer/SA.ico'), os.path.join(tempdir, 'SA.ico'))
#         shutil.copyfile(os.path.abspath('installer/left.bmp'), os.path.join(tempdir, 'left.bmp'))
#         shutil.copyfile(os.path.abspath('installer/top.bmp'), os.path.join(tempdir, 'top.bmp'))
#         shutil.copyfile(os.path.abspath('installer/MtoAEULA.txt'), os.path.join(tempdir, 'MtoAEULA.txt'))
#         shutil.copyfile(os.path.abspath('installer/MtoA.nsi'), os.path.join(tempdir, 'MtoA.nsi'))
#         zipfile.ZipFile(os.path.abspath('%s.zip' % package_name), 'r').extractall(tempdir)
#         NSIS_PATH = env.subst(env['NSIS_PATH'])
#         os.environ['NSISDIR'] = NSIS_PATH
#         os.environ['NSISCONFDIR'] = NSIS_PATH
#         mtoaVersionString = MTOA_VERSION
#         mtoaVersionString = mtoaVersionString.replace('.dev', ' Dev')
#         mtoaVersionString = mtoaVersionString.replace('.RC', ' RC')
#         mayaVersionString = maya_base_version
#         mayaVersionString = mayaVersionString.replace('20135', '2013.5')
#         os.environ['MTOA_VERSION_NAME'] = mtoaVersionString
#         os.environ['MAYA_VERSION'] = mayaVersionString
#         subprocess.call([os.path.join(NSIS_PATH, 'makensis.exe'), '/V3', os.path.join(tempdir, 'MtoA.nsi')])
#         shutil.copyfile(os.path.join(tempdir, 'MtoA.exe'), installer_name)
#     else:
#         shutil.copyfile(os.path.abspath('%s.zip' % package_name), os.path.join(tempdir, "package.zip"))
#         shutil.copyfile(os.path.abspath('installer/unix_installer.py'), os.path.join(tempdir, 'unix_installer.py'))
#         commandFilePath = os.path.join(tempdir, 'unix_installer.sh')
#         commandFile = open(commandFilePath, 'w')
#         commandFile.write('python ./unix_installer.py %s' % maya_base_version)
#         commandFile.close()
#         subprocess.call(['chmod', '+x', commandFilePath])
#         installerPath = os.path.abspath('./%s' % (installer_name))
#         subprocess.call(['installer/makeself.sh', tempdir, installerPath,
#                          'MtoA for Linux Installer', './unix_installer.sh'])
#         subprocess.call(['chmod', '+x', installerPath])

# env['BUILDERS']['PackageInstaller'] = Builder(action = Action(create_installer,  "Creating installer for package: '$SOURCE'"))

# if system.os() == 'linux':
#     def check_compliance(target, source, env):
#         REFERENCE_API_LIB = env['REFERENCE_API_LIB']
#         REFERENCE_API_VERSION = env['REFERENCE_API_VERSION']
#         CURRENT_API_LIB = os.path.abspath(str(source[0]))
#         print REFERENCE_API_LIB
#         subprocess.call(['abi-dumper', REFERENCE_API_LIB, '-lver', REFERENCE_API_VERSION, '-o', 'old.dump'])
#         subprocess.call(['abi-dumper', CURRENT_API_LIB, '-lver', MTOA_VERSION, '-o', 'new.dump'])
#         subprocess.call(['abi-compliance-checker', '-l', 'libmtoa_api', '-old', 'old.dump', '-new', 'new.dump'])

#     env['BUILDERS']['ComplianceChecker'] = Builder(action = Action(check_compliance, "Checking compliance for file: '$SOURCE'"))
#     COMPLIANCECHECKER = env.ComplianceChecker('check_compliance', MTOA_API[0])
#     top_level_alias(env, 'check_compliance', COMPLIANCECHECKER)

# INSTALLER = env.PackageInstaller('create_installer', package_name)
# DEPLOY = env.PackageDeploy('deploy', installer_name)

# ################################
# ## TARGETS ALIASES AND DEPENDENCIES
# ################################

aliases = []
aliases.append(env.Alias('install-module',  env['TARGET_MODULE_PATH']))
aliases.append(env.Alias('install-python',  env['TARGET_PYTHON_PATH']))
aliases.append(env.Alias('install-icons',   env['TARGET_ICONS_PATH']))
aliases.append(env.Alias('install-bin',     env['TARGET_BINARIES']))
aliases.append(env.Alias('install-plugins', env['TARGET_PLUGIN_PATH']))
aliases.append(env.Alias('install-shaders', env['TARGET_SHADER_PATH']))
aliases.append(env.Alias('install-ext',     env['TARGET_EXTENSION_PATH']))

# top_level_alias(env, 'mtoa', MTOA)
# top_level_alias(env, 'docs', MTOA_API_DOCS)
# top_level_alias(env, 'shaders', MTOA_SHADERS)
# top_level_alias(env, 'testsuite', TESTSUITE)
top_level_alias(env, 'install', aliases)
top_level_alias(env, 'pack', PACKAGE)
# top_level_alias(env, 'deploy', DEPLOY)
# top_level_alias(env, 'installer', INSTALLER)

# env.Depends(INSTALLER, PACKAGE)
# env.Depends(DEPLOY, INSTALLER)

# env.AlwaysBuild(PACKAGE)

# #env.AlwaysBuild('install')
# Default(['mtoa', 'shaders'])

# ## Process top level aliases into the help message
# Help('''%s

# Top level targets:
#     %s

# Individual tests can be run using the 'test_nnnn' target.
# Note that this folder must fall within the TEST_PATTERN glob.
# ''' % (vars.GenerateHelpText(env), get_all_aliases()))

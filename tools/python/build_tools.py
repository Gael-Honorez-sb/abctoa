# vim: filetype=python

## load our own python modules
import system

import os, string, platform, subprocess, shutil

ALIASES = ''
def top_level_alias(env, name, targets):
   '''create a top level alias so that the help system will know about it'''
   global ALIASES
   ALIASES = '%s %s' % (ALIASES, name)
   env.Alias(name, targets)

def get_all_aliases():
   return ALIASES

def find_in_path(file):
   '''
   Searches for file in the system path. Returns a list of directories containing file
   '''
   path = os.environ['PATH']
   path = string.split(path, os.pathsep)
   return filter(os.path.exists, map(lambda dir, file=file: os.path.join(dir, file), path))

def find_files_recursive(path, valid_extensions):
   '''
   Returns a list of all files with an extension from the 'valid_extensions' list
   '''
   path = os.path.normpath(path)
   list = []
   for root, dirs, files in os.walk(path):
      if '.svn' in dirs:
         dirs.remove('.svn')
      for f in files:
         if os.path.splitext(f)[1] in valid_extensions:
            # Build the absolute path and then remove the root path, to get the relative path from root
            file = os.path.join(root, f)[len(path) + 1:]
            list += [file]
   return list

def find_files(path, valid_extensions):
   '''
   Returns a list of all files with an extension from the 'valid_extensions' list
   '''
   path = os.path.normpath(path)
   return [f for f in os.listdir(path) if any([f.endswith(ext) for ext in valid_extensions])]

def copy_dir_recursive(src, dest):
   '''
   Copy directories recursively, ignoring .svn dirs
   <dest> directory must not exist
   '''
   for f in os.listdir(src):
      src_path  = os.path.join(src, f)
      dest_path = os.path.join(dest, f)
      if os.path.isdir(src_path):
         if f != '.svn':
            os.makedirs(dest_path)
            #shutil.copystat(src_path, dest_path)
            copy_dir_recursive(src_path, dest_path)
      else:
         shutil.copy2(src_path, dest_path)

def saferemove(path):
   '''
   handy function to remove files only if they exist
   '''
   if os.path.exists(path):
      os.remove(path)

def copy_file_or_link(src, target):
   '''
   Copies a file or a symbolic link (creating a new link in the target dir)
   '''
   if os.path.isdir(target):
      target = os.path.join(target, os.path.basename(src))

   if os.path.islink(src):
      linked_path = os.readlink(src)
      os.symlink(linked_path, target)
   else:
      shutil.copy(src, target)

def process_return_code(retcode):
   '''
   translates a process return code (as obtained by os.system or subprocess) into a status string
   '''
   if retcode == 0:
      status = 'OK'
   else:
      if system.os() == 'windows':
         if retcode < 0:
            status = 'CRASHED'
         else:
            status = 'FAILED'
      else:
         if retcode > 128:
            status = 'CRASHED'
         else:
            status = 'FAILED'
   return status

def get_arnold_version(path, components = 4):
   '''
   Obtains Arnold library version by parsing 'ai_version.h'
   '''
   ARCH_VERSION=''
   MAJOR_VERSION=''
   MINOR_VERSION=''
   FIX_VERSION=''

   f = open(path, 'r')
   while True:
      line = f.readline().lstrip(' \t')
      if line == "":
         # We have reached the end of file.
         break
      if line.startswith('#define'):
         tokens = line.split()
         if tokens[1] == 'AI_VERSION_ARCH_NUM':
            ARCH_VERSION = tokens[2]
         elif tokens[1] == 'AI_VERSION_MAJOR_NUM':
            MAJOR_VERSION = tokens[2]
         elif tokens[1] == 'AI_VERSION_MINOR_NUM':
            MINOR_VERSION = tokens[2]
         elif tokens[1] == 'AI_VERSION_FIX':
            FIX_VERSION = tokens[2].strip('"')
   f.close()

   if (components > 0):
      version = ARCH_VERSION
   if (components > 1):
      version += '.' + MAJOR_VERSION
   if (components > 2):
      version += '.' + MINOR_VERSION
   if (components > 3):
      version += '.' + FIX_VERSION
   return version

def get_maya_version(path):
   f = open(path, 'r')
   while True:
      line = f.readline().lstrip(' \t')
      if line == "":
         # We have reached the end of file.
         break
      if line.startswith('#define'):
         tokens = line.split()
         if tokens[1] == 'MAYA_API_VERSION':
            return tokens[2][:]
   f.close()

def get_latest_revision():
   '''
   This function will give us the information we need about the latest snv revision of the root arnold directory
   '''
   p = subprocess.Popen('svn info .', shell=True, stdout = subprocess.PIPE)
   retcode = p.wait()

   revision = 'not found'
   url      = 'not found'

   for line in p.stdout:
      if line.startswith('URL:'):
         url = line[5:].strip()
      elif line.startswith('Last Changed Rev:'):
         revision = 'r' + line[18:].strip()

   return (revision, url)

## Hacky replacement for the string partition method which is only available from Python 2.5
def strpartition(string, sep):
   index = string.find(sep)
   if index == -1:
      return (string, '', '')
   return (string[:index], sep, string[index + 1:])

def append_to_path(env, new_path):
    if system.os() == 'windows':
       env.AppendENVPath('PATH', new_path, envname='ENV', sep=';', delete_existing=1)
    else :
       env.AppendENVPath('PATH', new_path, envname='ENV', sep=':', delete_existing=1)

def prepend_to_path(env, new_path):
    if system.os() == 'windows':
       env.PrependENVPath('PATH', new_path, envname='ENV', sep=';', delete_existing=1)
    else :
       env.PrependENVPath('PATH', new_path, envname='ENV', sep=':', delete_existing=1)

def get_default_path(var, default):
   if var in os.environ:
      return os.environ[var]
   else:
      return default

def get_escaped_path(path):
   if system.os() == 'windows':
      return path.replace("\\", "\\\\")
   else:
      return path

def get_version():
  f = open(os.path.join('maya', 'alembicHolder', 'Version.h'), 'r')
  while True: 
    line = f.readline().lstrip(' \t')
    if line == "": 
      break
    if line.startswith('#define'):
       tokens = line.split()
       if tokens[1] == 'HOLDER_VERSION_NUM':
        return tokens[2]

  return "0"

def make_module(env, target, source):
   # When symbols defined in the plug-in
   if not os.path.exists(os.path.dirname(source[0])) and os.path.dirname(source[0]):
      os.makedirs(os.path.dirname(source[0]))
   f = open(source[0], 'w' )
   # Maya got problems with double digit versions
   # f.write('+ mtoa %s %s\n' % (get_mtoa_version(3), target[0]))
   version = get_version()
   f.write('+ AbcToA %s .\\AbcToA-%s\n' % (version, version))
   f.write('PATH +:=procedurals\n')
   f.write('PATH +:=bin\n')
   f.write('ARNOLD_PLUGIN_PATH +:=shaders\n')
   f.write('MTOA_EXTENSIONS_PATH +:=extensions\n')
   f.close()

def get_mayalibrary_extension():
   if system.os() == 'windows':
      return ".mll"
   elif system.os() == 'linux':
      return ".so"
   elif system.os() == 'darwin':
      return ".dylib"
   else:
      return ""

def get_library_extension():
   if system.os() == 'windows':
      return ".dll"
   elif system.os() == 'linux':
      return ".so"
   elif system.os() == 'darwin':
      return ".dylib"
   else:
      return ""

def get_executable_extension():
   if system.os() == 'windows':
      return ".exe"
   else:
      return ""

try:
    # available in python >= 2.6
    relpath = os.path.relpath
except AttributeError:
    def relpath(path, start=os.path.curdir):
        """Return a relative version of a path"""

        if not path:
            raise ValueError("no path specified")

        start_list = os.path.abspath(start).split(os.path.sep)
        path_list = os.path.abspath(path).split(os.path.sep)

        # Work out how much of the filepath is shared by start and path.
        i = len(os.path.commonprefix([start_list, path_list]))

        rel_list = [os.path.pardir] * (len(start_list)-i) + path_list[i:]
        if not rel_list:
            return os.path.curdir
        return os.path.join(*rel_list)

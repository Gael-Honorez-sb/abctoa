#! /usr/bin/python

import sys
import os
import errno
import shutil
import glob
import tarfile
import zipfile
import platform

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else: raise

MAJOR_VERSION = '@ABCTOA_MAJOR_VERSION@'
MINOR_VERSION = '@ABCTOA_MINOR_VERSION@'
PATCH_VERSION = '@ABCTOA_PATCH_VERSION@'
ABCTOA_VERSION = "%s.%s.%s" % (MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION)
MTOA_VERSION = '@MTOA_VERSION@'
MAYA_VERSION = '@MAYA_VERSION@'

# Binary distribution
files_src = ['INSTALL', 'README', 'LICENSE.txt', 'COPYING.LESSER']

name_osx = 'abcToA-osx-%s-maya-%s-MtoA-%s' % (ABCTOA_VERSION, MAYA_VERSION, MTOA_VERSION)
name_win = 'abcToA-win-%s-maya-%s-MtoA-%s' % (ABCTOA_VERSION, MAYA_VERSION, MTOA_VERSION)
name_linux = 'abcToA-linux-%s-maya-%s-MtoA-%s' % (ABCTOA_VERSION, MAYA_VERSION, MTOA_VERSION)


def copyFilesToDistDir(files, distDir):
    for fn in files:
        shutil.copy(fn, distDir)


def createArchive(distDir, name, isSrc=False):
    if not isSrc and platform.system() == "Windows":
        f = zipfile.ZipFile(os.path.join('..', '%s.zip' % name), 'w')
        for fn in glob.iglob(os.path.join(distDir, '*')):
            f.write(fn, arcname=os.path.join(name, os.path.basename(fn)))
        f.close()
    else:
        f = tarfile.open(os.path.join('..', '%s.tar.gz' % name), 'w:gz')
        f.add(distDir, arcname=name)
        f.close()


def createBinaryDistribution(name, droot):
    outname = ""
    if platform.system() == "Windows":
        outname = os.path.join('%s.zip' % name)
        print "writing", outname
        zf = zipfile.ZipFile(os.path.join('%s.zip' % name), 'w')
        for root, dirs, files in os.walk(droot):
            rroot = os.path.relpath(root, droot)
            for f in files:
                if not f.endswith(".lib"):
                    rfile = os.path.join(rroot, f)
                    zf.write(os.path.join(droot, rfile), arcname=os.path.join("modules", "abcToA-%s" % ABCTOA_VERSION, rfile))
        for f in files_src:
            zf.write(f, arcname=os.path.join(f))
        zf.write("abcToA.mod", arcname=os.path.join("modules", "abcToA.mod"))
    else:
        outname = os.path.join('%s.tar.gz' % name)
        print "writing", outname
        zf = tarfile.open(os.path.join('%s.tar.gz' % name), 'w:gz')
        for root, dirs, files in os.walk(droot):
            rroot = os.path.relpath(root, droot)
            for f in files:
                rfile = os.path.join(rroot, f)
                zf.add(os.path.join(droot, rfile), arcname=os.path.join("modules", "abcToA-%s" % ABCTOA_VERSION, rfile))
        for f in files_src:
            zf.add(f, arcname=os.path.join(f))
        zf.add("abcToA.mod", arcname=os.path.join("modules", "abcToA.mod"))
        zf.close()

    return outname

# Binary distribution
droot = '%s' % ("@INSTALL_DIR@")

if platform.system() == "Darwin":
    # OS X distribution
    outname = createBinaryDistribution(name_osx, droot)
elif platform.system() == "Windows":
    # Windows
    outname = createBinaryDistribution(name_win, droot)
    serverPath = "//server01/shared/Dev/AbcToA_Builds"
    if os.path.exists(serverPath):
        shutil.copyfile(outname, os.path.join(serverPath, outname))

elif platform.system() == "Linux":
    # Linux
    outname = createBinaryDistribution(name_linux, droot)
else:
    print 'Warning: unknown system "%s", not creating binary package' % platform.system()
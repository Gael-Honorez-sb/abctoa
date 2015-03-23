# Alembic Holder
# Copyright (c) 2014, Gael Honorez, All rights reserved.
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3.0 of the License, or (at your option) any later version.
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
# You should have received a copy of the GNU Lesser General Public
# License along with this library.

import maya.cmds as cmds

import maya.OpenMayaUI as apiUI
import shiboken
from PySide import QtGui

global _shadermanager
_shadermanager = None

def getMayaWindow():
    """
    Get the main Maya window as a QtGui.QMainWindow instance
    @return: QtGui.QMainWindow instance of the top level Maya windows
    """
    ptr = apiUI.MQtUtil.mainWindow()
    if ptr is not None:
        return shiboken.wrapInstance(long(ptr), QtGui.QMainWindow)

def alembicShaderManager(mayaWindow):
    global _shadermanager
    import shaderManager
    
    reload(shaderManager)
    return shaderManager.manager(mayaWindow)

def reloadShaderManager(mayaWindow):
    global _shadermanager
    import shaderManager
    _shadermanager.clearing()
    _shadermanager.deleteLater()
    reload(shaderManager)
    _shadermanager = shaderManager.manager(mayaWindow)

def createAlembicHolder():
    x = cmds.createNode('alembicHolder', n="AlembicHolderShape")
    cmds.setAttr("%s.overrideLevelOfDetail" % x, 1)
    cmds.setAttr("%s.overrideVisibility" % x, 1) 
    cmds.setAttr("%s.visibleInRefractions" % x, 1)
    cmds.setAttr("%s.visibleInReflections" % x, 1)
    cmds.connectAttr("time1.outTime", "%s.time" % x)

def exportAssignations():
    ''' export json and shaders'''
    from alembicHolder.cmds import cachesBaker
    cachesBaker.writeAbcShaders()
    cachesBaker.writeJson()

def importAssignations():
    ''' select a json file, and apply it to a alembicHolder '''
    from alembicHolder.cmds import cachesBaker
    cachesBaker.applyAssignations()

def assignTagsFromSetName():
    import json
    sets = cmds.ls(sl=True, type="objectSet")

    for set in sets:
        for s in cmds.sets( set, q=True ):
            tags = []
            if not cmds.objExists(s + ".mtoa_constant_tags"): 
                cmds.addAttr(s, ln='mtoa_constant_tags', dt='string') 
            else:
                try:
                    tags = json.loads(cmds.getAttr(s + ".mtoa_constant_tags"))
                except:
                    pass

            if not set in tags:
                tags.append(set)
                cmds.setAttr(s + ".mtoa_constant_tags", json.dumps(tags), type="string")

				
def registerAlembicHolder():
    if not cmds.about(b=1):
        mayaWindow = getMayaWindow()
        global _shadermanager
        _shadermanager = alembicShaderManager(mayaWindow)        
        cmds.menu('AlembicHolderMenu', label='Alembic Holder', parent='MayaWindow', tearOff=True )
        cmds.menuItem('CreateAlembicHolder', label='Create Holder', parent='AlembicHolderMenu', c=lambda *args: createAlembicHolder())
        cmds.menuItem('AlembicShaderManager', label='Shader Manager', parent='AlembicHolderMenu', c=lambda *args: _shadermanager.show())
        cmds.menuItem('ReloadAlembicShaderManager', label='Reload Shader Manager', parent='AlembicHolderMenu', c=lambda *args: reloadShaderManager(mayaWindow))
        cmds.menuItem( divider=True )
        cmds.menuItem('exportAssign', label='Export Assignations on selected caches', parent='AlembicHolderMenu', c=lambda *args: exportAssignations())
        cmds.menuItem('importtAssign', label='Import Assignation on selected caches', parent='AlembicHolderMenu', c=lambda *args: importAssignations())
        cmds.menuItem( divider=True )
        cmds.menuItem('assignTagsSets', label='Assign tags from Selected Selection Sets', parent='AlembicHolderMenu', c=lambda *args: assignTagsFromSetName())
        cmds.menuItem( divider=True )
        cmds.menuItem('OnlineDocumentation', label='Online Documentation', parent='AlembicHolderMenu', c=lambda *args: cmds.launch(webPage='http://bitbucket.org/thepilot/abctoarnold/wiki/Home'))


#-------------------------------------------------------------------------------
# Copyright (c) 2014 Gael Honorez.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the GNU Public License v3.0
# which accompanies this distribution, and is available at
# http://www.gnu.org/licenses/gpl.html
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#-------------------------------------------------------------------------------

import maya.cmds as cmds
import shaderManager

def alembicShaderManager():
    reload(shaderManager)
    shaderManager.manager().show()

def createAlembicHolder():
    x = cmds.createNode('alembicHolder', n="AlembicHolderShape")
    cmds.setAttr("%s.overrideLevelOfDetail" % x, 1)
    cmds.setAttr("%s.overrideVisibility" % x, 1) 
    cmds.setAttr("%s.visibleInRefractions" % x, 1)
    cmds.setAttr("%s.visibleInReflections" % x, 1)
    cmds.connectAttr("time1.outTime", "%s.time" % x)


def registerAlembicHolder():
    if not cmds.about(b=1):
        cmds.menu('AlembicHolderMenu', label='Alembic Holder', parent='MayaWindow', tearOff=True )
        cmds.menuItem('CreateAlembicHolder', label='Create Holder', parent='AlembicHolderMenu', c=lambda *args: createAlembicHolder())
        cmds.menuItem('AlembicShaderManager', label='Shader Manager', parent='AlembicHolderMenu', c=lambda *args: alembicShaderManager())

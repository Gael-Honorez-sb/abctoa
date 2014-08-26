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

def registerAlembicHolder():
	if not cmds.about(b=1):
		cmds.menu('AlembicHolderMenu', label='Alembic Holder', parent='MayaWindow', tearOff=True )
		cmds.menuItem('AlembicShaderManager', label='Shader Manager', parent='AlembicHolderMenu', c=lambda *args: alembicShaderManager())
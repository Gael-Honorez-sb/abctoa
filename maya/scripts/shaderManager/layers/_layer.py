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

from shaderManager.assignations._assignationGroup import assignationGroup

class layer(object):
    def __init__(self, parent=None, layername="", fromFile=False):
        self.parent = parent
        self.layerName = layername
        self.assignation = assignationGroup(fromFile)

        self._removeDisplacements = False
        self._removeProperties = False
        self._removeShaders = False

    @property
    def removeDisplacements(self):
        return self._removeDisplacements

    @property
    def removeProperties(self):
        return self._removeProperties

    @property
    def removeShaders(self):
        return self._removeShaders

    @removeProperties.setter
    def removeProperties(self, value):
        self._removeProperties= value
        self.parent.writeLayer()

    @removeDisplacements.setter
    def removeDisplacements(self, value):
        self._removeDisplacements= value
        self.parent.writeLayer()

    @removeShaders.setter
    def removeShaders(self, value):
        self._removeShaders = value
        self.parent.writeLayer()

    def addShaders(self, shaders):
        self.assignation.addShaders(shaders)

    def addOverrides(self, overrides):
        self.assignation.addOverrides(overrides)

    def addDisplacements(self, displacements):
        self.assignation.AddDisplacements(displacements)

    def getAssignation(self):
        return self.assignation

    def getLayerOverrides(self):
        return dict(removeDisplacements=self.removeDisplacements, removeProperties=self.removeProperties, removeShaders=self.removeShaders)

    def setRemovedShader(self, state):
        self.removeShaders = state     

    def setRemovedDisplace(self, state):
        self.removeDisplacements = state   

    def setRemovedProperties(self, state):
        self.removeProperties = state           

    def getOverrideValue(self, path, prop):
        return self.assignation.getOverrideValue(path, prop)

    def updateOverride(self, propName, default, value, curPath):
        self.assignation.updateOverride(propName, default, value, curPath)

    def removeOverride(self, curPath, propName):
        self.assignation.removeOverride(curPath, propName)

    def getOverrides(self):
        return self.assignation.getOverrides()

    def assignShader(self, path, shader):
        self.assignation.assignShader(path, shader)
    
    def assignDisplacement(self, path, shader):
        self.assignation.assignDisplacement(path, shader)

    def getShaders(self):
        return self.assignation.getShaders()

    def getDisplacements(self):
        return self.assignation.getDisplacements()

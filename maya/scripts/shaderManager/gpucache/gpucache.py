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

from shaderManager.assignations._assignations import cacheAssignations


import json
import maya.cmds as cmds

import pymel.core as pm


class gpucache(object):
    def __init__(self, shape, parent=None):
        self.parent = parent
        self.shape = shape
        self.cache = None
        self.ABCcache = ""
        self.assignations = cacheAssignations(self)

        self.curPath = ""
        self.tags = []
        self.itemsTree = []

    def getAssignations(self):
        ''' return the assignations manager'''
        return self.assignations

    def getLayerOverrides(self, layer):
        return self.assignations.getLayerOverrides(layer)

    def updateOverride(self, propName, default, value, curPath, layer):
        self.assignations.updateOverride(propName, default, value, curPath, layer)

    def getPropertyState(self, layer, propName, curPath):
        return self.assignations.getPropertyState(layer, propName, curPath)

    def assignShader(self, path, shader):
        ''' assign a shader to a path'''
        layer = self.parent.getLayer()
        self.assignations.assignShader(layer, path, shader)

    def assignDisplacement(self, path, shader):
        ''' assign a displacement to a path'''
        layer = self.parent.getLayer()
        self.assignations.assignDisplacement(layer, path, shader)


    def updateCache(self):
        self.ABCcache =  cmds.getAttr("%s.cacheFileName" % self.shape)
        self.ABCcurPath = cmds.getAttr("%s.cacheGeomPath" % self.shape)

    def updateTags(self):
        self.tags = cmds.ABCGetTags(self.ABCcache)

    def updateShaders(self, shaders):
        cmds.setAttr(self.shape + ".mtoa_constant_shaderAssignation", json.dumps(shaders), type="string")

    def updateDisplacements(self, shaders):
        cmds.setAttr(self.shape + ".mtoa_constant_displacementAssignation", json.dumps(shaders), type="string")
          
    def updateOverrides(self, val):
        cmds.setAttr(self.shape + ".mtoa_constant_overrides", json.dumps(val), type="string")

    def updateLayerOverrides(self, val):       
        cmds.setAttr(self.shape + ".mtoa_constant_layerOverrides", json.dumps(val), type="string")

    def updateConnections(self):
        pm.disconnectAttr("%s.shaders" % self.shape)
        port = 0
        
        added = []
        for shader in self.assignations.getAllShaders():
            if not shader in added:
                if cmds.objExists(shader):
                    cmds.connectAttr( shader + ".message", self.shape + ".shaders[%i]" % port)
                    added.append(shader)
                    port = port + 1

        for shader in self.assignations.getAllDisplacements():
            shaderClean = shader.replace(".message", "")
            if not shaderClean in added:
                if cmds.objExists(shaderClean):
                    cmds.connectAttr( shaderClean + ".message", self.shape + ".shaders[%i]" % port)
                    added.append(shaderClean)
                    port = port + 1

    def setSelection(self, topath):
        if topath == "/":
            topath =""
        cmds.setAttr("%s.cacheSelectionPath" % self.shape, str(topath).replace("|", "/"), type="string")


    def setToPath(self, topath):
        cmds.setAttr("%s.cacheGeomPath" % self.shape, str(topath).replace("/", "|"), type="string")
        for item in self.itemsTree:
            if item.getPath() != topath:
                item.setCheckState(0, False)

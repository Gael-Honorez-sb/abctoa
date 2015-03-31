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

from shaderManager.assignations._assignations import cacheAssignations
from PySide import QtCore

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
        self.tags = {}
        self.itemsTree = []

    def isValid(self):
        ''' check if this cache is still valid'''
        return cmds.objExists(self.shape)


    def getAbcShader(self):
        return cmds.getAttr(self.shape + ".abcShaders")

    def getAllTags(self):
        return self.tags

    def getAbcPath(self):
        return self.ABCcache

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

    def removeShader(self, shader):
        ''' remove a shader '''
        self.assignations.removeShader(shader)

    def removeDisplacement(self, shader):
        ''' remove a displacement shader '''
        self.assignations.removeDisplacement(shader)                

    def renameShader(self, oldname, newname):
        ''' rename a shader '''
        self.assignations.renameShader(oldname, newname)

    def renameDisplacement(self, oldname, newname):
        ''' rename a displacement shader '''
        self.assignations.renameDisplacement(oldname, newname)

    def updateCache(self):
        if self.isValid():
            self.ABCcache =  cmds.getAttr("%s.cacheFileName" % self.shape)
            self.ABCcurPath = cmds.getAttr("%s.cacheGeomPath" % self.shape)

    def updateTags(self):
        tags = cmds.ABCGetTags(self.ABCcache)
        try:
            self.tags = json.loads(tags)
        except:
            self.tags = {}

    def updateShaders(self, shaders):
        if self.isValid():
            cmds.setAttr(self.shape + ".shadersAssignation", json.dumps(shaders), type="string")
            cmds.setAttr(self.shape + ".forceReload", 1)

    def updateDisplacements(self, shaders):
        if self.isValid():
            cmds.setAttr(self.shape + ".displacementsAssignation", json.dumps(shaders), type="string")

    def updateOverrides(self, val):
        if self.isValid():
            cmds.setAttr(self.shape + ".attributes", json.dumps(val), type="string")

    def updateLayerOverrides(self, val):
        if self.isValid():
            cmds.setAttr(self.shape + ".layersOverride", json.dumps(val), type="string")

    def updateConnections(self):
        if self.isValid():
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
        if self.isValid():
            if len(topath) == 0:
                topath = ""
            else:
                topath = json.dumps(topath)

            cmds.setAttr("%s.cacheSelectionPath" % self.shape, topath, type="string")

    def getSelection(self):
        if self.isValid():
            return cmds.getAttr("%s.cacheSelectionPath" % self.shape)

    def setToPath(self, topath):
        if self.isValid():
            cmds.setAttr("%s.cacheGeomPath" % self.shape, topath, type="string")
        # for item in self.itemsTree:
        #     if item.getPath() != topath:
        #         item.setCheckState(0, QtCore.Qt.Unchecked)

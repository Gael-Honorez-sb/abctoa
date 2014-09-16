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

from pprint import pprint
import fnmatch
import re

class assignationGroup(object):
    def __init__(self, parent=None, fromFile = False):
        self.parent = parent
        self.fromFile = fromFile
        self.shaders = {}
        self.overrides = {}
        self.displacements = {}

    def addShaders(self, shaders):
        self.shaders = shaders

    def addOverrides(self, overrides):
        self.overrides = overrides

    def removeOverride(self, curPath, propName):
        if curPath in self.overrides:
            if propName in self.overrides[curPath]:
                del self.overrides[curPath][propName]

        if len(self.overrides[curPath]) == 0:
            del self.overrides[curPath]


    def updateOverride(self, propName, default, value, curPath):
        if not default:
            if not curPath in self.overrides:
                self.overrides[curPath] = {}
            self.overrides[curPath][propName] = value
        else:
            self.removeOverride(curPath, propName)

    def addDisplacements(self, displacements):
        self.displacements = displacements

    def setFromFile(self, state):
        self.fromFile = state

    def getShaderFromPath(self, path):
        for shader in self.shaders:
            if path in self.shaders[shader]:
                return self.createShaderEntity(shader)

        foundShader = None
        foundPath = ""
        # get the closest path possible
        for shader in self.shaders:
            for shaderpath in self.shaders[shader]:
                if shaderpath in path:
                    if shaderpath > foundPath:
                        foundPath = shaderpath
                        foundShader = shader

        if foundShader:
            return self.createShaderEntity(foundShader, inherited=True)


        ### if we go this far, we didn't find any shader. We are iterating over the wildcards.
        wildShaders = self.getWildShaders()
        for shader in wildShaders:
            for wildcard in wildShaders[shader]:
                pattern = fnmatch.translate(wildcard)
            if re.match(pattern, path):
                return self.createShaderEntity(shader, inherited=True)

        return None

    def getDisplaceFromPath(self, path):
        for shader in self.displacements:
            if path in self.displacements[shader]:
                return self.createShaderEntity(shader)

        foundShader = None
        foundPath = ""
        # get the closest path possible
        for shader in self.displacements:
            for shaderpath in self.displacements[shader]:
                if shaderpath in path:
                    if shaderpath > foundPath:
                        foundPath = shaderpath
                        foundShader = shader

        if foundShader:
            return self.createShaderEntity(foundShader, inherited=True)


        ### if we go this far, we didn't find any shader. We are iterating over the wildcards.
        wildShaders = self.getWildDisplacements()
        for shader in wildShaders:
            for wildcard in wildShaders[shader]:
                pattern = fnmatch.translate(wildcard)
            if re.match(pattern, path):
                return self.createShaderEntity(shader, inherited=True)

        return None

    def getOverrides(self):
        return self.overrides

    def getOverridesFromPath(self, path):
        if path in self.overrides:
            return self.createOverrideEntity(self.overrides[path])

        foundAttribute = None
        foundPath = ""
        for attributepath in self.overrides:
            if attributepath in path:
                if attributepath > foundPath:
                    foundPath = attributepath
                    foundAttribute = self.overrides[attributepath]

        if foundAttribute:
            return self.createOverrideEntity(foundAttribute, inherited=True)

        ### if we go this far, we didn't find any shader. We are iterating over the wildcards.            
        wildAttributes = self.getWildAttributes()
        for wildcard in wildAttributes:
            pattern = fnmatch.translate(wildcard)
            if re.match(pattern, path):
                return self.createOverrideEntity(wildAttributes[wildcard], inherited=True)


        return None

    def getOverrideValue(self, path, prop):
        if path in self.overrides:
            if prop in self.overrides[path]:
                return self.overrides[path][prop]
        return None


    def createShaderEntity(self, shader, inherited= False):
        return dict(shader=shader, fromfile=self.fromFile, inherited = inherited)

    def createOverrideEntity(self, overrides, inherited = False):
        return dict(overrides=overrides, fromfile=self.fromFile, inherited = inherited)

    def assignShader(self, path, shader):
        toRemove = []

        if shader == None:
            for sh in self.shaders:
                if path in self.shaders[sh]:
                    self.shaders[sh].remove(path)
                    if len(self.shaders[sh]) == 0:
                        toRemove.append(sh)

        else:
            if not shader in self.shaders:
                self.shaders[shader] = []
            if not path in self.shaders[shader]:
                self.shaders[shader].append(path)

            for othershader in self.shaders:
                if shader == othershader:
                    continue

                if path in self.shaders[othershader] :
                    self.shaders[othershader].remove(path)
                    if len(self.shaders[othershader]) == 0:
                        toRemove.append(othershader)

        for shaderToRemove in toRemove:
            del self.shaders[shaderToRemove]

    def assignDisplacement(self, path, shader):
        toRemove = []

        if shader == None:
            for sh in self.displacements:
                if path in self.displacements[sh]:
                    self.displacements[sh].remove(path)
                    if len(self.displacements[sh]) == 0:
                        toRemove.append(sh)

        else:

            if not shader in self.displacements:
                self.displacements[shader] = []
            if not path in self.displacements[shader]:
                self.displacements[shader].append(path)

            for othershader in self.displacements:
                if shader == othershader:
                    continue
                if path in self.displacements[othershader] :
                    self.displacements[othershader].remove(path)
                    if len(self.displacements[othershader]) == 0:
                        toRemove.append(othershader)

        for shaderToRemove in toRemove:
            del self.displacements[shaderToRemove]


    def getShaders(self):
        return self.shaders

    def getWildShaders(self):
        wildShaders = {}
        for shader in self.shaders:
            for path in self.shaders[shader]:
                if not path.startswith("/"):
                    if not shader in wildShaders:
                        wildShaders[shader] = []    
                    wildShaders[shader].append(path)

        return wildShaders

    def getWildDisplacements(self):
        wildShaders = {}
        for shader in self.displacements:
            for path in self.displacements[shader]:
                if not path.startswith("/"):
                    if not shader in wildShaders:
                        wildShaders[shader] = []    
                    wildShaders[shader].append(path)

        return wildShaders

    def getWildAttributes(self):
        wildAttributes = {}
        for path in self.overrides:
            if not path.startswith("/"):
                wildAttributes[path] = self.overrides[path]

        return wildAttributes

    def getWildCards(self):
        wilds = []
        for paths in self.shaders.values() + self.getDisplacements().values():
            for path in paths:
                if not path.startswith("/"):
                    if not path in wilds:
                        wilds.append(dict(name=path, fromfile=self.fromFile))
        return wilds


    def getDisplacements(self):
        return self.displacements

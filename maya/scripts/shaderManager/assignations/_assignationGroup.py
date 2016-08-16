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
    def __init__(self, parent=None, gpucache=None, fromFile = False, fromlayer = None):
        self.parent = parent
        self.gpucache = gpucache
        self.fromFile = fromFile
        self.fromLayer = fromlayer
        self.shaders = {}
        self.overrides = {}
        self.displacements = {}

    def getAllTags(self):
        return self.gpucache.getAllTags()
        
    def addShaders(self, shaders):
        self.shaders = shaders

    def AddDisplacements(self, shaders):
        self.displacements = shaders

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

    def isPathContainsInOtherPath(self, path, otherpath):
        if otherpath == "/":
            return True
            
        pathParts = path.split("/")
        jsonPathParts = otherpath.split("/")

        if len(jsonPathParts) > len(pathParts):
            return False

        validPath = True

        for i in range(len(jsonPathParts)):
            if pathParts[i] != jsonPathParts[i]:
                validPath = False


        return validPath


    def getShaderFromPath(self, path):
        for shader in self.shaders:
            if path in self.shaders[shader]:
                return self.createShaderEntity(shader)

        foundShader = None
        foundPath = ""
        # get the closest path possible
        for shader in self.shaders:
            for shaderpath in self.shaders[shader]:
                if self.isPathContainsInOtherPath(path, shaderpath):
                    if shaderpath > foundPath:
                        foundPath = shaderpath
                        foundShader = shader

        if foundShader:
            return self.createShaderEntity(foundShader, inherited=True)

        wildShaders = self.getWildShaders()

        # find from tag assignation.
        tags = self.getAllTags()
        for shader in wildShaders:
            for tag in wildShaders[shader]:
                if tags:
                    if tag in tags:
                        if path in tags[tag]:
                            return self.createShaderEntity(shader, inherited=True)

        ### if we go this far, we didn't find any shader. We are iterating over the wildcards.        
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
                if self.isPathContainsInOtherPath(path, shaderpath):
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


    def getOverridesFromPath(self, path, onlyInherited=False):
        ''' Get all overrides of a path (inherited included) '''

        concatenedOverrides = {}

        # we start with the wildcard as they are the most generic one.
        # They will eventually get overridden by the specific ones.

        wildAttributes = self.getWildAttributes()
        for wildcard in sorted(wildAttributes):
            # by sorting, we are iterating from the smaller to biggest wildcard. 
            pattern = fnmatch.translate(wildcard)
            if re.match(pattern, path):
                overrides = wildAttributes[wildcard]
                for attr in overrides:
                    concatenedOverrides[attr] = self.createOverrideEntity(overrides[attr], inherited=True)

        for attributepath in sorted(self.overrides):
            # by sorting, we are iterating from the smaller to biggest attr. 
            if self.isPathContainsInOtherPath(path, attributepath) and attributepath != path:
                overrides = self.overrides[attributepath]
                for attr in overrides:
                    concatenedOverrides[attr] = self.createOverrideEntity(overrides[attr], inherited=True)


        # finally, we iterate over our own path if possible and allowed to.
        if onlyInherited == False:
            if path in self.overrides:
                overrides = self.overrides[path]
                for attr in overrides:
                    concatenedOverrides[attr] = self.createOverrideEntity(overrides[attr])

        return concatenedOverrides

    def getOverrideValue(self, path, prop):
        if path in self.overrides:
            if prop in self.overrides[path]:
                return self.overrides[path][prop]
        return None


    def createShaderEntity(self, shader, inherited= False):
        return dict(shader=shader, fromfile=self.fromFile, inherited = inherited, fromlayer=self.fromLayer)

    def createOverrideEntity(self, override, inherited = False):
        return dict(override=override, fromfile=self.fromFile, inherited = inherited, fromlayer=self.fromLayer)

    def removeShader(self, shader):
        ''' remove a shader from all paths '''
        if shader in self.shaders:
            del self.shaders[shader]

    def removeDisplacement(self, shader):
        ''' remove a displacement shader from all paths '''
        if shader in self.displacements:
            del self.displacements[shader]

    def renameShader(self, oldname, newname):
        ''' rename a shader  '''
        if oldname in self.shaders:
            self.shaders[newname] = self.shaders[oldname]
            del self.shaders[oldname]

    def renameDisplacement(self, oldname, newname):
        ''' rename a displacement shader  '''
        if oldname in self.displacements:
            self.displacements[newname] = self.displacements[oldname]
            del self.displacements[oldname]

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

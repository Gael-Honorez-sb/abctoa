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

import copy
from shaderManager.layers._layers import Layers
from shaderManager.assignations._assignationGroup import assignationGroup

class cacheAssignations(object):
    def __init__(self, parent=None):
        self.parent = parent
        self.gpucache = self.parent

        self.mainAssignations = assignationGroup(self, self.gpucache)
        self.mainAssignationsFromFile = assignationGroup(self, self.gpucache, fromFile=True)

        self.layers = Layers(self, self.gpucache)
        self.layersFromFile = Layers(self, self.gpucache, fromFile=True)


    def writeLayer(self):
        fromfiledict = self.layersFromFile.getLayerDict()
        layerdict = self.layers.getLayerDict()

        cleanedDict = copy.deepcopy(layerdict)

        for layer in layerdict:
            if layer in fromfiledict:
                # check if the layer overrides were false or true in the orig file
                if fromfiledict[layer]["removeShaders"] == layerdict[layer]["removeShaders"] :
                    del cleanedDict[layer]["removeShaders"]

                if fromfiledict[layer]["removeDisplacements"] == layerdict[layer]["removeDisplacements"] :
                    del cleanedDict[layer]["removeDisplacements"]

                if fromfiledict[layer]["removeProperties"] == layerdict[layer]["removeProperties"]:
                    del cleanedDict[layer]["removeProperties"]
            else:

                if layerdict[layer]["removeShaders"] == False :
                    del cleanedDict[layer]["removeShaders"]

                if layerdict[layer]["removeDisplacements"] == False :
                    del cleanedDict[layer]["removeDisplacements"]

                if layerdict[layer]["removeProperties"] == False :
                    del cleanedDict[layer]["removeProperties"]

            if len(layerdict[layer]["properties"]) == 0:
                del cleanedDict[layer]["properties"]

            if len(layerdict[layer]["shaders"]) == 0:
                del cleanedDict[layer]["shaders"]


            if len(layerdict[layer]["displacements"]) == 0:
                del cleanedDict[layer]["displacements"]


            if len(cleanedDict[layer]) == 0:
                del cleanedDict[layer]

        self.parent.updateLayerOverrides(cleanedDict)

    def writeOverrides(self):
        self.parent.updateOverrides(self.mainAssignations.getOverrides())

    def addShaders(self, shaders, fromFile=False):
        if not fromFile:
            self.mainAssignations.addShaders(shaders)
        else:
            self.mainAssignationsFromFile.addShaders(shaders)

    def addOverrides(self, overrides, fromFile=False):
        if not fromFile:
            self.mainAssignations.addOverrides(overrides)
        else:
            self.mainAssignationsFromFile.addOverrides(overrides)

    def addDisplacements(self, displacements, fromFile=False):
        if not fromFile:
            self.mainAssignations.addDisplacements(displacements)
        else:
            self.mainAssignationsFromFile.addDisplacements(displacements)

    def addLayers(self, layers, fromFile=False):
        if not fromFile:
            self.layers.addLayers(layers, fromFile)
        else:
            self.layersFromFile.addLayers(layers, fromFile)


    def getShader(self, path, layer):
        shader = None
        if layer == None:
            shader = self.mainAssignations.getShaderFromPath(path)
            if not shader:
                shader = self.mainAssignationsFromFile.getShaderFromPath(path)
        else:
            shader = self.layers.getShaderFromPath(path, layer)
            if not shader:
                shader = self.layersFromFile.getShaderFromPath(path, layer)

        return shader

    def getAllTags(self):
        return self.gpucache.getAllTags()

    def getAllShaderPaths(self):
        paths = []
        for shader in self.mainAssignations.getShaders().values():
            paths.append(shader)
        return paths

    def getAllLayerShaderPaths(self):
        return self.layers.getAllShaderPaths()

    def getAllShaders(self):
        return self.mainAssignations.getShaders().keys()  + self.layers.getShaders()

    def getAllWidcards(self):
        return self.mainAssignations.getWildCards() + self.layers.getWildCards()

    def getAllDisplacements(self):
        return self.mainAssignations.getDisplacements().keys() + self.layers.getDisplacements()

    def getDisplace(self, path, layer):
        shader = None
        if layer == None:
            shader = self.mainAssignations.getDisplaceFromPath(path)
            if not shader:
                shader = self.mainAssignationsFromFile.getDisplaceFromPath(path)
        else:
            shader = self.layers.getDisplaceFromPath(path, layer)
            if not shader:
                shader = self.layersFromFile.getDisplaceFromPath(path, layer)

        return shader

    def getOverrides(self, path, layer, onlyInherited=False, layerOnly=False):
        overrides = {}

        if not layerOnly:
            # get the overrides from the main file, main layer.
            overridesMainFile = {}
            if layer != None:
                # if we are not on the main layer, we need anything from the main layer.
                overridesMainFile = self.mainAssignationsFromFile.getOverridesFromPath(path, onlyInherited=False)
                # and we tag them as inherited
                for attr in overridesMainFile:
                    overridesMainFile[attr]["inherited"] = True
            else:
                overridesMainFile = self.mainAssignationsFromFile.getOverridesFromPath(path, onlyInherited=onlyInherited)
            for attr in overridesMainFile:
                overrides[attr] = overridesMainFile[attr]

            # Then the ones from the direct assignation.
            overridesMain = {}
            if layer != None:
                overridesMain = self.mainAssignations.getOverridesFromPath(path, onlyInherited=False)
                # and we tag them as inherited
                for attr in overridesMain:
                    overridesMain[attr]["inherited"] = True                
            else:
                overridesMain = self.mainAssignations.getOverridesFromPath(path, onlyInherited=onlyInherited)

            for attr in overridesMain:
                overrides[attr] = overridesMain[attr]            

        if layer != None:
            # The upper, possible overrides come from layer from file.
            overridesLayerFile = self.layersFromFile.getOverridesFromPath(path, layer, onlyInherited=onlyInherited)
            for attr in overridesLayerFile:
                overrides[attr] = overridesLayerFile[attr]
                
            # Then possibly the direct layer overrides
            overridesLayer = self.layers.getOverridesFromPath(path, layer, onlyInherited=onlyInherited)
            for attr in overridesLayer:
                overrides[attr] = overridesLayer[attr]

        return overrides


    def getLayerOverrides(self, layer):
        layerOverrides = None
        layerOverrides = self.layers.getLayerOverrides(layer)
        if not layerOverrides:
            layerOverrides = self.layersFromFile.getLayerOverrides(layer)
        return layerOverrides


    def setRemovedShader(self, layer, state):
        self.layers.setRemovedShader(layer, state)

    def setRemovedDisplace(self, layer, state):
        self.layers.setRemovedDisplace(layer, state)

    def setRemovedProperties(self, layer, state):
        self.layers.setRemovedProperties(layer, state)


    def getPropertyState(self, layer, propName, curPath):

        # get the override
        attributes = self.getOverrides(curPath, layer)
        if propName in attributes:
            fromFile = attributes[propName].get("fromfile", False)
            inherited = attributes[propName].get("inherited", False)
            fromlayer = attributes[propName].get("fromlayer", None)

            if fromlayer != layer:
                return 1

            if fromFile:
                return 3
            if inherited:
                return 1

            return 2

        return 0


    def updateOverride(self, propName, default, value, curPath, layer):
        ''' Update overrides'''
        valueInherited = None
        attributes = self.getOverrides(curPath, layer, onlyInherited=True)

        if propName in attributes:
            fromFile = attributes[propName].get("fromfile", False)
            inherited = attributes[propName].get("inherited", False)

            if fromFile or inherited:
                valueInherited = attributes[propName].get("override", value)


        if valueInherited != None:
            if valueInherited != value:
                default = False
            else:
                default = True

        if layer is not None:                
            if default:
                self.layers.removeOverride(layer, curPath, propName)
            else:
                self.layers.updateOverride(propName, default, value, curPath, layer)
        else:
            if default:
                self.mainAssignations.removeOverride(curPath, propName)
            else:
                self.mainAssignations.updateOverride(propName, default, value, curPath)

            self.parent.updateOverrides(self.mainAssignations.getOverrides())

    def assignShader(self, layer, path, shader):
        if layer == None:
            self.mainAssignations.assignShader(path, shader)
            self.parent.updateShaders(self.mainAssignations.getShaders())
        else:
            self.layers.assignShader(layer, path, shader)

        self.parent.updateConnections()



    def assignDisplacement(self, layer, path, shader):
        if layer == None:
            self.mainAssignations.assignDisplacement(path, shader)
            self.parent.updateDisplacements(self.mainAssignations.getDisplacements())
        else:
            self.layers.assignDisplacement(layer, path, shader)

        self.parent.updateConnections()

    def removeShader(self, shader):
        ''' remove a shader '''
        self.mainAssignations.removeShader(shader)
        self.layers.removeShader(shader)

        self.parent.updateShaders(self.mainAssignations.getShaders())
        self.parent.updateConnections()

    def removeDisplacement(self, shader):
        ''' remove a displacement shader '''
        self.mainAssignations.removeDisplacement(shader) 
        self.layers.removeDisplacement(shader)

        self.parent.updateDisplacements(self.mainAssignations.getDisplacements())
        self.parent.updateConnections()

    def renameShader(self, oldname, newname):
        ''' rename a shader '''
        self.mainAssignations.renameShader(oldname, newname)
        self.layers.renameShader(oldname, newname)

        self.parent.updateShaders(self.mainAssignations.getShaders())
        self.parent.updateConnections()

    def renameDisplacement(self, oldname, newname):
        ''' rename a displacement shader '''
        self.mainAssignations.renameDisplacement(oldname, newname)
        self.layers.renameDisplacement(oldname, newname)
        
        self.parent.updateDisplacements(self.mainAssignations.getDisplacements())
        self.parent.updateConnections()        
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

from shaderManager.layers._layer import layer


class Layers(object):
    def __init__(self, parent=None, fromFile = False):
        self.parent = parent
        self.fromFile = fromFile
        self.layers = {}

    def addLayer(self, layername, layerdata):
        if not layername in self.layers:
            self.layers[layername] = layer(self, layername, self.fromFile)
            if "shaders" in layerdata:
                self.layers[layername].addShaders(layerdata["shaders"])
            if "properties" in layerdata:
                self.layers[layername].addOverrides(layerdata["properties"])
            if "displacements" in layerdata:
                self.layers[layername].addDisplacements(layerdata["displacements"])

            if "removeShaders" in layerdata:
                self.layers[layername].setRemovedShader(layerdata["removeShaders"])
            if "removeProperties" in layerdata:
                self.layers[layername].setRemovedProperties(layerdata["removeProperties"])
            if "removeDisplacements" in layerdata:
                self.layers[layername].setRemovedDisplace(layerdata["removeDisplacements"])

    def addLayers(self, layers, fromfile=False):
        for layer in layers:
            self.addLayer(layer, layers[layer])

    def  getShaderFromPath(self, path, layer):
        if layer in self.layers:
            return self.layers[layer].getAssignation().getShaderFromPath(path)
        return None

    def  getDisplaceFromPath(self, path, layer):
        if layer in self.layers:
            return self.layers[layer].getAssignation().getDisplaceFromPath(path)
        return None

    def getOverridesFromPath(self, path, layer, onlyInherited=False):
        if layer in self.layers:
            return self.layers[layer].getAssignation().getOverridesFromPath(path, onlyInherited=onlyInherited)
        return {}

    def getLayerOverrides(self, layer):
        if layer in self.layers:
            return self.layers[layer].getLayerOverrides()
        return None

    def setRemovedShader(self, layer, state):
        if layer in self.layers:
            self.layers[layer].setRemovedShader(state)
        else:
            self.addLayer(layer, dict(removeShaders=state))

    def setRemovedDisplace(self, layer, state):
        if layer in self.layers:
            self.layers[layer].setRemovedDisplace(state)
        else:
            self.addLayer(layer, dict(removeDisplacements=state))

    def setRemovedProperties(self, layer, state):
        if layer in self.layers:
            self.layers[layer].setRemovedProperties(state)
        else:
            self.addLayer(layer, dict(removeProperties=state))

    def getOverrideValue(self, layer, path, propname):
        if layer in self.layers:
            return self.layers[layer].getOverrideValue(path, propname)
        return None


    def updateOverride(self, propName, default, value, curPath, layer):
        if layer in self.layers:
            self.layers[layer].updateOverride(propName, default, value, curPath)
            self.writeLayer()
        else:
            if not default:
                properties = {}
                properties[curPath]={}
                properties[curPath][propName] = value
                self.addLayer(layer, dict(properties=properties))
                self.writeLayer()


    def removeShader(self, shader):
        ''' remove a shader '''
        for layer in self.layers:
            self.layers[layer].assignation.removeShader(shader)

    def removeDisplacement(self, shader):
        ''' remove a displacement shader '''
        for layer in self.layers:
            self.layers[layer].assignation.removeDisplacement(shader) 

    def renameShader(self, oldname, newname):
        ''' rename a shader '''
        for layer in self.layers:
            self.layers[layer].assignation.renameShader(oldname, newname)

    def renameDisplacement(self, oldname, newname):
        ''' rename a displacement shader '''
        for layer in self.layers:
            self.layers[layer].assignation.renameDisplacement(oldname, newname)

    def assignShader(self, layer, path, shader):
        if layer in self.layers:
            self.layers[layer].assignShader(path, shader)
        else:
            shaders = {}
            shaders[shader] = [path]
            self.addLayer(layer, dict(shaders=shaders))

        self.writeLayer()



    def assignDisplacement(self, layer, path, shader):
        if layer in self.layers:
            self.layers[layer].assignDisplacement(path, shader)
        else:
            shaders = {}
            shaders[shader] = [path]
            self.addLayer(layer, dict(displacements=shaders))

        self.writeLayer()


    def removeOverride(self, layer, curPath, propName):
        if layer in self.layers:
            self.layers[layer].removeOverride(curPath, propName)
            self.writeLayer()

    def writeLayer(self):
        self.parent.writeLayer()

    def getLayerDict(self):
        result = {}
        for layer in self.layers:
            curlayer = self.layers[layer]
            result[layer] = {}
            result[layer]["removeShaders"] = curlayer.removeShaders
            result[layer]["removeDisplacements"] = curlayer.removeDisplacements
            result[layer]["removeProperties"] = curlayer.removeProperties
            result[layer]["properties"] = curlayer.getOverrides()
            result[layer]["shaders"] = curlayer.getShaders()
            result[layer]["displacements"] = curlayer.getDisplacements()

        return result

    def getShaders(self):
        shaders = []
        for layer in self.layers:
            shaders += self.layers[layer].assignation.getShaders().keys()
        return shaders

    def getDisplacements(self):
        shaders = []
        for layer in self.layers:
            shaders += self.layers[layer].assignation.getDisplacements().keys()
        return shaders

    def getWildCards(self):
        wilds = []
        for layer in self.layers:
            wilds += self.layers[layer].assignation.getWildCards()
        return wilds        
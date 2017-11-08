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
from Qt import QtCore

import json
import maya.cmds as cmds

import pymel.core as pm

from imath import *
from alembic.Abc import *
from alembic.AbcGeom import *
from alembic.AbcCoreAbstract import *
from alembic.AbcCollection import *

class gpucache(object):
    def __init__(self, shape, parent=None):
        self.parent = parent
        self.shape = shape
        self.cache = None
        self.ABCcache = ""
        self.archive = None
        self.assignations = cacheAssignations(self)

        self.curPath = ""
        self.tags = {}
        self.itemsTree = []

    def isValid(self):
        ''' check if this cache is still valid'''
        return cmds.objExists(self.shape)


    def getNamedObj(self, iObjTop, objectPath=""):
        nextChildHeader = iObjTop.getHeader()
        nextParentObject = iObjTop

        for childName in objectPath.split("/"):
            if childName == "":
                continue
            nextChildHeader = nextParentObject.getChildHeader(str(childName))
            if not nextChildHeader:
                break
            else:
                nextParentObject = IObject( nextParentObject, str(childName))

        if ( nextChildHeader ):
            iObjTop = nextParentObject
            return iObjTop

        return None

    def getHierarchy(self, objectPath="/"):
        results = []
        if self.archive:

            archiveTop = self.archive.getTop()
            iObj = self.getNamedObj(archiveTop, objectPath)
            if iObj == None:
                return None

            if iObj:

                for i in range(iObj.getNumChildren()):
                    oType = "Unknown"
                    child = iObj.getChild(i)
                    if IXform.matches(child.getHeader()):
                        oType = "Transform"
                    elif IPoints.matches(child.getHeader()):
                        oType = "Points"
                    elif IPolyMesh.matches(child.getHeader()):
                        oType = "Shape"
                    elif ICurves.matches(child.getHeader()):
                        oType = "Curves"
                    elif ILight.matches(child.getHeader()):
                        oType = "Light"
                        light = ILight(child.getParent(), child.getName())
                        ps = light.getSchema()
                        arbGeomParams = ps.getArbGeomParams()
                        lightTypeHeader = arbGeomParams.getPropertyHeader("light_type")
                        if IInt32GeomParam.matches( lightTypeHeader ):
                            param = IInt32GeomParam( arbGeomParams,  "light_type")
                            if param.valid():
                                if ( param.getScope() == GeometryScope.kConstantScope or param.getScope() == GeometryScope.kUnknownScope):
                                    valueSample = param.getExpandedValue().getVals()
                                    lightType = valueSample[0]
                                    if lightType == 0:
                                        oType = "DistantLight"
                                    elif lightType == 1:
                                        oType = "PointLight"
                                    elif lightType == 2:
                                        oType = "SpotLight"                                                                                
                                    elif lightType == 3:
                                        oType = "QuadLight"
                                    elif lightType == 4:
                                        oType = "PhotometricLight"
                                    elif lightType == 5:
                                        oType = "DiskLight"
                                    elif lightType == 6:
                                        oType = "CylinderLight"                                                                                
                    elif ICollections.matches(child.getHeader()):
                        oType = "Collections"
                    
                    results.append(dict(type=oType, name=child.getName()))
                    

        return results

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
            self.ABCcache =  str(cmds.getAttr("%s.cacheFileName" % self.shape) or "")
            self.ABCcurPath = str(cmds.getAttr("%s.cacheGeomPath" % self.shape) or "")
            if self.ABCcache:
                self.archive = IArchive(self.ABCcache)


    def getShadersAttributes(self):
        results = []
        if self.archive:

            archiveTop = self.archive.getTop()
            iObj = self.getShaderAttribute(archiveTop, objectPath)
            if iObj == None:
                return None

    def visitShaderAttributeAndTags(self, iObj, allowedTags, results):
        if not iObj.valid():
            return
        ohead = iObj.getHeader()
        arbGeomParams = None
        if IXform.matches(ohead):
            arbGeomParams = IXform(iObj, WrapExistingFlag.kWrapExisting).getSchema().getArbGeomParams()
        if IPolyMesh.matches(ohead):
            arbGeomParams = IPolyMesh(iObj, WrapExistingFlag.kWrapExisting).getSchema().getArbGeomParams()
        if ISubD.matches(ohead):
            arbGeomParams = ISubD(iObj, WrapExistingFlag.kWrapExisting).getSchema().getArbGeomParams()
        if ICurves.matches(ohead):
            arbGeomParams = ICurves(iObj, WrapExistingFlag.kWrapExisting).getSchema().getArbGeomParams()
        if IPoints.matches(ohead):
            arbGeomParams = IPoints(iObj, WrapExistingFlag.kWrapExisting).getSchema().getArbGeomParams()                

        if arbGeomParams is not None and arbGeomParams.valid():
            for attrName in allowedTags:
                propHeader = arbGeomParams.getPropertyHeader(attrName)
                if(propHeader is not None):
                     if (IStringGeomParam.matches( propHeader )):
                        param = IStringGeomParam( arbGeomParams, attrName )
                        if param.valid():
                            val = param.getExpandedValue().getVals()[0]
                            try:
                                for tag in json.loads(val):
                                    if not tag in results:
                                        results[tag] = [iObj.getFullName()]
                                    else:
                                        results[tag].append(iObj.getFullName())
                            except:
                                if not val in results:
                                    results[val] = [iObj.getFullName()]
                                else:
                                    results[val].append(iObj.getFullName())

        for i in range(iObj.getNumChildren()):
            self.visitShaderAttributeAndTags(IObject(iObj, iObj.getChildHeader(i).getName() ), allowedTags, results)

    '''
    Update all the tags available in the current cache.
    '''
    def updateTags(self):
        if not self.archive:
            return
        aTop = self.archive.getTop()
        allowedTags = ["mtoa_constant_tags", "tags"]
        shAttr = str(cmds.getAttr(self.shape + ".shadersAttribute"))
        if shAttr != "":
            allowedTags.append(shAttr)
        self.tags = {}
        self.visitShaderAttributeAndTags(aTop, allowedTags, self.tags)



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
            try:
                topath = json.loads(topath)
                topath = topath[0].replace("/", "|")
                cmds.setAttr("%s.cacheGeomPath" % self.shape, topath, type="string")
            except:
                cmds.setAttr("%s.cacheGeomPath" % self.shape, "", type="string")
        # for item in self.itemsTree:
        #     if item.getPath() != topath:
        #         item.setCheckState(0, QtCore.Qt.Unchecked)

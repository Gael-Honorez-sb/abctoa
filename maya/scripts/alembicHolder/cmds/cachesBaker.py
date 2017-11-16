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

import maya.cmds as cmds
import json
import os

def getHolder(x):
    ''' return the shape of an holder from the shape or the group'''

    shape = None
    if cmds.nodeType(x) == "alembicHolder":
        shape = x
    else:
        shapes = cmds.listRelatives(x, shapes=True)
        if shapes:
            shape = shapes[0]
    return shape


def applyAssignations():
    ''' select a json file, and apply it to a alembicHolder '''
    
    tr = cmds.ls( type= 'transform', sl=1) + cmds.ls(type= 'alembicHolder', sl=1)
    
    if len(tr) == 0:
        return
            
    singleFilter = "Json Files (*.json)"
    jsonfiles = cmds.fileDialog2(fileMode=1, caption="Import Json", fileFilter=singleFilter)
    if len(jsonfiles) > 0:
        jsonfile = jsonfiles[0]
    else:
        return

    if os.path.exists(jsonfile):
        shaderFile = None
        potentialFile = os.path.join(os.path.dirname(jsonfile), "shaders_%s" % os.path.basename(jsonfile)).replace(".json", ".abc")
        if os.path.exists(potentialFile):
            shaderFile = potentialFile

        for x in tr:
            shape = getHolder(x)
            if not shape:
                continue

            cmds.setAttr("%s.jsonFile" % shape, jsonfile, type="string")
            if shaderFile:
                cmds.setAttr("%s.abcShaders" % shape, shaderFile, type="string")


def writeAbcShaders():
    ''' export all shaders connected to a holder in abc '''

    cmds.loadPlugin('abcMayaShader.mll', qt=1)

    tr = cmds.ls( type= 'transform', sl=1) + cmds.ls(type= 'alembicHolder', sl=1)
    
    if len(tr) == 0:
        return

    for x in tr:
        shape = getHolder(x)
        if not shape:
            continue

        cache = cmds.getAttr("%s.cacheFileNames[0]" % shape)
        abcfile = os.path.join(os.path.dirname(cache), "shaders_%s" % os.path.basename(cache))
        try:
            cmds.abcCacheExport(f=abcfile, node=shape)
        except Exception, e:
            print "Failed to export Abc Shaders: %s" % e

def writeJson(namespace=None):
    '''Write a json file next to the selected alembicHolders cache containing the current assignations.

    Keyword arguments:
    namespace -- When looking for shaders in the procedural, look/use this namespace.
    '''

    tr = cmds.ls( type= 'transform', sl=1) + cmds.ls(type= 'alembicHolder', sl=1)

    if len(tr) == 0:
        return
    
    for x in tr:
        shape = getHolder(x)
        if not shape:
        	continue

        if cmds.nodeType(shape) == "gpuCache" or cmds.nodeType(shape) == "alembicHolder":

            cache = cmds.getAttr("%s.cacheFileNames[0]" % shape)
            jsonfile = cache.replace(".abc", ".json")

            assignations = {}

            if cmds.objExists("%s.shadersAssignation" % shape):
                try:
                    cur = cmds.getAttr("%s.shadersAssignation"  % shape)
                    assignations["shaders"] = json.loads(cur)
                except:
                    pass

            if cmds.objExists("%s.attributes" % shape):
                try:
                    cur = cmds.getAttr("%s.attributes"  % shape)
                    assignations["attributes"] = json.loads(cur)
                except:
                    pass

            if cmds.objExists("%s.displacementsAssignation" % shape):
                try:
                    cur = cmds.getAttr("%s.displacementsAssignation"  % shape)
                    assignations["displacement"] = json.loads(cur)
                except:
                    pass

            if cmds.objExists("%s.layersOverride" % shape):
                try:
                    cur = cmds.getAttr("%s.layersOverride"  % shape)
                    assignations["layers"] = json.loads(cur)
                except:
                    pass

            if cmds.objExists("%s.shadersAttribute" % shape):
                try:
                    cur = cmds.getAttr("%s.shadersAttribute"  % shape)
                    if cur != "":
                        assignations["shadersAttribute"] = cur
                except:
                    pass

            if namespace:
                assignations["namespace"] = namespace
            else :
                assignations["namespace"] = shape.replace(":", "_").replace("|", "_")

            try:
                with open(jsonfile, 'w') as outfile:
                    json.dump(assignations, outfile, separators=(',',':'), sort_keys=True, indent=4)
            except:
                print "Error writing json file", outfile
                
                

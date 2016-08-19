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


def applyRules(name):
    '''
    Apply a rule to find a shader matching a certain name.
    Return the shader, or None if no shader is Found.
    '''
    bestShaderSoFar = None
    shaders = cmds.ls(type="shadingEngine")
    for shader in shaders:
        if name in shader:
            if bestShaderSoFar is None:
                # no shader found yet, we take this one.
                bestShaderSoFar = shader
                continue
            
            # we can assume, once the namespace are removed, the smaller the shader name is,
            # the closer it is to the name.
            shortShaderName = shader.split(":")[-1]
            shortBestShaderName = shader.split(":")[-1]
            if len(shortShaderName) < shortBestShaderName:
                bestShaderSoFar = shader

    if bestShaderSoFar is None:
        # We didn't find anything we try again but this time, we remove any "_SHDSG" from the name of the shader.
        if "_SHDSG" in name :
            bestShaderSoFar = applyRules(name.replace("_SHDSG", ""))

    return bestShaderSoFar
# Alembic Holder
# Copyright (c) 2014, Gaël Honorez, All rights reserved.
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

def writeJson(namespace=None):
	'''Write a json file next to the selected alembicHolders cache containing the current assignations.

	Keyword arguments:
	namespace -- When looking for shaders in the procedural, look under this namespace.
	'''

	tr = cmds.ls( type= 'transform', sl=1) + cmds.ls(type= 'alembicHolder', sl=1)
	
	if len(tr) == 0:
	    return
	
	for x in tr:
	    if cmds.nodeType(x) == "alembicHolder":
	        shape = x
	    else:
	        shapes = cmds.listRelatives(x, shapes=True)
	        if shapes:
	            shape = shapes[0]

	    if cmds.nodeType(shape) == "gpuCache" or cmds.nodeType(shape) == "alembicHolder":

	    	cache = cmds.getAttr("%s.cacheFileName" % shape)
	    	jsonfile = cache.replace(".abc", ".json")

	    	assignations = {}

	    	if cmds.objExists("%s.mtoa_constant_shaderAssignation" % shape):
	    		try:
	    			cur = cmds.getAttr("%s.mtoa_constant_shaderAssignation"  % shape)
	    			assignations["shaders"] = json.loads(cur)
	    		except:
	    			pass

	    	if cmds.objExists("%s.mtoa_constant_overrides" % shape):
	    		try:
		    		cur = cmds.getAttr("%s.mtoa_constant_overrides"  % shape)
		    		assignations["overrides"] = json.loads(cur)
	    		except:
	    			pass

	    	if cmds.objExists("%s.mtoa_constant_displacementAssignation" % shape):
	    		try:
		    		cur = cmds.getAttr("%s.mtoa_constant_displacementAssignation"  % shape)
		    		assignations["displacement"] = json.loads(cur)
	    		except:
	    			pass

	    	if cmds.objExists("%s.mtoa_constant_layerOverrides" % shape):
	    		try:
		    		cur = cmds.getAttr("%s.mtoa_constant_layerOverrides"  % shape)
		    		assignations["layers"] = json.loads(cur)
	    		except:
	    			pass

	    	if namespace:
	    		assignations["namespace"] = namespace

	    	try:
	    		with open(jsonfile, 'w') as outfile:
		    		json.dump(assignations, outfile, separators=(',',':'), sort_keys=True, indent=4))
			except:
				print "Error writing json file", outfile
	    		
	    		

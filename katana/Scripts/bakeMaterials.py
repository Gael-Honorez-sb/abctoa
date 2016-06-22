import alembic
from alembic.AbcCoreAbstract import *
from alembic.Abc import *
from alembic.Util import *
from alembic.AbcGeom import *
from alembic.AbcMaterial import *
import imath
import json

def bakeMaterials(alembicFile, jsonfile):

    archive = IArchive(alembicFile)
    root = archive.getTop()

    e_materials = []

    materials = IObject(root, "materials")
    for i in range(materials.getNumChildren()):
        matName = materials.getChildHeader(i).getName()
        e_mat = dict(name=matName)
        matOO = materials.getChild(i)
        matObj = IMaterial( materials, matOO.getName() ) 
        
        for targetName in matObj.getSchema().getNetworkTerminalTargetNames():
            shaderTypeNames = matObj.getSchema().getNetworkTerminalShaderTypesForTarget(targetName)
            if targetName == "arnold":
                for shaderType in shaderTypeNames:
                    terminal =  matObj.getSchema().getNetworkTerminal(targetName, shaderType)
                    e_mat["terminal"] = terminal
        
        if matObj.getSchema().getNumNetworkInterfaceParameterMappings() != 0:
            e_mat["mapping"] = []
            mappingNames = matObj.getSchema().getNetworkInterfaceParameterMappingNames()
           
            for mapName in mappingNames:

                mapping = matObj.getSchema().getNetworkInterfaceParameterMapping(mapName)
                mapping["interfaceName"] = mapName
                e_mat["mapping"].append(mapping)
        
        nodes = []
        for i in range(matObj.getSchema().getNumNetworkNodes()):
            node = matObj.getSchema().getNetworkNode(i)
            target = node.getTarget()
            nodeType = node.getNodeType() 
            n = dict(name=node.getName(), type=nodeType, target=target)  
            n["connections"] = []
            numConnections = node.getNumConnections()
            if (numConnections):
                for c in range(numConnections):
                      n["connections"].append(node.getConnection(c))
                  
            parameters =  node.getParameters()
            params = []
            for p in range(parameters.getNumProperties()):
                param = dict(name = str(parameters.getProperty(p)))
                prop = parameters.getProperty(p)
                val = prop.getValue()
                param["val"] = val
                if type(val) == imath.V2f:
                    param["type"] = "vector2f"
                    param["val"] = [val.x, val.y]
                elif type(val) == imath.V3f: 
                    param["type"] = "vector3f"
                    param["val"] = [val.x, val.y, val.z]
                elif type(val) == imath.Color3f: 
                    param["type"] = "color3f"
                    param["val"] = [val.r, val.g, val.b]
                elif type(val) == imath.Color4f: 
                    param["type"] = "color4f"
                    param["val"] = [val.r, val.g, val.b, val.a]
                elif type(val) == imath.StringArray: 
                    param["type"] = "stringArray"
                    param["val"]=[]
                    for v in val:
                        param["val"].append(v)
                elif type(val) == imath.FloatArray: 
                    param["type"] = "floatArray"
                    param["val"]=[]
                    for v in val:
                        param["val"].append(v)
                elif type(val) == imath.IntArray: 
                    param["type"] = "intArray"
                    param["val"]=[]                        
                elif type(val) == str: 
                    param["type"] = "string"                
                elif type(val) == bool: 
                    param["type"] = "bool"
                elif type(val) == int: 
                    param["type"] = "integer"
                elif type(val) == float: 
                    param["type"] = "float"
                 
                else:
                    print type(val)
                params.append(param)
            n["params"]=params
            nodes.append(n)

        e_mat["nodes"] = nodes
        e_materials.append(e_mat)
        
    with open(jsonfile, 'w') as outfile:
        json.dump(e_materials, outfile, separators=(',',':'), sort_keys=True, indent=4)   


import json

def reOrderNode(node, xPos, yPos, conns, createdNodes):
    if node in conns :
        yPos+=100
        xOffset = False
        if len(conns[node]) > 1:
            xPos -= 150 * (len(conns[node]) -1 )
            xOffset = True
            
        for con in conns[node]:
            otherNode = createdNodes[con]
            NodegraphAPI.SetNodePosition(otherNode, (xPos, yPos)) 
            reOrderNode(con,xPos, yPos, conns, createdNodes)  
            if xOffset:
                xPos += 300  


def importMaterials(jsonfile):

    with open(jsonfile, 'r') as infile:
        e_materials = json.load(infile)


    root = NodegraphAPI.GetRootNode()
    createdNodes = {}
    xPos = 200
    xPosGroup = 0
    xPosGroupOffset = 200
    yPos = 0

    mergeNode = NodegraphAPI.CreateNode('Merge', root)

    mergeNode.setName(os.path.basename(jsonfile))
    DrawingModule.SetCustomNodeColor(mergeNode, 0.35, 0.25, 0.38)

    LookFileMaterialsOut = NodegraphAPI.CreateNode('LookFileMaterialsOut', root)
    rootf, ext = os.path.splitext(jsonfile)
    LookFileMaterialsOut.getParameter('saveTo').setValue(rootf +".kif", 0)
    DrawingModule.SetCustomNodeColor(LookFileMaterialsOut, 0.15, 0.25, 0.28)

    for mat in e_materials:

        xPos = 0     
        name =str(mat["name"])
        
        groupNode = NodegraphAPI.CreateNode('Group', root)
        groupNode.setName(name)
        groupNode.addOutputPort("out")

        DrawingModule.SetCustomNodeColor(groupNode, 0.2, 0.275, 0.40)

        NodegraphAPI.SetNodePosition(groupNode, (xPosGroup, 0)) 
       
       
        xPosGroup += xPosGroupOffset
        for node in mat["nodes"]:
            nodetype = str(node["type"])
            nodename = str(node["name"])
            
            arnoldNode = NodegraphAPI.CreateNode('ArnoldShadingNode', groupNode)
            DrawingModule.SetCustomNodeColor(arnoldNode, 0.2, 0.275, 0.40)
            NodegraphAPI.SetNodePosition(arnoldNode, (xPos, 0))
            arnoldNode.getParameter('name').setValue(nodename, 0)
            arnoldNode.getParameter('nodeType').setValue(nodetype, 0)
            arnoldNode.checkDynamicParameters()
            createdNodes[nodename] = arnoldNode


            for param in node["params"]:
                paramAttr = 'parameters.%s' % param["name"]
                paramValue = param["val"]
                paramType = param["type"]
                paramNode = arnoldNode.getParameter(paramAttr)
                if not paramNode:
                    continue
                paramKValue = arnoldNode.getParameter( 'parameters.%s.value' % param["name"])
                paramKEnable = arnoldNode.getParameter( 'parameters.%s.enable' % param["name"])
                if paramType == "vector2f":
                    if ( paramKValue.getChildByIndex(0).getValue(0) != paramValue[0] or paramKValue.getChildByIndex(1).getValue(0) != paramValue[1] ) :
                        paramKEnable.setValue(1,0)
                        paramKValue.getChildByIndex(0).setValue(str(paramValue[0]),0)
                        paramKValue.getChildByIndex(1).setValue(str(paramValue[1]),0)

                elif paramType == "vector3f":
                    if ( paramKValue.getChildByIndex(0).getValue(0) != paramValue[0] or paramKValue.getChildByIndex(1).getValue(0) != paramValue[1] or paramKValue.getChildByIndex(2).getValue(0) != paramValue[2] ) :
                        paramKEnable.setValue(1,0)
                        paramKValue.getChildByIndex(0).setValue(str(paramValue[0]),0)
                        paramKValue.getChildByIndex(1).setValue(str(paramValue[1]),0)
                        paramKValue.getChildByIndex(2).setValue(str(paramValue[2]),0)

                elif paramType == "vector4f":
                    if ( paramKValue.getChildByIndex(0).getValue(0) != paramValue[0] or 
                         paramKValue.getChildByIndex(1).getValue(0) != paramValue[1] or 
                         paramKValue.getChildByIndex(2).getValue(0) != paramValue[2] or 
                         paramKValue.getChildByIndex(3).getValue(0) != paramValue[3] ) :
                        paramKEnable.setValue(1,0)
                        paramKValue.getChildByIndex(0).setValue(str(paramValue[0]),0)
                        paramKValue.getChildByIndex(1).setValue(str(paramValue[1]),0)
                        paramKValue.getChildByIndex(2).setValue(str(paramValue[2]),0)
                        paramKValue.getChildByIndex(3).setValue(str(paramValue[3]),0)
                elif paramType == "color4f":
                    if ( paramKValue.getChildByIndex(0).getValue(0) != paramValue[0] or 
                         paramKValue.getChildByIndex(1).getValue(0) != paramValue[1] or 
                         paramKValue.getChildByIndex(2).getValue(0) != paramValue[2] or 
                         paramKValue.getChildByIndex(3).getValue(0) != paramValue[3] ) :
                        paramKEnable.setValue(1,0)
                        paramKValue.getChildByIndex(0).setValue(str(paramValue[0]),0)
                        paramKValue.getChildByIndex(1).setValue(str(paramValue[1]),0)
                        paramKValue.getChildByIndex(2).setValue(str(paramValue[2]),0)
                        paramKValue.getChildByIndex(3).setValue(str(paramValue[3]),0)
                elif paramType == "color3f":
                    if ( paramKValue.getChildByIndex(0).getValue(0) != paramValue[0] or paramKValue.getChildByIndex(1).getValue(0) != paramValue[1] or paramKValue.getChildByIndex(2).getValue(0) != paramValue[2] ) :
                        paramKEnable.setValue(1,0)
                        paramKValue.getChildByIndex(0).setValue(str(paramValue[0]),0)
                        paramKValue.getChildByIndex(1).setValue(str(paramValue[1]),0)
                        paramKValue.getChildByIndex(2).setValue(str(paramValue[2]),0)
                elif paramType == "stringArray":
                    paramKEnable.setValue(1,0)
                    for i in range(len(paramValue)):
                        paramKValue.getChildByIndex(i).setValue(str(paramValue[i]),0)

                elif paramType == "intArray":
                    paramKEnable.setValue(1,0)
                    for i in range(len(paramValue)):
                        paramKValue.getChildByIndex(i).setValue(str(paramValue[i]),0)

                elif paramType == "floatArray":
                    paramKEnable.setValue(1,0)
                    for i in range(len(paramValue)):
                        paramKValue.getChildByIndex(i).setValue(str(paramValue[i]),0)

                else: 
                    valueInK = paramKValue.getValue(0)
                    if paramValue != valueInK:
                        paramKEnable.setValue(1,0)
                        if paramType == "integer" or paramType == "string":
                            paramValue = str(paramValue)
                        paramKValue.setValue(paramValue,0)

        # now we connect the nodes
        conns = {}
        for node in mat["nodes"]:
            for con in node["connections"]:
                inputPortName = str(con["inputName"])
                outputPortName = str(con ["connectedOuputName"])
                if outputPortName == "":
                    outputPortName = "out"
                else:
                    outputPortName = "out.%s" % outputPortName
                outputNode = con["connectedNodeName"]       
            
                outPort = createdNodes[outputNode].getOutputPort(outputPortName)
                createdNodes[node["name"]].getInputPort(inputPortName).connect(outPort)
                if not node["name"] in conns:
                    conns[node["name"]] = [outputNode]
                else:
                    if not outputNode in conns[node["name"]]:
                        conns[node["name"]].append(outputNode)
                #_ , otherNodePos = NodegraphAPI.GetNodePosition(createdNodes[outputNode])
                #print otherNodePos

                #NodegraphAPI.SetNodePosition(createdNodes[node["name"]], (xPos, otherNodePos+100))

        terminalNode = mat["terminal"]
        terminalNodeName = terminalNode["nodeName"]
        reOrderNode(terminalNodeName, xPos, yPos, conns, createdNodes)

        # and now the terminal                
        networkMaterialNode = NodegraphAPI.CreateNode('NetworkMaterial', groupNode)
        DrawingModule.SetCustomNodeColor(networkMaterialNode, 0.14, 0.10, 0.40)

        NodegraphAPI.SetNodePosition(networkMaterialNode, (xPos, -100))
        networkMaterialNode.addInputPort('arnoldSurface')
        networkMaterialNode.getInputPort('arnoldSurface').connect(createdNodes[terminalNodeName].getOutputPort('out'))
        networkMaterialNode.getParameter('name').setValue(str(name), 0)
        networkMaterialNode.getParameter('namespace').setValue(str(os.path.basename(rootf).replace("-", "_")), 0)

        networkMaterialNode.getOutputPort('out').connect(groupNode.getReturnPort('out'))

        mergeNode.addInputPort('i0').connect(groupNode.getOutputPort('out'))

        for map in mat["mapping"]:
            paramName = "parameters.%s" % map["mapToParamName"]
            param = createdNodes[map["mapToNodeName"]].getParameter(paramName)
            hintsParam = param.getChild('hints')
            if not hintsParam:
                param.createChildString('hints', '')
                hintsParam = param.getChild('hints')
            page, name = map["interfaceName"].split(":")
            hints = dict(dstName = str(page) + "_" + str(name), dstPage = str(page))
            hintsParam.setValue(repr(hints), 0)
            

        xPos += 400

    NodegraphAPI.SetNodePosition(mergeNode, ((xPosGroup-xPosGroupOffset) / 2, -500))
    mergeNode.getOutputPort('out').connect(LookFileMaterialsOut.getInputPort('in'))
    NodegraphAPI.SetNodePosition(LookFileMaterialsOut, ((xPosGroup-xPosGroupOffset) / 2, -600))
    
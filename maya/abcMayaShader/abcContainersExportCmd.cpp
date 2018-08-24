/*Abc Shader Exporter
Copyright (c) 2014, Gaël Honorez, All rights reserved.
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 3.0 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public
License along with this library.*/


#include "abcContainersExportCmd.h"
#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include "abcExporterUtils.h"

#include <sstream>


//
// abcContainersExportCmd::doIt
//
//  This is the function used to invoke the command. The
// command is not undoable and it does not change any state,
// so it does not use the method to call back throught redoIt.
//////////////////////////////////////////////////////////////////////

namespace Abc =  Alembic::Abc;
namespace Mat = Alembic::AbcMaterial;


MStatus abcContainersExportCmd::doIt( const MArgList &args)
{
  MStatus stat = MStatus::kSuccess;
  MArgDatabase argData( syntax(), args);

  if(!argData.isFlagSet( "-f"))
  {
      MGlobal::displayError("no output file!");
      return MStatus::kSuccess;

  }
  MString filename("");
  argData.getFlagArgument( "-f", 0, filename);

  Abc::OArchive archive(Alembic::AbcCoreOgawa::WriteArchive(), filename.asChar() );
  Abc::OObject root(archive, Abc::kTop);
  Abc::OObject materials(root, "materials");

    MSelectionList list;
    if(argData.isFlagSet( "-ls"))
    {
        MGlobal::getActiveSelectionList (list);
    }
    else
    {
         MItDependencyNodes nodeIt;
         for (; !nodeIt.isDone(); nodeIt.next())
         {
            MObject node = nodeIt.item();
            if (node.isNull())
                continue;
            list.add (node);
         }
    }

    CMayaScene::Begin(MTOA_SESSION_ASS);
    CArnoldSession* arnoldSession = CMayaScene::GetArnoldSession();
    std::set<AtNode*> exportedNodes;
    MItSelectionList iter(list, MFn::kContainer);
     for (; !iter.isDone(); iter.next())
     {
         MObject dependNode;
         iter.getDependNode(dependNode);

         MFnContainerNode container(dependNode);
         //cout << "found a container" << endl;

         // create the material
         Mat::OMaterial matObj(materials, container.name().asChar());
         MObjectArray members;
         container.getMembers(members);
         MPlug toExport;
         // get the shading engine
         for (unsigned int j = 0; j < members.length(); j++)
         {

            MObject shaderNode = members[j];

            if(shaderNode.hasFn(MFn::kShadingEngine))
            {
                MFnDependencyNode fnDGNode(shaderNode);
                MPlugArray connections;
                MPlug shaderPlug = fnDGNode.findPlug("surfaceShader");
                shaderPlug.connectedTo(connections, true, false);
                if (connections.length() > 0)
                {
                    toExport = connections[0];
                }
                break;
            }

        }
         if(!toExport.isNull())
         {
             CNodeTranslator* translator = arnoldSession->ExportNode(toExport);
             
             if(true)
             {
                 AtNode* root = translator->GetArnoldNode();

                 exportedNodes.insert(root);
                 getAllArnoldNodes(root, exportedNodes);

                 std::set<AtNode*>::const_iterator sit (exportedNodes.begin()), send(exportedNodes.end());
                 for(;sit!=send;++sit)
                 {
                     // adding the node to the network
                     MString nodeName(container.name());
                     nodeName = nodeName + "_" + MString(AiNodeGetName(*sit));
                     matObj.getSchema().addNetworkNode(nodeName.asChar(), "arnold", AiNodeEntryGetName(AiNodeGetNodeEntry(*sit)));

                     if(root == *sit)
                     {
                         cout << "root node " << nodeName.asChar() << endl;
                         //TODO : get if it's a volume, eventually
                        matObj.getSchema().setNetworkTerminal(
                        "arnold",
                        "surface",
                        nodeName.asChar(),
                        "out");
                     }
                     cout << "added node " << nodeName.asChar() << endl;
                     //export parameters
                     AtParamIterator* nodeParam = AiNodeEntryGetParamIterator(AiNodeGetNodeEntry(*sit));
                     int outputType = AiNodeEntryGetOutputType(AiNodeGetNodeEntry(*sit));

                     while (!AiParamIteratorFinished(nodeParam))
                     {
                         const AtParamEntry *paramEntry = AiParamIteratorGetNext(nodeParam);
                         const char* paramName = AiParamGetName(paramEntry);

                         if(AiParamGetType(paramEntry) == AI_TYPE_ARRAY)
                         {
                             AtArray* paramArray = AiNodeGetArray(*sit, paramName);
                            // process array...
                             if(AiParamGetType(paramEntry) == AI_TYPE_ARRAY)
                             {
                                 AtArray* paramArray = AiNodeGetArray(*sit, paramName);

                                 processArrayValues(*sit, paramName, paramArray, outputType, matObj, nodeName, container.name());
                                 for(unsigned int i=0; i < AiArrayGetNumElements(paramArray); i++)
                                 {
                                     processArrayParam(*sit, paramName, paramArray, i, outputType, matObj, nodeName, container.name());
                                 }

                             }
                         }
                         else
                         {
                            processLinkedParam(*sit, AiParamGetType(paramEntry), outputType, matObj, nodeName, paramName, container.name());
                         }
                     }

                 }

            // now, we export all the interface thingies
            MStringArray publishedNames;
            MPlugArray publishedPlugs;
            container.getPublishedPlugs (publishedPlugs, publishedNames) ;
            if( publishedNames.length() >0)
            {
                for (unsigned i=0; i < publishedNames.length(); ++i)
                {

                    MPlug publishedPlug = publishedPlugs[i];
                    MString plugName = publishedPlug.name();

                    MStringArray parts;
                    plugName.split('.', parts);
                    AtNode* aNode = AiNodeLookUpByName(parts[0].asChar());


                    MString nodeName(container.name());
                    nodeName = nodeName + "_" + MString(AiNodeGetName(aNode));

                    // try to find the fucker
                    const AtNodeEntry* arnoldNodeEntry = AiNodeGetNodeEntry(aNode);
                    CBaseAttrHelper helper(arnoldNodeEntry);
                     AtParamIterator* nodeParam = AiNodeEntryGetParamIterator(AiNodeGetNodeEntry(aNode));

                     if (parts[1] == "fileTextureName")
                         parts[1] = "filename";

                     while (!AiParamIteratorFinished(nodeParam))
                     {
                         const AtParamEntry *paramEntry = AiParamIteratorGetNext(nodeParam);
                         const char* paramName = AiParamGetName(paramEntry);



                         if(helper.GetMayaAttrName(paramName) == parts[1].asChar())
                         {
                             cout << "Published name "<< publishedNames[i].asChar() << " for " << nodeName.asChar() << "." << paramName << endl;

                             matObj.getSchema().setNetworkInterfaceParameterMapping(publishedNames[i].asChar(), nodeName.asChar(), paramName);
                             break;
                         }

                     }

                }

            }

         }
        }
     }
  CMayaScene::End();

  return MStatus::kSuccess;
}


//  There is never anything to undo.
//////////////////////////////////////////////////////////////////////
MStatus abcContainersExportCmd::undoIt(){
  return MStatus::kSuccess;
}

//
//  There is never really anything to redo.
//////////////////////////////////////////////////////////////////////
MStatus abcContainersExportCmd::redoIt(){
  return MStatus::kSuccess;
}

//
//
//////////////////////////////////////////////////////////////////////
bool abcContainersExportCmd::isUndoable() const{
  return false;
}

//
//
//////////////////////////////////////////////////////////////////////
bool abcContainersExportCmd::hasSyntax() const {
  return true;
}

//
//
//////////////////////////////////////////////////////////////////////
MSyntax abcContainersExportCmd::mySyntax() {
  MSyntax syntax;

  //syntax.addFlag( "-ls", "-listShader");
  syntax.addFlag( "-sl", "-selection", MSyntax::kString);
  syntax.addFlag( "-f", "-file", MSyntax::kString);

  return syntax;
}

//
//
//////////////////////////////////////////////////////////////////////
bool abcContainersExportCmd::isHistoryOn() const {
  //what is this supposed to do?
  return false;
}

MString abcContainersExportCmd::commandString() const {
  return MString();
}


MStatus abcContainersExportCmd::setHistoryOn( bool state ){
  //ignore it for now
  return MStatus::kSuccess;
}

MStatus abcContainersExportCmd::setCommandString( const MString &str) {
  //ignore it for now
  return MStatus::kSuccess;
}

#include "abcTreesExport.h"

#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>

#include <boost/lexical_cast.hpp>

#include <sstream>

template <class T>
 inline std::string to_string (const T& t)
 {
 std::stringstream ss;
 ss << t;
 return ss.str();
 }
//
// abcTreeExportCmd::doIt
//
//  This is the function used to invoke the command. The
// command is not undoable and it does not change any state,
// so it does not use the method to call back throught redoIt.
//////////////////////////////////////////////////////////////////////

namespace Abc =  Alembic::Abc;
namespace Mat = Alembic::AbcMaterial;

namespace // <anonymous>
{
const MStringArray INVALID_COMPONENTS;
const char* rgbComp[3] = {"r", "g", "b"};
const MStringArray RGB_COMPONENTS(rgbComp, 3);
const char* rgbaComp[4] = {"r", "g", "b", "a"};
const MStringArray RGBA_COMPONENTS(rgbaComp, 4);
const char* point2Comp[2] = {"x", "y"};
const MStringArray POINT2_COMPONENTS(point2Comp, 2);
const char* vectorComp[3] = {"x", "y", "z"};
const MStringArray VECTOR_COMPONENTS(vectorComp, 3);
}

const MStringArray& GetComponentNames(int arnoldParamType)
{
   MStringArray componentNames;
   switch (arnoldParamType)
   {
   case AI_TYPE_RGB:
      return RGB_COMPONENTS;
   case AI_TYPE_RGBA:
      return RGBA_COMPONENTS;
   case AI_TYPE_VECTOR:
   case AI_TYPE_POINT:
      return VECTOR_COMPONENTS;
   case AI_TYPE_POINT2:
      return POINT2_COMPONENTS;
   default:
      return INVALID_COMPONENTS;
   }
}

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
  
  Abc::OArchive archive(Alembic::AbcCoreHDF5::WriteArchive(), filename.asChar() );
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
    AtNodeSet* exportedNodes = new AtNodeSet;
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
             CNodeTranslator* translator = arnoldSession->ExportNode(toExport, exportedNodes, NULL ,false, &stat);
             if(exportedNodes->size() > 0)
             {
                 AtNode* root = translator->GetArnoldRootNode();

                 std::set<AtNode*>::const_iterator sit (exportedNodes->begin()), send(exportedNodes->end()); 
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
                                 cout << "this is an array of size " <<  paramArray->nelements << endl;
                                 
                                 processArrayValues(*sit, paramName, paramArray, outputType, matObj, nodeName, container.name());
                                 for(unsigned int i=0; i < paramArray->nelements; i++)
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
    
void abcContainersExportCmd::processArrayValues(AtNode* sit, const char *paramName, AtArray* paramArray, int outputType, Mat::OMaterial matObj, MString nodeName, MString containerName)
{
    int typeArray = paramArray->type;
    if (typeArray == AI_TYPE_INT || typeArray == AI_TYPE_UINT || typeArray == AI_TYPE_ENUM)
    {
        //type int
        Abc::OInt32ArrayProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        std::vector<int> vals;
        for(unsigned int i=0; i < paramArray->nelements; i++)
            vals.push_back(AiArrayGetInt(paramArray, i));
            
        prop.set(vals);

    }
    else if (typeArray == AI_TYPE_FLOAT)
    {
        // type float
        Abc::OFloatArrayProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        std::vector<float> vals;
        for(unsigned int i=0; i < paramArray->nelements; i++)
            vals.push_back(AiArrayGetFlt(paramArray, i));
        prop.set(vals);

    }
    else if (typeArray == AI_TYPE_BOOLEAN)
    {
        // type bool
        Abc::OBoolArrayProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        std::vector<Abc::bool_t> vals;
        for(unsigned int i=0; i < paramArray->nelements; i++)
            vals.push_back(AiArrayGetBool(paramArray, i));

        prop.set(vals);

    }
    else if (typeArray == AI_TYPE_RGB)
    {
        // type rgb
        Abc::OC3fArrayProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        std::vector<Imath::C3f> vals;
        for(unsigned int i=0; i < paramArray->nelements; i++)
        {
            AtColor a_val = AiArrayGetRGB(paramArray, i);
            Imath::C3f color_val( a_val.r, a_val.g, a_val.b );
            vals.push_back(color_val);
        }
        prop.set(vals);        
    }

    else if (typeArray == AI_TYPE_POINT2)
    {
        // type point2
        Abc::OP2fArrayProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        std::vector<Imath::V2f> vals;
        for(unsigned int i=0; i < paramArray->nelements; i++)
        {
            AtPoint2 a_val = AiArrayGetPnt2(paramArray, i);
            Imath::V2f vec_val( a_val.x, a_val.y );
            vals.push_back(vec_val);
        }
        prop.set(vals);        
    }


    else if (typeArray == AI_TYPE_POINT)
    {
        // type point
        Abc::OP3fArrayProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        std::vector<Imath::V3f> vals;
        for(unsigned int i=0; i < paramArray->nelements; i++)
        {
            AtPoint a_val = AiArrayGetPnt(paramArray, i);
            Imath::V3f vec_val( a_val.x, a_val.y, a_val.z );
            vals.push_back(vec_val);
        }
        prop.set(vals);    
    }
    else if (typeArray == AI_TYPE_VECTOR)
    {
        Abc::OV3fArrayProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        std::vector<Imath::V3f> vals;
        for(unsigned int i=0; i < paramArray->nelements; i++)
        {
            AtPoint a_val = AiArrayGetPnt(paramArray, i);
            Imath::V3f vec_val( a_val.x, a_val.y, a_val.z );
            vals.push_back(vec_val);
        }
        prop.set(vals);    
    }
    else if (typeArray == AI_TYPE_STRING)
    {
        // type string
        Abc::OStringArrayProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        std::vector<std::string> vals;
        for(unsigned int i=0; i < paramArray->nelements; i++)
            vals.push_back(AiArrayGetStr(paramArray, i));
        prop.set(vals);    
    }
    
    cout << "exported " << paramArray->nelements << " array values of type " << typeArray << " for " << nodeName.asChar() << "." << paramName << endl;
}



void abcContainersExportCmd::processArrayParam(AtNode* sit, const char *paramName, AtArray* paramArray, int index, int outputType, Mat::OMaterial matObj, MString nodeName, MString containerName)
{
    //first, test if the entry is linked
    //Build the paramname...
    
    int typeArray = paramArray->type;

    MString paramNameArray = MString(paramName) + "[" + MString(to_string(index).c_str()) +"]"; 
    if (!AiNodeGetLink(sit, paramNameArray.asChar()) == NULL)    
        processLinkedParam(sit, typeArray, outputType, matObj, nodeName, paramNameArray.asChar(), containerName);

}

void abcContainersExportCmd::processLinkedParam(AtNode* sit, int inputType, int outputType,  Mat::OMaterial matObj, MString nodeName, const char* paramName, MString containerName)
{
    if (AiNodeIsLinked(sit, paramName))
    {
        // check what is linked exactly

        if(AiNodeGetLink(sit, paramName))
        {
            cout << nodeName << "." << paramName << " is linked" << endl;
            exportLink(sit, outputType, matObj, nodeName, paramName, containerName);
        }
        else
        {
            const MStringArray componentNames = GetComponentNames(inputType);
            unsigned int numComponents = componentNames.length(); 
            for (unsigned int i=0; i < numComponents; i++) 
            {
                MString compAttrName = MString(paramName) + "." + componentNames[i]; 
                if(AiNodeIsLinked(sit, compAttrName.asChar()))
                {
                    cout << "exporting link : " << nodeName << "." << compAttrName.asChar() << endl;
                    exportLink(sit, outputType, matObj, nodeName, compAttrName.asChar(), containerName);

                }
            }

        }



    }
    else
        exportParameter(sit, matObj, inputType, nodeName, paramName);
        

}

void abcContainersExportCmd::exportLink(AtNode* sit, int outputType,  Mat::OMaterial matObj, MString nodeName, const char* paramName, MString containerName)
{
    int comp;
    cout << "checking link " << AiNodeGetName(sit) << "." << paramName << endl;
    AtNode* linked = AiNodeGetLink(sit, paramName, &comp);
        
    MString nodeNameLinked(containerName);
    nodeNameLinked = nodeNameLinked + "_" + MString(AiNodeGetName(linked));
    std::string outPlug;
    
    if(comp == -1)
        outPlug = "";
    else
    {
        if(outputType == AI_TYPE_RGB || outputType == AI_TYPE_RGBA)
        {
            if(comp == 0)
                outPlug = "r";
            else if(comp == 1)
                outPlug = "g";
            else if(comp == 2)
                outPlug = "b";
            else if(comp == 3)
                outPlug = "a";
        }
        else if(outputType == AI_TYPE_VECTOR || outputType == AI_TYPE_POINT ||outputType == AI_TYPE_POINT2)
        {
            if(comp == 0)
                outPlug = "x";
            else if(comp == 1)
                outPlug = "y";
            else if(comp == 2)
                outPlug = "z";                                 
        }

    }
    cout << "Exporting link from " << nodeNameLinked.asChar() << "." << outPlug << " to " <<nodeName.asChar() <<"."<< paramName << endl;
    matObj.getSchema().setNetworkNodeConnection(nodeName.asChar(), paramName, nodeNameLinked.asChar(), outPlug); 

}


void abcContainersExportCmd::exportParameterFromArray(AtNode* sit, Mat::OMaterial matObj, AtArray* paramArray, int index, MString nodeName, const char* paramName)
{
    int type = paramArray->type;

    cout << "Array type " << type << " for " << nodeName.asChar() << "." << paramName << endl;
    if (type == AI_TYPE_INT || type == AI_TYPE_UINT || type == AI_TYPE_ENUM)
    {
        //type int
        Abc::OInt32Property prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        prop.set(AiArrayGetInt(paramArray, index));
    }
    else if (type == AI_TYPE_FLOAT)
    {
        // type float
        Abc::OFloatProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        prop.set(AiArrayGetFlt(paramArray, index));

    }
    else if (type == AI_TYPE_BOOLEAN)
    {
        // type bool
        Abc::OBoolProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        prop.set(AiArrayGetBool(paramArray, index));
    }
    else if (type == AI_TYPE_RGB)
    {
        // type rgb
        Abc::OC3fProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        AtColor a_val = AiArrayGetRGB(paramArray, index);
        Imath::C3f color_val( a_val.r, a_val.g, a_val.b );
        prop.set(color_val);
        cout << "exporting RGB value of " <<  a_val.r << " " << a_val.g << " " << a_val.b << " for " << nodeName.asChar() <<"."<<paramName << endl;
    }
    else if (type == AI_TYPE_POINT2)
    {
        // type point2
        Abc::OP2fProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        AtPoint2 a_val = AiArrayGetPnt2(paramArray, index);
        Imath::V2f vec_val( a_val.x, a_val.y );
        prop.set(vec_val);
    }


    else if (type == AI_TYPE_POINT)
    {
        // type point
        Abc::OP3fProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        AtVector a_val = AiArrayGetPnt(paramArray, index);
        Imath::V3f vec_val( a_val.x, a_val.y, a_val.z );
        prop.set(vec_val);
    }
    else if (type == AI_TYPE_VECTOR)
    {
        // type vector
        Abc::OV3fProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        AtVector a_val = AiArrayGetVec(paramArray, index);
        Imath::V3f vec_val( a_val.x, a_val.y, a_val.z );
        prop.set(vec_val);
    }
    else if (type == AI_TYPE_STRING)
    {
        // type string
        Abc::OStringProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        prop.set(AiArrayGetStr(paramArray, index));
    }

}


void abcContainersExportCmd::exportParameter(AtNode* sit, Mat::OMaterial matObj, int type, MString nodeName, const char* paramName)
{
    //header->setMetaData(md);

    if (type == AI_TYPE_INT || type == AI_TYPE_UINT || type == AI_TYPE_ENUM)
    {
        //type int
        Abc::OInt32Property prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        prop.set(AiNodeGetInt(sit, paramName));
    }
    else if (type == AI_TYPE_FLOAT)
    {
        // type float
        
        Alembic::AbcCoreAbstract::MetaData md;
        const AtNodeEntry* arnoldNodeEntry = AiNodeGetNodeEntry(sit); 
        float val;
        bool success = AiMetaDataGetFlt(arnoldNodeEntry, paramName, "min", &val);
        if(success)
            md.set("min", boost::lexical_cast<std::string>(val)); 

        success = AiMetaDataGetFlt(arnoldNodeEntry, paramName, "max", &val);
        if(success)
            md.set("max", boost::lexical_cast<std::string>(val)); 
        
        success = AiMetaDataGetFlt(arnoldNodeEntry, paramName, "softmin", &val);
        if(success)
            md.set("softmin", boost::lexical_cast<std::string>(val)); 
        
        success = AiMetaDataGetFlt(arnoldNodeEntry, paramName, "softmax", &val);
        if(success)
            md.set("softmax", boost::lexical_cast<std::string>(val)); 

        Abc::OFloatProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName, md); 
        prop.set(AiNodeGetFlt(sit, paramName));

    }
    else if (type == AI_TYPE_BOOLEAN)
    {
        // type bool
        Abc::OBoolProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        prop.set(AiNodeGetBool(sit, paramName));
    }
    else if (type == AI_TYPE_RGB)
    {
        // type rgb
        Abc::OC3fProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        AtColor a_val = AiNodeGetRGB(sit, paramName);
        Imath::C3f color_val( a_val.r, a_val.g, a_val.b );
        prop.set(color_val);
        cout << "exporting RGB value of " <<  a_val.r << " " << a_val.g << " " << a_val.b << " for " << nodeName.asChar() <<"."<<paramName << endl;
    }
    else if (type == AI_TYPE_POINT2)
    {
        // type point
        Abc::OP2fProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        AtPoint2 a_val = AiNodeGetPnt2(sit, paramName);
        Imath::V2f vec_val( a_val.x, a_val.y);
        prop.set(vec_val);
    }
    else if (type == AI_TYPE_POINT)
    {
        // type point
        Abc::OP3fProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        AtVector a_val = AiNodeGetPnt(sit, paramName);
        Imath::V3f vec_val( a_val.x, a_val.y, a_val.z );
        prop.set(vec_val);
    }
    else if (type == AI_TYPE_VECTOR)
    {
        // type vector
        Abc::OV3fProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        AtVector a_val = AiNodeGetVec(sit, paramName);
        Imath::V3f vec_val( a_val.x, a_val.y, a_val.z );
        prop.set(vec_val);
    }
    else if (type == AI_TYPE_STRING)
    {
        // type string
        Abc::OStringProperty prop(matObj.getSchema().getNetworkNodeParameters(nodeName.asChar()), paramName); 
        prop.set(AiNodeGetStr(sit, paramName));
    }


}

//
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

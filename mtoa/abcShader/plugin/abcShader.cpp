#include "abcShader.h"
#include <ai_nodes.h>
#include "translators/NodeTranslator.h"
//class CExtensionAttrHelper;

static const char* declStrings[][4] = {
   {"constant BYTE", "constant ARRAY BYTE", "uniform BYTE", "varying BYTE"}, // AI_TYPE_BYTE
   {"constant INT", "constant ARRAY INT", "uniform INT", "varying INT"}, // AI_TYPE_INT
   {"constant UINT", "constant ARRAY UINT", "uniform UINT", "varying UINT"}, // AI_TYPE_UINT
   {"constant BOOL", "constant ARRAY BOOL", "uniform BOOL", "varying BOOL"}, // AI_TYPE_BOOLEAN
   {"constant FLOAT", "constant ARRAY FLOAT", "uniform FLOAT", "varying FLOAT"}, // AI_TYPE_FLOAT
   {"constant RGB", "constant ARRAY RGB", "uniform RGB", "varying RGB"}, // AI_TYPE_RGB
   {"constant RGBA", "constant ARRAY RGBA", "uniform RGBA", "varying RGBA"}, // AI_TYPE_RGBA
   {"constant VECTOR", "constant ARRAY VECTOR", "uniform VECTOR", "varying VECTOR"}, // AI_TYPE_VECTOR
   {"constant POINT", "constant ARRAY POINT", "uniform POINT", "varying POINT"}, // AI_TYPE_POINT
   {"constant POINT2", "constant ARRAY POINT2", "uniform POINT2", "varying POINT2"}, // AI_TYPE_POINT2
   {"constant STRING", "constant ARRAY STRING", "uniform STRING", "varying STRING"}, // AI_TYPE_STRING
   {"constant POINTER", "constant ARRAY POINTER", "uniform POINTER", "varying POINTER"}, // AI_TYPE_POINTER
   {"constant NODE", "constant ARRAY NODE", "uniform NODE", "varying NODE"}, // AI_TYPE_NODE
   {"constant ARRAY", "constant ARRAY", "uniform ARRAY", "varying ARRAY"}, // AI_TYPE_ARRAY ??
   {"constant MATRIX", "constant ARRAY MATRIX", "uniform MATRIX", "varying MATRIX"}, // AI_TYPE_MATRIX
   {"constant ENUM", "constant ARRAY ENUM", "uniform ENUM", "varying ENUM"} // AI_TYPE_ENUM   
}; 

template <signed ATTR>
void TExportAttribute(AtNode* node, MPlug& plug, const char* attrName) { }

template <>
void TExportAttribute<AI_TYPE_BYTE>(AtNode* node, MPlug& plug, const char* attrName)
{
   AiNodeSetByte(node, attrName, plug.asChar());
}

template <>
void TExportAttribute<AI_TYPE_INT>(AtNode* node, MPlug& plug, const char* attrName)
{
   AiNodeSetInt(node, attrName, plug.asInt());
}

template <>
void TExportAttribute<AI_TYPE_BOOLEAN>(AtNode* node, MPlug& plug, const char* attrName)
{
   AiNodeSetBool(node, attrName, plug.asBool());
}

template <>
void TExportAttribute<AI_TYPE_FLOAT>(AtNode* node, MPlug& plug, const char* attrName)
{
   AiNodeSetFlt(node, attrName, plug.asFloat());
}

template <>
void TExportAttribute<AI_TYPE_RGB>(AtNode* node, MPlug& plug, const char* attrName)
{
    // temp fix
    if (GetSessionMode() == MTOA_SESSION_ASS)
        AiNodeSetRGB(node, attrName, pow(plug.child(0).asFloat(), 2.2f) , pow(plug.child(1).asFloat(), 2.2f), pow(plug.child(2).asFloat(), 2.2f));
    else
        AiNodeSetRGB(node, attrName, plug.child(0).asFloat(), plug.child(1).asFloat(), plug.child(2).asFloat());
}

template <>
void TExportAttribute<AI_TYPE_RGBA>(AtNode* node, MPlug& plug, const char* attrName)
{
    if (GetSessionMode() == MTOA_SESSION_ASS)
        AiNodeSetRGBA(node, attrName, pow(plug.child(0).asFloat(), 2.2f) , pow(plug.child(1).asFloat(), 2.2f), pow(plug.child(2).asFloat(), 2.2f), plug.child(3).asFloat());
    else
        AiNodeSetRGBA(node, attrName, plug.child(0).asFloat(), plug.child(1).asFloat(), plug.child(2).asFloat(), plug.child(3).asFloat());
}

template <>
void TExportAttribute<AI_TYPE_VECTOR>(AtNode* node, MPlug& plug, const char* attrName)
{
   AiNodeSetVec(node, attrName, plug.child(0).asFloat(), plug.child(1).asFloat(), plug.child(2).asFloat());
}

template <>
void TExportAttribute<AI_TYPE_POINT>(AtNode* node, MPlug& plug, const char* attrName)
{
   AiNodeSetPnt(node, attrName, plug.child(0).asFloat(), plug.child(1).asFloat(), plug.child(2).asFloat());
}

template <>
void TExportAttribute<AI_TYPE_POINT2>(AtNode* node, MPlug& plug, const char* attrName)
{
   AiNodeSetPnt2(node, attrName, plug.child(0).asFloat(), plug.child(1).asFloat());
}

template <>
void TExportAttribute<AI_TYPE_STRING>(AtNode* node, MPlug& plug, const char* attrName)
{
   AiNodeSetStr(node, attrName, plug.asString().asChar());
}

typedef bool (*declarationPointer)(AtNode*, const char*, unsigned int);


AtNode* CAbcShaderTranslator::CreateArnoldNodes()
{
   return AddArnoldNode("AbcShader");
}


template <signed ATTR>
void TExportUserAttribute(const CAbcShaderTranslator* translator, AtNode* node, MPlug& plug, const char* attrName)
{
    //TExportAttribute<ATTR>(node, plug, attrName);

}

void CAbcShaderTranslator::ProcessExtraParameter(AtNode* anode, MObject oAttr, MPlug pAttr, const char* aname)
{

    int type = 0;

   MPlugArray connections;
   pAttr.connectedTo(connections, true, false);

   if (connections.length() > 0)
        type = AI_TYPE_NODE;
    
    else if (oAttr.hasFn(MFn::kNumericAttribute)) 
    {
         MFnNumericAttribute nAttr(oAttr);
         MFnNumericData::Type unitType = nAttr.unitType();
         switch (unitType)
         {
             case MFnNumericData::kBoolean:
                type = AI_TYPE_BOOLEAN;
                //TExportUserAttribute<AI_TYPE_BOOLEAN>(this, anode, pAttr, aname);
                break;
             case MFnNumericData::kByte:
             case MFnNumericData::kChar:
                type = AI_TYPE_BYTE;
                //TExportUserAttribute<AI_TYPE_BYTE>(this, anode, pAttr, aname);
                break;
             case MFnNumericData::kShort:
             case MFnNumericData::kLong:
                type = AI_TYPE_INT;
                //TExportUserAttribute<AI_TYPE_INT>(this, anode, pAttr, aname);
                break;
             case MFnNumericData::kFloat:
             case MFnNumericData::kDouble:
                type = AI_TYPE_FLOAT;
                //TExportUserAttribute<AI_TYPE_FLOAT>(this, anode, pAttr, aname);
                break;
             case MFnNumericData::k2Float:
             case MFnNumericData::k2Double:
                type = AI_TYPE_POINT2;
                //TExportUserAttribute<AI_TYPE_POINT2>(this, anode, pAttr, aname);
                break;
             case MFnNumericData::k3Float:
             case MFnNumericData::k3Double:
                if (nAttr.isUsedAsColor())
                    type = AI_TYPE_RGB;
                   //TExportUserAttribute<AI_TYPE_RGB>(this, anode, pAttr, aname);
                else
                    type = AI_TYPE_VECTOR;
                   //TExportUserAttribute<AI_TYPE_VECTOR>(this, anode, pAttr, aname);
                break;
             case MFnNumericData::k4Double:
                type = AI_TYPE_RGBA;
                //TExportUserAttribute<AI_TYPE_RGBA>(this, anode, pAttr, aname);
                break;
             default:
                // not supported: k2Short, k2Long, k3Short, k3Long, kAddr
                AiMsgError("[mtoa.translator]  Unsupported user attribute type");
                break;
         }
    }
    
    if(!AiNodeLookUpUserParameter(anode, aname))
        AiNodeDeclare(anode, aname, declStrings[type][0]);
    ProcessParameter(anode, aname, type, aname);
}

void CAbcShaderTranslator::Export(AtNode* shader)
{
    
    ProcessParameter(shader, "file", AI_TYPE_STRING, "shader"); 
    ProcessParameter(shader, "shader", AI_TYPE_STRING, "shaderFrom"); 
    
    MObject object = GetMayaObject();
    MFnDependencyNode fnDepNode(GetMayaObject());
    for (unsigned int i=0; i<fnDepNode.attributeCount(); ++i) 
    {
        MObject oAttr = fnDepNode.attribute(i);
        MPlug pAttr(object, oAttr); 
        if (!pAttr.parent().isNull()) 
            continue;
      // we only need to export the compound attribute, not the compounds itself
            
        MFnAttribute fnAttr(oAttr);    
        MString name = fnAttr.name(); 
        
        if(name != "message" && name !="caching" && name != "isHistoricallyInteresting" && name != "nodeState" && name != "binMembership" && name != "shader" && name != "shaderFrom" && name != "shaders" && name != "outColor" && name != "aiUserOptions")
            ProcessExtraParameter(shader, oAttr, pAttr, name.asChar());
    }
    
    
}

void CAbcShaderTranslator::NodeInitializer(CAbTranslator context)
{
   CExtensionAttrHelper helper(context.maya, "AbcShader");

} 
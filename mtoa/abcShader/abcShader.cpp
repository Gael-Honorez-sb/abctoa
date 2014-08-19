#include "abcShader.h"
#include <ai_nodes.h>
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



AtNode* CAbcShaderTranslator::CreateArnoldNodes()
{
   return AddArnoldNode("AbcShader");
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
				break;
			 case MFnNumericData::kByte:
			 case MFnNumericData::kChar:
				type = AI_TYPE_BYTE;
				break;
			 case MFnNumericData::kShort:
			 case MFnNumericData::kLong:
				type = AI_TYPE_INT;
				break;
			 case MFnNumericData::kFloat:
			 case MFnNumericData::kDouble:
				type = AI_TYPE_FLOAT;
				break;
			 case MFnNumericData::k2Float:
			 case MFnNumericData::k2Double:
				type = AI_TYPE_POINT2;
				break;
			 case MFnNumericData::k3Float:
			 case MFnNumericData::k3Double:
				if (nAttr.isUsedAsColor())
					type = AI_TYPE_RGB;
				else
					type = AI_TYPE_VECTOR;
				break;
			 case MFnNumericData::k4Double:
				type = AI_TYPE_RGBA;
				break;
			 default:
				// not supported: k2Short, k2Long, k3Short, k3Long, kAddr
				AiMsgError("[mtoa.translator]  Unsupported user attribute type");
				break;
		 }
	}
	else if (oAttr.hasFn(MFn::kTypedAttribute)) 
	{
         MFnTypedAttribute tAttr(oAttr);
         const bool usedAsColor = tAttr.isUsedAsColor();
         switch (tAttr.attrType())	
         {
         case MFnData::kString:
			type = AI_TYPE_STRING;
            break;
         case MFnData::kStringArray:
			type = AI_TYPE_STRING;
            break;
         case MFnData::kDoubleArray:
			type = AI_TYPE_FLOAT;
            break;
         case MFnData::kIntArray:
			type = AI_TYPE_INT;
            break;
         case MFnData::kPointArray:
            if (usedAsColor)
				type = AI_TYPE_RGB;
            else
				type = AI_TYPE_VECTOR;
            break;
         case MFnData::kVectorArray:
            if (usedAsColor)
				type = AI_TYPE_RGB;
            else
				type = AI_TYPE_VECTOR;
            break;
         default:
            // kMatrix, kNumeric (this one should have be caught be hasFn(MFn::kNumericAttribute))
            //AiMsgError("[mtoa.translator]  %s: Unsupported user attribute type for %s",
            //   GetTranslatorName().asChar(), pAttr.partialName(true, false, false, false, false, true).asChar());
            break;
         }		 
	}	
	if(!AiNodeLookUpUserParameter(anode, aname))
		AiNodeDeclare(anode, aname, declStrings[type][0]);
	/*if (type == AI_TYPE_RGB || type == AI_TYPE_RGBA)
	{
		if(GetSessionMode() == MTOA_SESSION_ASS)
		{
			if( type == AI_TYPE_RGB)
				AiNodeSetRGB(anode, aname, pow(pAttr.child(0).asFloat(), 2.2f) , pow(pAttr.child(1).asFloat(), 2.2f), pow(pAttr.child(2).asFloat(), 2.2f));
			else
				AiNodeSetRGBA(anode, aname, pow(pAttr.child(0).asFloat(), 2.2f) , pow(pAttr.child(1).asFloat(), 2.2f), pow(pAttr.child(2).asFloat(), 2.2f), pAttr.child(3).asFloat());
		}
		else
			ProcessParameter(anode, aname, type, aname);
	}
	else*/
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
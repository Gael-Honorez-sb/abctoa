#include "translators/shader/ShaderTranslator.h"

class CAbcShaderTranslator
   :   public CShaderTranslator
{
public:

   virtual void Export(AtNode* shader);
   AtNode* CreateArnoldNodes();
   static void* creator() {return new CAbcShaderTranslator();}
   static void NodeInitializer(CAbTranslator context); 
   void ProcessExtraParameter(AtNode* anode, MObject oAttr, MPlug pAttr, const char* aname);
   //static void TExportUserAttribute(AtNode* node, MPlug& plug, const char* attrName);
};

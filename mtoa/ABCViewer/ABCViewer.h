
#include "translators/shape/ShapeTranslator.h"

class CABCViewerTranslator
   :   public CShapeTranslator
{
public:

   virtual void Export(AtNode* camera);
   virtual void Update(AtNode* procedural);
   virtual void UpdateMotion(AtNode* procedural, unsigned int step);   
   static void NodeInitializer(CAbTranslator context);
   AtNode* CreateArnoldNodes();
   static void* creator()
   {
      return new CABCViewerTranslator();
   }
protected:
    void ProcessRenderFlagsCustom(AtNode* node);
   void ExportBoundingBox(AtNode* procedural);
   void ExportStandinsShaders(AtNode* procedural);
   AtNode* ExportProcedural(AtNode* procedural, bool update);   

   virtual void ExportShaders();

protected:
   MFnDagNode m_DagNode;

};  

#include "translators/shape/ShapeTranslator.h"
#include "translators/NodeTranslator.h" 
#include "pystring.h"

class CABCViewerTranslator
   :   public CShapeTranslator
{
public:

    AtNode* CreateArnoldNodes();
    virtual void Export(AtNode* procedural);
    void ExportMotion(AtNode*); 
    static void NodeInitializer(CAbTranslator context);
   
    static void* creator()
    {
        return new CABCViewerTranslator();
    }
protected:

   void ProcessRenderFlagsCustom(AtNode* node);
   void ExportBoundingBox(AtNode* procedural);
   void ExportStandinsShaders(AtNode* procedural);
   void ExportProcedural(AtNode* procedural, bool update);

   virtual void ExportShaders();

protected:
   MFnDagNode m_DagNode;

};
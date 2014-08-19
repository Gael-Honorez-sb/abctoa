#include "abcShader.h"
#include "extension/Extension.h"

extern "C"
{

DLLEXPORT void initializeExtension(CExtension &plugin)
{
   MStatus status;

   plugin.Requires("abcMayaShader");
   status = plugin.RegisterTranslator("abcMayaShader",
         "AbcShader",
         CAbcShaderTranslator::creator,
         CAbcShaderTranslator::NodeInitializer);
}

DLLEXPORT void deinitializeExtension(CExtension &plugin)
{
}

}

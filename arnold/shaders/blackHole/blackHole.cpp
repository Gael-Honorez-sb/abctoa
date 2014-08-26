/*
 * Matte shader: creates an alpha matte (or blackhole) in the rendered image
 */

#include <ai.h>
#include <cstring>

AI_SHADER_NODE_EXPORT_METHODS(blackholeMtd);

enum blackHoleParams
{
   p_input
};


node_parameters
{
   AiParameterRGBA ( "input", 0,0,0,0 );

   AiMetaDataSetInt(mds, NULL, "maya.id", 0x70509);

   AiMetaDataSetStr(mds, NULL, "maya.classification", "shader/surface");
}

node_initialize
{
}

node_update
{

}

node_finish
{
}

shader_evaluate
{
   if (sg->Rt == AI_RAY_CAMERA)
      sg->out.RGBA = AI_RGBA_BLACK;
   else
      sg->out.RGBA = AiShaderEvalParamRGBA(p_input);


}

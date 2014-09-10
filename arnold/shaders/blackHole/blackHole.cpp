/*BlackHole Shader
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

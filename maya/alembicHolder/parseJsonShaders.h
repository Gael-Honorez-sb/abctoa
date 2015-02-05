#include <json/json.h>
#include <maya/MColor.h>
#include <maya/MObject.h>
#include <maya/MString.h>
#include <stdlib.h>

#ifndef _ParseJsonShaders_h_
#define _ParseJsonShaders_h_


const size_t hash( std::string const& s );
MObject findShader( MObject& setNode );
bool findShaderColor(MString shaderName, MColor & shaderColor);
void ParseShaders(Json::Value jroot, std::map<std::string, MColor> & shaderColors);

#endif

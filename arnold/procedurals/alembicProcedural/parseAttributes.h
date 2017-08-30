
#include <ai.h>
#include <string>
#include <vector>
#include <map>
#include "json/json.h"
#include <vector>
#include <regex>
#include "ProcArgs.h"
#include <Alembic/Abc/All.h>
#include <Alembic/AbcMaterial/IMaterial.h>



#ifndef _Alembic_Arnold_ParseOverrides_h_
#define _Alembic_Arnold_ParseOverrides_h_


void getTags(Alembic::AbcGeom::IObject iObj, std::vector<std::string> & tags, ProcArgs* args);
void getAllTags(Alembic::AbcGeom::IObject iObj, std::vector<std::string> & tags, ProcArgs* args);
bool isVisible(Alembic::AbcGeom::IObject child, const Alembic::AbcGeom::IXformSchema xs, ProcArgs* args);
bool isVisibleForArnold(Alembic::AbcGeom::IObject child, ProcArgs* args);
void OverrideProperties(Json::Value & jroot, Json::Value jrootOverrides);
void ParseShaders(Json::Value jroot, const std::string& ns, const std::string& nameprefix, ProcArgs* args, uint8_t type);
Json::Value OverrideAssignations(Json::Value jroot, Json::Value jrootOverrides);

#endif
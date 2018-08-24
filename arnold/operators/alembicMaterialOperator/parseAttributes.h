
#include <ai.h>
#include <string>
#include <vector>
#include <map>
#include "json/json.h"
#include <vector>
#include <regex>
#include "operatorData.h"

#include <Alembic/Abc/All.h>



#ifndef _Alembic_Arnold_ParseOverrides_h_
#define _Alembic_Arnold_ParseOverrides_h_

void OverrideProperties(Json::Value & jroot, Json::Value jrootOverrides);
void ParseShaders(Json::Value jroot, const std::string& ns, const std::string& nameprefix, OperatorData* data, uint8_t type);
Json::Value OverrideAssignations(Json::Value jroot, Json::Value jrootOverrides);

#endif
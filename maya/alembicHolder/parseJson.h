#ifndef _ParseJson_h_
#define _ParseJson_h_

#include <json/json.h>
#include <maya/MColor.h>

struct holderPrms
{
	bool linkAttributes;
    std::vector<std::string> attributes;
    Json::Value attributesRoot;

};


void update(Json::Value& a, Json::Value& b);
void OverrideProperties(Json::Value & jroot, Json::Value jrootAttributes);


#endif
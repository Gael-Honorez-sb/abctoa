#include "parseJson.h"

// Recursively copy the values of b into a.
void update(Json::Value& a, Json::Value& b) {
    Json::Value::Members memberNames = b.getMemberNames();
    for (Json::Value::Members::const_iterator it = memberNames.begin();
            it != memberNames.end(); ++it)
    {
        const std::string& key = *it;
        if (a[key].isObject()) {
            update(a[key], b[key]);
        } else {
            a[key] = b[key];
        }
    }
}

void OverrideProperties(Json::Value & jroot, Json::Value jrootAttributes)
{
    for( Json::ValueIterator itr = jrootAttributes.begin() ; itr != jrootAttributes.end() ; itr++ )
    {
        const Json::Value paths = jrootAttributes[itr.key().asString()];
        for( Json::ValueIterator overPath = paths.begin() ; overPath != paths.end() ; overPath++ )
        {
            Json::Value attr = paths[overPath.key().asString()];
            jroot[itr.key().asString()][overPath.key().asString()] = attr;
        }
    }
}
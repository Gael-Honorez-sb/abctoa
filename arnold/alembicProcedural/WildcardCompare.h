// WildcardCompare.h
#ifndef WILDCARDCOMPARE_H
#define WILDCARDCOMPARE_H

#include <string>
#include <vector>

bool WildcardCompare(std::string searchTerm, std::string checkAgaints)
{
    bool found = false;
    std::vector<std::string> parts;
    std::string temp;
    if (searchTerm == "*")
        return true;
    if (searchTerm.size() - 1 > checkAgaints.size())
        return false;
    std::string::const_iterator it = searchTerm.begin(), end = searchTerm.end();
    size_t counter = 0;
    while (it != end)
    {
        if (*it != '*')
            temp += *it;
        if (*it == '*')
        {
            parts.push_back(temp);
            temp = "";
        }
        it++;
    }
    parts.push_back(temp);
    std::vector<std::string>::const_iterator vecIt = parts.begin(), vecEnd = parts.end();
    if (!parts[0].empty())
    {
        if (parts[0] != checkAgaints.substr(0, parts[0].size()))
            return false;
    }
    else
        vecIt++;
    size_t size = checkAgaints.size();
    size_t pos = 0;
    size_t tempSize;
    while (vecIt != vecEnd)
    {
        temp = *vecIt;
        tempSize = temp.size();
        if (temp.empty())
            return true;
        while (pos + tempSize < size)
        {
            if (temp == checkAgaints.substr(pos, temp.size()))
            {
                checkAgaints.erase(0, pos + temp.size());
                found = true;
                break;
            }
            pos++;
        }
        if (found == false)
            return false;
        pos = 0;
        vecIt++;
    }
    return true;
}

#endif
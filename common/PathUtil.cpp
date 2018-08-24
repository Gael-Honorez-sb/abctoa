//-*****************************************************************************
//
// Copyright (c) 2009-2010,
//  Sony Pictures Imageworks Inc. and
//  Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Sony Pictures Imageworks, nor
// Industrial Light & Magic, nor the names of their contributors may be used
// to endorse or promote products derived from this software without specific
// prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//-*****************************************************************************
#include "PathUtil.h"

#include <regex>
#include <pystring.h>
#include <string.h>

//-*****************************************************************************

bool pathContainsOtherPath(const std::string &path, const std::string &otherPath )
{
    std::vector<std::string> pathParts;
    std::vector<std::string> jsonPathParts;

    TokenizePath(path, "/", pathParts);
    TokenizePath(otherPath, "/", jsonPathParts);

    if(jsonPathParts.size() > pathParts.size())
        return false;

    bool validPath = true;
    for(int i = 0; i < jsonPathParts.size(); i++)
    {
        if(pathParts[i].compare(jsonPathParts[i]) != 0)
            validPath = false;
    }
    if(validPath)
        return validPath;

    return false;
}

bool pathInJsonString(const std::string &path, const std::string &jsonString )
{
    std::vector<std::string> pathParts;
    std::vector<std::string> jsonPathParts;
    pystring::split(path, pathParts, "/");

    Json::Value jroot;
    Json::Reader reader;
    bool parsingSuccessful = reader.parse( jsonString, jroot, false );
    if(parsingSuccessful)
    {
        for( Json::ValueIterator itr = jroot.begin() ; itr != jroot.end() ; itr++ )
        {
            std::string jpath = jroot[itr.key().asUInt()].asString();
            pystring::split(jpath, jsonPathParts, "/");

            if(jsonPathParts.size() > pathParts.size())
                continue;

            bool validPath = true;
            for(int i = 0; i < jsonPathParts.size(); i++)
                if(pathParts[i].compare(jsonPathParts[i]) != 0)
                    validPath = false;
           
            if(validPath)
                return validPath;
        }
    }
    return false;
}

void TokenizePath(const std::string &path, const char* separator, std::vector<std::string> &result)
{
    std::vector<std::string> tokenizer;

    pystring::split(path, tokenizer, separator);
    
    for (std::vector<std::string>::iterator iter = tokenizer.begin(); iter != tokenizer.end(); ++iter)
    {
        if ((*iter).empty()) { continue; }
        result.push_back(*iter);
    }
}

/*
* Return a new string with all occurrences of 'from' replaced with 'to'
*/
std::string replace_all(const std::string &str, const char *from, const char *to)
{
    std::string result(str);
    std::string::size_type
        index = 0,
        from_len = strlen(from),
        to_len = strlen(to);
    while ((index = result.find(from, index)) != std::string::npos) {
        result.replace(index, from_len, to);
        index += to_len;
    }
    return result;
}


/*
* Translate a shell pattern into a regular expression
* This is a direct translation of the algorithm defined in fnmatch.py.
*/
static std::string translate(const char *pattern)
{
    int i = 0, n = strlen(pattern);
    std::string result;

    while (i < n) {
        char c = pattern[i];
        ++i;

        if (c == '*') {
            result += ".*";
        } else if (c == '?') {
            result += '.';
        } else if (c == '[') {
            int j = i;
            /*
* The following two statements check if the sequence we stumbled
* upon is '[]' or '[!]' because those are not valid character
* classes.
*/
            if (j < n && pattern[j] == '!')
                ++j;
            if (j < n && pattern[j] == ']')
                ++j;
            /*
* Look for the closing ']' right off the bat. If one is not found,
* escape the opening '[' and continue. If it is found, process
* the contents of '[...]'.
*/
            while (j < n && pattern[j] != ']')
                ++j;
            if (j >= n) {
                result += "\\[";
            } else {
                std::string stuff = replace_all(std::string(&pattern[i], j - i), "\\", "\\\\");
                char first_char = pattern[i];
                i = j + 1;
                result += "[";
                if (first_char == '!') {
                    result += "^" + stuff.substr(1);
                } else if (first_char == '^') {
                    result += "\\" + stuff;
                } else {
                    result += stuff;
                }
                result += "]";
            }
        } else {
            if (isalnum(c) || c== '_') {
                result += c;
            } else {
                result += "\\";
                result += c;
            }
        }
    }
    /*
* Make the expression multi-line and make the dot match any character at all.
*/
    return result;
}

bool matchPattern(const std::string& str, const std::string& pat)
{
    bool result = false;

    try 
    {
        std::regex rx (translate(pat.c_str()).c_str());
        result = std::regex_search(str,rx);
    } 
 
    catch (const std::regex_error& e) 
    {
        return false;
    }
    
    return result;
}

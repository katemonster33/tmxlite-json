/*********************************************************************
Raphaël Frantz 2021

tmxlite - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

#ifdef USE_EXTLIBS
#include <cJSON.h>
#else
#include "detail/cJSON.h"
#endif
#include <tmxlite/FreeFuncs.hpp>
#include <tmxlite/ObjectTypes.hpp>
#include <tmxlite/detail/Log.hpp>

using namespace tmx;

bool ObjectTypes::load(const std::string &path)
{
    std::string contents;
    if (!readFileIntoString(path, &contents))
    {
        Logger::log("Failed to read file " + path, Logger::Type::Error);
        return reset();
    }
    return loadFromString(contents, getFilePath(path));
}

bool ObjectTypes::loadFromString(const std::string &data, const std::string &workingDir)
{
    reset();

    //open the doc
    cJSON* doc = cJSON_Parse(data.c_str());
    if (!doc)
    {
        Logger::log("Failed opening object types", Logger::Type::Error);
        return false;
    }

    //make sure we have consistent path separators
    m_workingDirectory = workingDir;
    std::replace(m_workingDirectory.begin(), m_workingDirectory.end(), '\\', '/');

    if (!m_workingDirectory.empty() &&
        m_workingDirectory.back() == '/')
    {
        m_workingDirectory.pop_back();
    }


    //find the node and bail if it doesn't exist
    cJSON *node = nullptr;
    for(cJSON *child = doc->child; child != nullptr; child = child->next)
    {
        if(std::string(child->string) == "objecttypes") {
            node = child;
            break;
        }
    }
    if (!node)
    {
        Logger::log("Failed object types: no objecttypes node found", Logger::Type::Error);
        return reset();
    }

    return parseObjectTypesNode(*node);
}

bool ObjectTypes::parseObjectTypesNode(const cJSON &node)
{
    //<objecttypes> <-- node
    //  <objecttype name="Character" color="#1e47ff">
    //    <property>...

    //parse types
    for(cJSON *child = node.child; child != nullptr; child = child->next)
    {
        std::string attribString = child->string;
        if (attribString == "objecttype")
        {
            Type type;

            //parse the metadata of the type
            type.colour = colourFromString("#FFFFFFFF");
            for(cJSON *attrib = child->child; attrib != nullptr; attrib = attrib->next) {
                if(std::string(attrib->string) == "name") {
                    type.name = std::string(attrib->valuestring);
                } else if(std::string(attrib->string) == "color") {
                    type.colour = colourFromString(attrib->valuestring);
                } else {
                    for(cJSON *property = attrib->child; property != nullptr; property = property->next) {
                        Property prop;
                        prop.parse(*property, true);
                        type.properties.push_back(prop);
                    }
                }
            }

            m_types.push_back(type);
        }
        else
        {
            LOG("Unidentified name " + attribString + ": node skipped", Logger::Type::Warning);
        }
    }

    return true;
}

bool ObjectTypes::reset()
{
    m_workingDirectory.clear();
    m_types.clear();
    return false;
}

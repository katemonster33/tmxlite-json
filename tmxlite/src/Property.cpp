/*********************************************************************
Matt Marchant 2016 - 2021
http://trederia.blogspot.com

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
#include <tmxlite/Property.hpp>
#include <tmxlite/detail/Log.hpp>
#include <tmxlite/FreeFuncs.hpp>

using namespace tmx;

Property::Property()
    : m_type(Type::Undef)
{
}

Property Property::fromBoolean(bool value)
{
    Property p;
    p.m_type = Type::Boolean;
    p.m_boolValue = value;
    return p;
}

Property Property::fromFloat(float value)
{
    Property p;
    p.m_type = Type::Float;
    p.m_floatValue = value;
    return p;
}

Property Property::fromInt(int value)
{
    Property p;
    p.m_type = Type::Int;
    p.m_intValue = value;
    return p;
}

Property Property::fromString(const std::string& value)
{
    Property p;
    p.m_type = Type::String;
    p.m_stringValue = value;
    return p;
}

Property Property::fromColour(const Colour& value)
{
    Property p;
    p.m_type = Type::Colour;
    p.m_colourValue = value;
    return p;
}

Property Property::fromFile(const std::string& value)
{
    Property p;
    p.m_type = Type::File;
    p.m_stringValue = value;
    return p;
}

Property Property::fromObject(int value)
{
    Property p;
    p.m_type = Type::Object;
    p.m_intValue = value;
    return p;
}

//public
void Property::parse(const cJSON& node, bool isObjectTypes)
{
    // The value attribute name is different in object types
    const char *const valueAttribute = isObjectTypes ? "default" : "value";

    std::string attribData = node.string;
    if (attribData != "property")
    {
        Logger::log("Node was not a valid property, node will be skipped", Logger::Type::Error);
        return;
    }

    attribData = "string";
    cJSON *valueNode = nullptr, *propertyNode = nullptr;
    for(cJSON *child = node.child; child != nullptr; child = child->next) {
        std::string childName = std::string(child->string);
        if(childName == "name") {
            m_name = child->valuestring;
        } else if(childName == "type") {
            attribData = child->valuestring;
        } else if(childName == valueAttribute) {
            valueNode = child;
        } else if(childName == "propertytype") {
            propertyNode = child;
        }
    }
    if (attribData == "bool")
    {
        attribData = valueNode != nullptr ? valueNode->valuestring : "false";;
        m_boolValue = (attribData == "true");
        m_type = Type::Boolean;
        return;
    }
    else if (attribData == "int")
    {
        m_intValue = valueNode != nullptr ? int(valueNode->valuedouble) : 0;
        m_type = Type::Int;
        return;
    }
    else if (attribData == "float")
    {
        m_floatValue = valueNode != nullptr ? float(valueNode->valuedouble) : 0.f;
        m_type = Type::Float;
        return;
    }
    else if (attribData == "string")
    {
        m_stringValue = valueNode != nullptr ? valueNode->string : "";

        //if value is empty, try getting the child value instead
        //as this is how multiline string properties are stored.
        if(m_stringValue.empty() && valueNode != nullptr)
        {
            m_stringValue = valueNode->child->valuestring;
        }

        m_type = Type::String;
        return;
    }
    else if (attribData == "color")
    {
        m_colourValue = colourFromString(valueNode != nullptr ? valueNode->valuestring : "#FFFFFFFF");
        m_type = Type::Colour;
        return;
    }
    else if (attribData == "file")
    {
        m_stringValue = valueNode != nullptr ? valueNode->valuestring : "";
        m_type = Type::File;
        return;
    }
    else if (attribData == "object")
    {
        m_intValue = valueNode != nullptr ? int(valueNode->valuedouble) : 0;
        m_type = Type::Object;
        return;
    }
    else if (attribData == "class")
    {
        m_type = Type::Class;
        m_propertyType = propertyNode != nullptr ? propertyNode->valuestring : "null";

        const std::string firstChildName = propertyNode->child != nullptr ? propertyNode->child->string : "";
        if (firstChildName == "properties" && propertyNode->child->child != nullptr)
        {
            for(cJSON *childProp = propertyNode->child->child; childProp != nullptr; childProp++)
            {
                m_classValue.emplace_back();
                m_classValue.back().parse(*childProp);
            }
        }
        return;
    }
}

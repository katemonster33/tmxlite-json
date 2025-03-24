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
#include <cJSON/cJSON.h>
#else
#include "detail/cJSON.h"
#endif
#include <tmxlite/Object.hpp>
#include <tmxlite/FreeFuncs.hpp>
#include <tmxlite/Map.hpp>
#include <tmxlite/Tileset.hpp>
#include <tmxlite/detail/Log.hpp>

#include <sstream>

using namespace tmx;

Object::Object()
    : m_UID     (0),
    m_rotation  (0.f),
    m_tileID    (0),
    m_flipFlags (0),
    m_visible   (true),
    m_shape     (Shape::Rectangle)
{

}

bool Object::parseChild(const struct cJSON &child, tmx::Map* map)
{
    std::string attribString = child.string;
    if(attribString == "id") {
        m_UID = int(child.valuedouble);
    } else if(attribString == "name") {
        m_name = child.valuestring;
    } else if(attribString == "type" || attribString == "class") {
        m_class = child.valuestring;
    } else if(attribString == "x") {
        m_position.x = float(child.valuedouble);
        m_AABB.left = m_position.x;
    } else if(attribString == "y") {
        m_position.y = float(child.valuedouble);
        m_AABB.top = m_position.y;
    } else if(attribString == "width") {
        m_AABB.width = float(child.valuedouble);
    } else if(attribString == "height") {
        m_AABB.height = float(child.valuedouble);
    } else if(attribString == "rotation") {
        m_rotation = float(child.valuedouble);
    } else if(attribString == "visible") {
        m_visible = std::string(child.valuestring) == "true";
    } else if(attribString == "gid") {
        m_tileID = int(child.valuedouble);
    } else if (attribString == "properties") {
        for(cJSON *propNode = child.child; propNode != nullptr; propNode = propNode->next) {
            m_properties.emplace_back();
            m_properties.back().parse(*propNode);
        }
    } else if (attribString == "ellipse") {
        m_shape = Shape::Ellipse;
    } else if (attribString == "point") {
        m_shape = Shape::Point;
    } else if (attribString == "polygon") {
        m_shape = Shape::Polygon;
        parsePoints(child);
    } else if (attribString == "polyline") {
        m_shape = Shape::Polyline;
        parsePoints(child);
    } else if (attribString == "text") {
        m_shape = Shape::Text;
        parseText(child);
    } else if(attribString == "template") {
        m_template = child.valuestring;
    } else {
        return false;
    }
    return true;
}

//public
bool Object::parse(const cJSON& node, Map* map)
{
    std::string attribString = node.string;
    if (attribString != "object") {
        Logger::log("This not an Object node, parsing skipped.", Logger::Type::Error);
        return false;
    }

    bool retval = Parsable::parse(node, map);
    if(retval) {
        static const std::uint32_t mask = 0xf0000000;
        m_flipFlags = ((m_tileID & mask) >> 28);
        m_tileID = m_tileID & ~mask;
        if(!m_template.empty()) {
            //parse templates last so we know which properties
            //ought to be overridden
            parseTemplate(m_template, map);
        }
    }
    return retval;
}

//private
void Object::parsePoints(const cJSON& node)
{
    for(cJSON *pointNode = node.child; pointNode != nullptr; pointNode = pointNode->next) {
        float x = 0, y = 0;
        for(cJSON *pointAttrNode = pointNode->child; pointAttrNode != nullptr; pointAttrNode = pointAttrNode->next) {
            std::string attrName = pointAttrNode->string;
            if(attrName == "x") {
                x = float(pointAttrNode->valuedouble);
            } else if(attrName == "y") {
                y = float(pointAttrNode->valuedouble);
            }
        }
        m_points.push_back({x, y});
    }
    if(m_points.empty())
    {
        Logger::log("Points for polygon or polyline object are missing", Logger::Type::Warning);
    }
}

void Object::parseText(const cJSON& node)
{
    for(cJSON *child = node.child; child != nullptr; child = child->next) {
        std::string name = child->string;
        if(name == "bold") {
            m_textData.bold = std::string(child->valuestring) == "true";
        } else if(name == "color") {
            m_textData.colour = colourFromString(child->valuestring);
        } else if(name == "fontfamily") {
            m_textData.fontFamily = child->valuestring;
        } else if(name == "italic") {
            m_textData.italic = std::string(child->valuestring) == "true";
        } else if(name == "kerning") {
            m_textData.kerning = std::string(child->valuestring) == "true";
        } else if(name == "pixelsize") {
            m_textData.pixelSize = uint32_t(child->valuedouble);
        } else if(name == "strikeout") {
            m_textData.strikethough = std::string(child->valuestring) == "true";
        } else if(name == "underline") {
            m_textData.underline = std::string(child->valuestring) == "true";
        } else if(name == "wrap") {
            m_textData.wrap = std::string(child->valuestring) == "true";
        } else if(name == "halign") {
            std::string alignment = child->valuestring;
            if (alignment == "left") {
                m_textData.hAlign = Text::HAlign::Left;
            } else if (alignment == "center") {
                m_textData.hAlign = Text::HAlign::Centre;
            } else if (alignment == "right") {
                m_textData.hAlign = Text::HAlign::Right;
            }
        } else if(name == "valign") {
            std::string alignment = child->valuestring;
            if (alignment == "top") {
                m_textData.vAlign = Text::VAlign::Top;
            } else if (alignment == "center") {
                m_textData.vAlign = Text::VAlign::Centre;
            } else if (alignment == "bottom") {
                m_textData.vAlign = Text::VAlign::Bottom;
            }
        } else if(name == "text") {
            m_textData.content = child->valuestring;
        }
    }

}

void Object::parseTemplate(const std::string& path, Map* map)
{
    assert(map);

    auto& templateObjects = map->getTemplateObjects();
    auto& templateTilesets = map->getTemplateTilesets();

    //load the template if not already loaded
    if (templateObjects.count(path) == 0)
    {
        auto templatePath = map->getWorkingDirectory() + "/" + path;

        cJSON *doc = cJSON_Parse(templatePath.c_str());
        if (!doc)
        {
            Logger::log("Failed opening template file " + path, Logger::Type::Error);
            return;
        }

        cJSON *templateNode = nullptr;
        for(cJSON *child = doc->child; child != nullptr; child = child->next) {
            if(std::string(child->string) == "template") {
                templateNode = child;
                break;
            }
        }
        if (!templateNode)
        {
            Logger::log("Template node missing from " + path, Logger::Type::Error);
            return;
        }

        cJSON *objectNode = nullptr;
        std::string tilesetName;
        for(cJSON *child = templateNode->child; child != nullptr; child = child->next) {
            std::string name = child->string;
            if(name == "tileset") {
                for(cJSON *tilesetChild = child->child; tilesetChild != nullptr; tilesetChild = tilesetChild->next) {
                    if(std::string(tilesetChild->string) == "source") {
                        tilesetName = tilesetChild->valuestring;
                        break;
                    }
                }
                if (!tilesetName.empty() &&
                    templateTilesets.count(tilesetName) == 0)
                {
                    templateTilesets.insert(std::make_pair(tilesetName, Tileset(map->getWorkingDirectory())));
                    templateTilesets.at(tilesetName).parse(*child, map);
                }
            } else if(name == "object") {
                objectNode = child;
            }
        }
        //if the template has a tileset load that (if not already loaded)

        //parse the object - don't pass the map pointer here so there's
        //no recursion if someone tried to get clever and put a template in a template
        if (objectNode)
        {
            templateObjects.insert(std::make_pair(path, Object()));
            templateObjects[path].parse(*objectNode, nullptr);
            templateObjects[path].m_tilesetName = tilesetName;
        }
    }

    //apply any non-overridden object properties from the template
    if (templateObjects.count(path) != 0)
    {
        const auto& obj = templateObjects[path];
        if (m_AABB.width == 0)
        {
            m_AABB.width = obj.m_AABB.width;
        }

        if (m_AABB.height == 0)
        {
            m_AABB.height = obj.m_AABB.height;
        }
        
        m_tilesetName = obj.m_tilesetName;

        if (m_name.empty())
        {
            m_name = obj.m_name;
        }

        if (m_class.empty())
        {
            m_class = obj.m_class;
        }

        if (m_rotation == 0)
        {
            m_rotation = obj.m_rotation;
        }

        if (m_tileID == 0)
        {
            m_tileID = obj.m_tileID;
        }

        if (m_flipFlags == 0)
        {
            m_flipFlags = obj.m_flipFlags;
        }

        if (m_shape == Shape::Rectangle)
        {
            m_shape = obj.m_shape;
        }

        if (m_points.empty())
        {
            m_points = obj.m_points;
        }

        //compare properties and only copy ones that don't exist
        for (const auto& p : obj.m_properties)
        {
            auto result = std::find_if(m_properties.begin(), m_properties.end(), 
                [&p](const Property& a)
                {
                    return a.getName() == p.getName();
                });

            if (result == m_properties.end())
            {
                m_properties.push_back(p);
            }
        }


        if (m_shape == Shape::Text)
        {
            //check each text property and update as necessary
            //TODO this makes he assumption we prefer the template
            //properties over the default ones - this might not
            //actually be the case....
            const auto& otherText = obj.m_textData;
            if (m_textData.fontFamily.empty())
            {
                m_textData.fontFamily = otherText.fontFamily;
            }

            if (m_textData.pixelSize == 16)
            {
                m_textData.pixelSize = otherText.pixelSize;
            }

            //TODO this isn't actually right if we *want* to be false
            //and the template is set to true...
            if (m_textData.wrap == false)
            {
                m_textData.wrap = otherText.wrap;
            }

            if (m_textData.colour == Colour())
            {
                m_textData.colour = otherText.colour;
            }

            if (m_textData.bold == false)
            {
                m_textData.bold = otherText.bold;
            }

            if (m_textData.italic == false)
            {
                m_textData.italic = otherText.italic;
            }

            if (m_textData.underline == false)
            {
                m_textData.underline = otherText.underline;
            }

            if (m_textData.strikethough == false)
            {
                m_textData.strikethough = otherText.strikethough;
            }

            if (m_textData.kerning == true)
            {
                m_textData.kerning = otherText.kerning;
            }

            if (m_textData.hAlign == Text::HAlign::Left)
            {
                m_textData.hAlign = otherText.hAlign;
            }

            if (m_textData.vAlign == Text::VAlign::Top)
            {
                m_textData.vAlign = otherText.vAlign;
            }

            if (m_textData.content.empty())
            {
                m_textData.content = otherText.content;
            }
        }
    }
}

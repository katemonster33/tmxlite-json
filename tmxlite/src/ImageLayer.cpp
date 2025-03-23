/*********************************************************************
Matt Marchant 2016 - 2023
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
#include <tmxlite/ImageLayer.hpp>
#include <tmxlite/FreeFuncs.hpp>
#include <tmxlite/detail/Log.hpp>

using namespace tmx;

ImageLayer::ImageLayer(const std::string& workingDir)
    : m_workingDir      (workingDir),
    m_hasTransparency   (false),
    m_hasRepeatX        (false),
    m_hasRepeatY        (false)
{

}

bool ImageLayer::parseChild(const cJSON &child, tmx::Map *map)
{
    std::string childName = child.string;
    if(childName == "repeatx") {
        m_hasRepeatX = std::string(child.valuestring) == "true";
    } else if(childName == "repeaty") {
        m_hasRepeatY = std::string(child.valuestring) == "true";
    } else if(childName == "transparentcolor") {
        m_transparencyColour = colourFromString(child.valuestring);
        m_hasTransparency = true;
    } else if(childName == "image") {
        m_filePath = resolveFilePath(child.valuestring, m_workingDir);
    } else if(childName == "imagewidth") {
        m_imageSize.x = (unsigned int)child.valuedouble;
    } else if(childName == "imageheight") {
        
        m_imageSize.y = (unsigned int)child.valuedouble;
    } else {
        return Layer::parseChild(child, map);
    }
    return true;
}

//public
bool ImageLayer::parse(const cJSON& node, Map* map)
{
    std::string attribName = node.string;
    if (attribName != "imagelayer")
    {
        Logger::log("Node not an image layer, node skipped", Logger::Type::Error);
        return false;
    }
    bool retval = Parsable::parse(node, map);

    if(retval){
        if(m_filePath.empty()) {
            Logger::log("Image Layer has missing source property", Logger::Type::Warning);
            return false;
        }
    }
    return retval;
}

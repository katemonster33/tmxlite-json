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
#include <tmxlite/FreeFuncs.hpp>
#include <tmxlite/ObjectGroup.hpp>
#include <tmxlite/detail/Log.hpp>

using namespace tmx;

ObjectGroup::ObjectGroup()
    : m_colour    (127, 127, 127, 255),
    m_drawOrder (DrawOrder::TopDown)
{

}

bool ObjectGroup::parseChild(const struct cJSON &child, tmx::Map* map)
{
    std::string childName = child.string;
    if(childName == "objects") {
        for(cJSON *objectNode = child.child; objectNode != nullptr; objectNode = objectNode->next) {
            m_objects.emplace_back();
            m_objects.back().parse(*objectNode, map);
        }
    } else if(childName == "draworder") {
        if(std::string(child.valuestring) == "index") {
            m_drawOrder = DrawOrder::Index;
        } else {
            m_drawOrder = DrawOrder::TopDown;
        }
    } else {
        return Layer::parseChild(child, map);
    }
    return true;
}

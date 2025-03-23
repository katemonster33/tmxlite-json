/*********************************************************************
Grant Gangi 2019
Matt Marchant 2023

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
#include "detail/cJson.h"
#endif
#include <tmxlite/LayerGroup.hpp>
#include <tmxlite/FreeFuncs.hpp>
#include <tmxlite/ObjectGroup.hpp>
#include <tmxlite/ImageLayer.hpp>
#include <tmxlite/TileLayer.hpp>
#include <tmxlite/detail/Log.hpp>

using namespace tmx;

LayerGroup::LayerGroup(const std::string& workingDir, const Vector2u& tileCount)
    : m_workingDir(workingDir),
    m_tileCount(tileCount)
{
}

bool LayerGroup::parseChild(const struct cJSON &child, tmx::Map* map)
{
    assert(map != nullptr);
    if(std::string(child.string) == "layer") {
        m_layerNode = &child;
    } else {
        return Layer::parseChild(child, map);
    }
    return true;
}

//public
bool LayerGroup::parse(const cJSON& node, Map* map)
{
    std::string attribString = node.string;
    if (attribString != "group") {
        Logger::log("Node was not a group layer, node will be skipped.", Logger::Type::Error);
        return false;
    }

    bool retval = Parsable::parse(node, map);
    if(retval && m_layerNode)
    {
        for(cJSON *layerChild = m_layerNode->child; layerChild != nullptr; layerChild = layerChild->next) {
            std::string layerChildType = layerChild->string;
            if (attribString == "layer") {
                m_layers.emplace_back(std::make_unique<TileLayer>(m_tileCount.x * m_tileCount.y));
                m_layers.back()->parse(*layerChild, map);
            } else if (attribString == "objectgroup") {
                m_layers.emplace_back(std::make_unique<ObjectGroup>());
                m_layers.back()->parse(*layerChild, map);
            } else if (attribString == "imagelayer") {
                m_layers.emplace_back(std::make_unique<ImageLayer>(m_workingDir));
                m_layers.back()->parse(*layerChild, map);
            } else if (attribString == "group") {
                m_layers.emplace_back(std::make_unique<LayerGroup>(m_workingDir, m_tileCount));
                m_layers.back()->parse(*layerChild, map);
            } else {
                LOG("Unidentified name " + attribString + ": node skipped", Logger::Type::Warning);
            }
        }
    }
    return retval;
}

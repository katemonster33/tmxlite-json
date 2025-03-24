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
#include <tmxlite/Map.hpp>
#include <tmxlite/FreeFuncs.hpp>
#include <tmxlite/ObjectGroup.hpp>
#include <tmxlite/ImageLayer.hpp>
#include <tmxlite/TileLayer.hpp>
#include <tmxlite/LayerGroup.hpp>
#include <tmxlite/detail/Log.hpp>
#include <tmxlite/detail/Android.hpp>

#include <queue>

using namespace tmx;

Map::Map()
    : m_orientation (Orientation::None),
    m_renderOrder   (RenderOrder::None),
    m_infinite      (false),
    m_hexSideLength (0.f),
    m_staggerAxis   (StaggerAxis::None),
    m_staggerIndex  (StaggerIndex::None),
    m_compressionLevel(-1)
{

}

//public
bool Map::load(const std::string& path)
{
    std::string contents;
    if (!readFileIntoString(path, &contents))
    {
        Logger::log("Failed to read file " + path, Logger::Type::Error);
        return reset();
    }
    return loadFromString(contents, getFilePath(path));
}

bool Map::loadFromString(const std::string& data, const std::string& workingDir)
{
    reset();

    //open the doc
    cJSON* doc = cJSON_Parse(data.c_str());
    if (!doc) {
        Logger::log("Failed opening map", Logger::Type::Error);
        return false;
    }

    //make sure we have consistent path separators
    m_workingDirectory = workingDir;
    std::replace(m_workingDirectory.begin(), m_workingDirectory.end(), '\\', '/');

    if (!m_workingDirectory.empty() &&
        m_workingDirectory.back() == '/') {
        m_workingDirectory.pop_back();
    }
    bool parseSuccess = parseMapNode(*doc);
    cJSON_Delete(doc);
    return parseSuccess;
}

//private
bool Map::parseMapNode(const cJSON& mapNode)
{
    //parse map attributes
    for(cJSON *child = mapNode.child; child != nullptr; child = child->next) {
        std::string childname = child->string;
        if(childname == "compressionlevel") {
            m_compressionLevel = int(child->valuedouble);
        } else if(childname == "version") {
            std::size_t pointPos = 0;
            std::string versionString = child->valuestring;
            if (versionString.empty() || (pointPos = versionString.find('.')) == std::string::npos) {
                Logger::log("Invalid map version value, map not loaded.", Logger::Type::Error);
                return reset();
            }
        
            m_version.upper = STOI(versionString.substr(0, pointPos));
            m_version.lower = STOI(versionString.substr(pointPos + 1));
        } else if(childname == "class") {
            m_class = child->valuestring;
        } else if(childname == "orientation") {
            std::string orientation = child->valuestring;
            if (orientation == "orthogonal") {
                m_orientation = Orientation::Orthogonal;
            } else if (orientation == "isometric") {
                m_orientation = Orientation::Isometric;
            } else if (orientation == "staggered") {
                m_orientation = Orientation::Staggered;
            } else if (orientation == "hexagonal") {
                m_orientation = Orientation::Hexagonal;
            } else {
                Logger::log(orientation + " format maps aren't supported yet, sorry! Map not loaded", Logger::Type::Error);
                return reset();
            }
        } else if(childname == "renderorder") {
            std::string renderorder = child->valuestring;
            if (renderorder == "right-down") {
                m_renderOrder = RenderOrder::RightDown;
            } else if (renderorder == "right-up") {
                m_renderOrder = RenderOrder::RightUp;
            } else if (renderorder == "left-down") {
                m_renderOrder = RenderOrder::LeftDown;
            } else if (renderorder == "left-up") {
                m_renderOrder = RenderOrder::LeftUp;
            } else {
                Logger::log(renderorder + ": invalid render order. Map not loaded.", Logger::Type::Error);
                return reset();
            }
        } else if(childname == "infinite") {
            m_infinite = child->type == cJSON_True;
        } else if(childname == "width") {
            m_tileCount.x = int(child->valuedouble);
        } else if(childname == "height") {
            m_tileCount.y = int(child->valuedouble);
        } else if(childname == "tilewidth") {
            m_tileSize.x = int(child->valuedouble);
        } else if(childname == "tileheight") {
            m_tileSize.y = int(child->valuedouble);
        } else if(childname == "hexsidelength") {
            m_hexSideLength = float(child->valuedouble);
        } else if(childname == "staggeraxis") {
            std::string staggeraxis = child->valuestring;
            if (staggeraxis == "x") {
                m_staggerAxis = StaggerAxis::X;
            } else if (staggeraxis == "y") {
                m_staggerAxis = StaggerAxis::Y;
            }
        } else if(childname == "staggerindex") {
            std::string staggerindex = child->valuestring;
            if (staggerindex == "odd") {
                m_staggerIndex = StaggerIndex::Odd;
            } else if (staggerindex == "even") {
                m_staggerIndex = StaggerIndex::Even;
            }
        } else if(childname == "parallaxoriginx") {
            m_parallaxOrigin.x = int(child->valuedouble);
        } else if(childname == "parallaxoriginy") {
            m_parallaxOrigin.y = int(child->valuedouble);
        } else if(childname == "backgroundcolor") {
            m_backgroundColour = colourFromString(child->valuestring);
        } else if(childname == "properties") {
            m_properties = Property::readProperties(*child);
        } else if(childname == "layers") {
            m_layers = Layer::readLayers(*child, this);
        } else if(childname == "tilesets") {
            m_tilesets = Tileset::readTilesets(*child, this);
        } else {
            LOG("Unidentified name " + childname + ": node skipped", Logger::Type::Warning);
        }
    }

    if (m_orientation == Orientation::None) {
        Logger::log("Missing map orientation attribute, map not loaded.", Logger::Type::Error);
        return reset();
    } else if(m_tileCount.x == 0 || m_tileCount.y == 0) {
        Logger::log("Invalid map tile count, map not loaded", Logger::Type::Error);
        return reset();
    } else if(m_tileSize.x == 0 || m_tileSize.y == 0) {
        Logger::log("Invalid tile size, map not loaded", Logger::Type::Error);
        return reset();
    } else if (m_orientation == Orientation::Hexagonal && m_hexSideLength <= 0) {
        Logger::log("Invalid he side length found, map not loaded", Logger::Type::Error);
        return reset();
    } else if ((m_orientation == Orientation::Staggered || m_orientation == Orientation::Hexagonal)
        && m_staggerAxis == StaggerAxis::None) {
        Logger::log("Map missing stagger axis property. Map not loaded.", Logger::Type::Error);
        return reset();
    } else if ((m_orientation == Orientation::Staggered || m_orientation == Orientation::Hexagonal)
        && m_staggerIndex == StaggerIndex::None) {
        Logger::log("Map missing stagger index property. Map not loaded.", Logger::Type::Error);
        return reset();
    }

    // fill animated tiles for easier lookup into map
    for(const auto& ts : m_tilesets)
    {
        for(const auto& tile : ts.getTiles())
        {
            if (!tile.animation.frames.empty())
            {
                m_animTiles[tile.ID + ts.getFirstGID()] = tile;
            }
        }
    }

    return true;
}

bool Map::reset()
{
    m_orientation = Orientation::None;
    m_renderOrder = RenderOrder::None;
    m_tileCount = { 0u, 0u };
    m_tileSize = { 0u, 0u };
    m_hexSideLength = 0.f;
    m_staggerAxis = StaggerAxis::None;
    m_staggerIndex = StaggerIndex::None;
    m_backgroundColour = {};
    m_workingDirectory = "";

    m_tilesets.clear();
    m_layers.clear();
    m_properties.clear();

    m_templateObjects.clear();
    m_templateTilesets.clear();

    m_animTiles.clear();

    return false;
}

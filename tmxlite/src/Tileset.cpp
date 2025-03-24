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
#include <tmxlite/Tileset.hpp>
#include <tmxlite/Map.hpp>
#include <tmxlite/FreeFuncs.hpp>
#include <tmxlite/detail/Log.hpp>

#include <ctype.h>

using namespace tmx;

//public
Tileset::Tileset(const std::string& workingDir)
    : m_workingDir          (workingDir),
    m_firstGID              (0),
    m_spacing               (0),
    m_margin                (0),
    m_tileCount             (0),
    m_columnCount           (0),
    m_objectAlignment       (ObjectAlignment::Unspecified),
    m_transparencyColour    (0, 0, 0, 0),
    m_hasTransparency       (false),
    m_doc(nullptr)
{

}

Tileset::~Tileset()
{
    if(m_doc != nullptr){
        cJSON_Delete(m_doc);
        m_doc = nullptr;
    }
}

bool Tileset::loadWithoutMap(const std::string& path)
{
    std::string resolved_path = tmx::resolveFilePath(path, m_workingDir);
    std::string contents;
    if (!readFileIntoString(resolved_path, &contents))
    {
        Logger::log("Failed to read file " + resolved_path, Logger::Type::Error);
        return reset();
    }

    m_workingDir = getFilePath(resolved_path);
    return loadWithoutMapFromString(contents);
}

bool Tileset::loadWithoutMapFromString(const std::string& xmlStr)
{
    if(m_doc != nullptr){
        cJSON_Delete(m_doc);
        m_doc = nullptr;
    }
    m_doc = cJSON_Parse(xmlStr.c_str());
    if (!m_doc)
    {
        Logger::log("Failed to parse tileset XML", Logger::Type::Error);
        //Logger::log("Reason: " + std::string(result.description()), Logger::Type::Error);
        return false;
    }
    cJSON *tilesetNode = nullptr;
    for(cJSON *child = m_doc->child; child != nullptr; child = child->next) {
        if(std::string(child->string) == "tileset") {
            tilesetNode = child;
            break;
        }
    }
    if (!tilesetNode)
    {
        Logger::log("Failed opening tileset: no tileset node found", Logger::Type::Error);
        return reset();
    }

    return parse(*tilesetNode, nullptr);
}

bool Tileset::parse(const cJSON& node, Map* map)
{
    //when parsing as part of a map, we may be looking at an inline node that
    //refers to another file
    if (map) {
        std::string sourcePath;
        for(cJSON *child = node.child; child != nullptr; child = child->next) {
            std::string childname = child->string;
            if(childname == "firstgid") {
                m_firstGID = int(child->valuedouble);
            } else if(childname == "source") {
                sourcePath = child->valuestring;
            }
        }

        if (m_firstGID == 0) {
            Logger::log("Invalid first GID in tileset. Tileset node skipped.", Logger::Type::Warning);
            return false;
        }

        if (!sourcePath.empty()) {
            return loadWithoutMap(sourcePath);
        }
    }
    std::string transparentColor;
    for(cJSON *child = node.child; child != nullptr; child = child->next) {
        std::string name = child->string;
        if(name == "name") {
            m_name = child->valuestring;
            LOG("found tile set " + m_name, Logger::Type::Info);
        } else if(name == "class") {
            m_class = child->valuestring;
        } else if(name == "tilewidth") {
            m_tileSize.x = int(child->valuedouble);
        } else if(name == "tileheight") {
            m_tileSize.y = int(child->valuedouble);
        } else if(name == "spacing") {
            m_spacing = int(child->valuedouble);
        } else if(name == "margin") {
            m_margin = int(child->valuedouble);
        } else if(name == "tilecount") {
            m_tileCount = int(child->valuedouble);
        } else if(name == "columns") {
            m_columnCount = int(child->valuedouble);
        } else if(name == "objectalignment") {
            std::string objectAlignment = child->valuestring;
            if (objectAlignment == "unspecified") {
                m_objectAlignment = ObjectAlignment::Unspecified;
            } else if (objectAlignment == "topleft") {
                m_objectAlignment = ObjectAlignment::TopLeft;
            } else if (objectAlignment == "top") {
                m_objectAlignment = ObjectAlignment::Top;
            } else if (objectAlignment == "topright") {
                m_objectAlignment = ObjectAlignment::TopRight;
            } else if (objectAlignment == "left") {
                m_objectAlignment = ObjectAlignment::Left;
            } else if (objectAlignment == "center") {
                m_objectAlignment = ObjectAlignment::Center;
            } else if (objectAlignment == "right") {
                m_objectAlignment = ObjectAlignment::Right;
            } else if (objectAlignment == "bottomleft") {
                m_objectAlignment = ObjectAlignment::BottomLeft;
            } else if (objectAlignment == "bottom") {
                m_objectAlignment = ObjectAlignment::Bottom;
            } else if (objectAlignment == "bottomright") {
                m_objectAlignment = ObjectAlignment::BottomRight;
            }
        } else if(name == "image") {
            m_imagePath = child->valuestring;
        } else if (name == "tileoffset") {
            parseOffsetNode(*child);
        } else if (name == "properties") {
            m_properties = Property::readProperties(*child);
        } else if (name == "terraintypes") {
            parseTerrainNode(*child);
        } else if (name == "tile") {
            parseTileNode(*child, map);
        } else if(name == "transparentcolor") {
            transparentColor = child->valuestring;
        } else if(name == "imagewidth") {
            m_imageSize.x = int(child->valuedouble);
        } else if(name == "imageheight") {
            m_imageSize.y = int(child->valuedouble);
        }
    }
    
    if (m_tileSize.x == 0 || m_tileSize.y == 0) {
        Logger::log("Invalid tile size found in tile set node. Node will be skipped.", Logger::Type::Error);
        return reset();
    }
    m_tileIndex.reserve(m_tileCount);
    m_tiles.reserve(m_tileCount);
    
    if (m_imagePath.empty()) {
        Logger::log("Tileset image node has missing source property, tile set not loaded", Logger::Type::Error);
        return reset();
    } else {
        //TODO this currently doesn't cover embedded images
        //mostly because I can't figure out how to export them
        //from the Tiled editor... but also resource handling
        //should be handled by the renderer, not the parser.
        m_imagePath = resolveFilePath(m_imagePath, m_workingDir);
        if (!transparentColor.empty()) {
            m_transparencyColour = colourFromString(transparentColor);
            m_hasTransparency = true;
        }
    }

    //if the tsx file does not declare every tile, we create the missing ones
    if (m_tiles.size() != getTileCount()) {
        for (std::uint32_t ID = 0; ID < getTileCount(); ID++) {
            createMissingTile(ID);
        }
    }

    return true;
}

std::uint32_t Tileset::getLastGID() const
{
    assert(!m_tileIndex.empty());
    return m_firstGID + static_cast<std::uint32_t>(m_tileIndex.size()) - 1;
}

const Tileset::Tile* Tileset::getTile(std::uint32_t id) const
{
    if (!hasTile(id))
    {
        return nullptr;
    }
    
    //corrects the ID. Indices and IDs are different.
    id -= m_firstGID;
    id = m_tileIndex[id];
    return id ? &m_tiles[id - 1] : nullptr;
}

//private
bool Tileset::reset()
{
    m_firstGID = 0;
    m_source = "";
    m_name = "";
    m_class = "";
    m_tileSize = { 0,0 };
    m_spacing = 0;
    m_margin = 0;
    m_tileCount = 0;
    m_columnCount = 0;
    m_objectAlignment = ObjectAlignment::Unspecified;
    m_tileOffset = { 0,0 };
    m_properties.clear();
    m_imagePath = "";
    m_transparencyColour = { 0, 0, 0, 0 };
    m_hasTransparency = false;
    m_terrainTypes.clear();
    m_tileIndex.clear();
    m_tiles.clear();
    if(m_doc) {
        cJSON_Delete(m_doc);
        m_doc = nullptr;
    }
    return false;
}

void Tileset::parseOffsetNode(const cJSON& node)
{
    for(cJSON *offsetNode = node.child; offsetNode != nullptr; offsetNode = offsetNode->next) {
        std::string nodeName = offsetNode->string;
        if(nodeName == "x") {
            m_tileOffset.x = int(offsetNode->valuedouble);
        } else if(nodeName == "y") {
            m_tileOffset.y = int(offsetNode->valuedouble);
        }
    }
}

void Tileset::parseTerrainNode(const cJSON& node)
{
    for(cJSON* child = node.child; child != nullptr; child = child->next)
    {
        std::string name = child->string;
        if (name == "terrain")
        {
            m_terrainTypes.emplace_back();
            auto& terrain = m_terrainTypes.back();
            for(cJSON *terrainNode = child->child; terrainNode != nullptr; terrainNode = terrainNode->next) {
                std::string tnodename = terrainNode->string;
                if(tnodename == "name") {
                    terrain.name = terrainNode->valuestring;
                } else if(tnodename == "tile") {
                    terrain.tileID = int(terrainNode->valuedouble);
                } else if(tnodename == "properties") {
                    terrain.properties = Property::readProperties(*terrainNode);
                }
            }
        }
    }
}

Tileset::Tile& Tileset::newTile(std::uint32_t ID)
{
    Tile& tile = (m_tiles.emplace_back(), m_tiles.back());
    if (m_tileIndex.size() <= ID)
    {
        m_tileIndex.resize(ID + 1, 0);
    }

    m_tileIndex[ID] = static_cast<std::uint32_t>(m_tiles.size());
    tile.ID = ID;
    return tile;
}

Tileset Tileset::readTileset(const cJSON& node, tmx::Map* map, bool &parseSuccess)
{
    Tileset output(map->getWorkingDirectory());
    parseSuccess = output.parse(node, map);
    return output;
}

std::vector<Tileset> Tileset::readTilesets(const cJSON& node, tmx::Map* map)
{
    std::vector<Tileset> output;
    for(cJSON *child = node.child; child != nullptr; child = child->next) {
        bool parseSuccess = false;
        auto tileset = readTileset(*child, map, parseSuccess);
        if(parseSuccess) {
            output.push_back(std::move(tileset));
        }
    }
    return output;
}

void Tileset::parseTileNode(const cJSON& node, Map* map)
{
    uint32_t tileId = 0;
    for(cJSON *tileNode = node.child; tileNode != nullptr; tileNode = tileNode->next) {
        if(std::string(tileNode->string) == "id") {
            tileId = int(tileNode->valuedouble);
        }
    }
    Tile& tile = newTile(tileId);

    //by default we set the tile's values as in an Image tileset
    tile.imagePath = m_imagePath;
    tile.imageSize = m_tileSize;
    tile.probability = 100;
    for(cJSON *tileNode = node.child; tileNode != nullptr; tileNode = tileNode->next) {
        std::string name = tileNode->string;
        if(name == "terrain") {
            std::string data = tileNode->valuestring;
            bool lastWasChar = true;
            std::size_t idx = 0u;
            for (auto i = 0u; i < data.size() && idx < tile.terrainIndices.size(); ++i) {
                if (isdigit(data[i])) {
                    tile.terrainIndices[idx++] = std::atoi(&data[i]);
                    lastWasChar = false;
                } else {
                    if (!lastWasChar) {
                        lastWasChar = true;
                    } else {
                        tile.terrainIndices[idx++] = -1;
                        lastWasChar = false;
                    }
                }
            }
            if (lastWasChar) {
                tile.terrainIndices[idx] = -1;
            }
        } else if(name == "probability") {
            tile.probability = int(tileNode->valuedouble);
        } else if(name == "type" || name == "class") {
            tile.className = tileNode->valuestring;
        } else if(name == "properties") {
            tile.properties = Property::readProperties(*tileNode);
        } else if (name == "objectgroup") {
            tile.objectGroup.parse(*tileNode, map);
        } else if (name == "image") {
            if (tileNode->valuestring == nullptr) {
                Logger::log("Tile image path missing", Logger::Type::Warning);
                continue;
            }
            tile.imagePath = resolveFilePath(tileNode->valuestring, m_workingDir);
        } else if (name == "imagewidth") {
            tile.imageSize.x = (unsigned int)tileNode->valuedouble;
        } else if (name == "imageheight") {
            tile.imageSize.y = (unsigned int)tileNode->valuedouble;
        } else if (name == "animation") {
            for(cJSON *animNode = node.child; animNode != nullptr; animNode = animNode->next) {
                Tile::Animation::Frame frame;
                for(cJSON* frameNode = animNode->child; frameNode != nullptr; frameNode = frameNode->next) {
                    std::string fnodename = frameNode->string;
                    if(fnodename == "duration") {
                        frame.duration = int(frameNode->valuedouble);
                    } else if(fnodename == "tileid") {
                        frame.tileID = int(frameNode->valueint) + m_firstGID;
                    }
                }
                tile.animation.frames.push_back(frame);
            }
        } else {
            LOG("Tileset: could not parse node " + name, tmx::Logger::Type::Warning);
        }
    }
    
    if (m_columnCount != 0) 
    {
        std::uint32_t rowIndex = tile.ID % m_columnCount;
        std::uint32_t columnIndex = tile.ID / m_columnCount;
        tile.imagePosition.x = m_margin + rowIndex * (m_tileSize.x + m_spacing);
        tile.imagePosition.y = m_margin + columnIndex * (m_tileSize.y + m_spacing);
    }
}

void Tileset::createMissingTile(std::uint32_t ID)
{
    //first, we check if the tile does not yet exist
    if (m_tileIndex.size() > ID && m_tileIndex[ID])
    {
        return;
    }

    Tile& tile = newTile(ID);
    tile.imagePath = m_imagePath;
    tile.imageSize = m_tileSize;

    std::uint32_t rowIndex = ID % m_columnCount;
    std::uint32_t columnIndex = ID / m_columnCount;
    tile.imagePosition.x = m_margin + rowIndex * (m_tileSize.x + m_spacing);
    tile.imagePosition.y = m_margin + columnIndex * (m_tileSize.y + m_spacing);
}

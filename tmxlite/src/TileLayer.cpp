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
#include <zstd.h>
#else
#include "detail/cJSON.h"
#endif

#ifdef USE_ZSTD
#include <zstd.h>
#endif

#include <tmxlite/FreeFuncs.hpp>
#include <tmxlite/TileLayer.hpp>
#include <tmxlite/detail/Log.hpp>

#include <sstream>

using namespace tmx;


TileLayer::TileLayer(std::size_t tileCount)
    : m_tileCount (tileCount),
    m_compression(CompressionType::None)
{
    m_tiles.reserve(tileCount);
}


bool TileLayer::parseChild(const struct cJSON &child, tmx::Map* map)
{
    std::string childName = child.string;
    if(childName == "data") {
        m_dataNode = &child;
    } else if(childName == "width") {
        m_size.x = (unsigned int)child.valuedouble;
    } else if(childName == "height") {
        m_size.y = (unsigned int)child.valuedouble;
    } else if(childName == "compression") {
        std::string strCompressionType = child.valuestring;
        if(strCompressionType == "zlib") {
            m_compression = CompressionType::Zlib;
        } else if(strCompressionType == "gzip") {
            m_compression = CompressionType::GZip;
        } else if(strCompressionType == "zstd") {
            m_compression = CompressionType::Zstd;
        }
    } else if(childName == "encoding") {
        if(std::string(child.valuestring) == "base64") {
            m_encoding = EncodingType::Base64;
        } else {
            m_encoding = EncodingType::Csv;
        }
    } else {
        return Layer::parseChild(child, map);
    }
    return true;
}

//public
bool TileLayer::parse(const cJSON& node, Map* map)
{
    bool retval = Parsable::parse(node, map);
    if(retval) {
        if(m_dataNode != nullptr) {
            if (m_encoding == EncodingType::Base64) {
                parseBase64(*m_dataNode);
            } else if (m_encoding == EncodingType::Csv) {
                parseCSV(*m_dataNode);
            }
        }
    }
    return retval;
}

std::vector<uint32_t> TileLayer::parseTileIds(std::string dataString, std::size_t tileCount)
{
    std::stringstream ss;
    ss << dataString;
    ss >> dataString;
    dataString = base64_decode(dataString);

    std::size_t expectedSize = tileCount * 4; //4 bytes per tile
    std::vector<unsigned char> byteData;
    byteData.reserve(expectedSize);

    switch (m_compression)
    {
    default:
        byteData.insert(byteData.end(), dataString.begin(), dataString.end());
        break;
    case CompressionType::Zstd:
#if defined USE_ZSTD || defined USE_EXTLIBS
        {
            std::size_t dataSize = dataString.length() * sizeof(unsigned char);
            std::size_t result = ZSTD_decompress(byteData.data(), expectedSize, &dataString[0], dataSize);
            
            if (ZSTD_isError(result))
            {
                std::string err = ZSTD_getErrorName(result);
                LOG("Failed to decompress layer data, node skipped.\nError: " + err, Logger::Type::Error);
            }
        }
#else
        Logger::log("Library must be built with USE_EXTLIBS or USE_ZSTD for Zstd compression", Logger::Type::Error);
        return {};
#endif
    case CompressionType::GZip:
#ifndef USE_EXTLIBS
        Logger::log("Library must be built with USE_EXTLIBS for GZip compression", Logger::Type::Error);
        return {};
#endif
        //[[fallthrough]];
    case CompressionType::Zlib:
    {
        //unzip
        std::size_t dataSize = dataString.length() * sizeof(unsigned char);

        if (!decompress(dataString.c_str(), byteData, dataSize, expectedSize))
        {
            LOG("Failed to decompress layer data, node skipped.", Logger::Type::Error);
            return {};
        }
    }
        break;
    }

    //data stream is in bytes so we need to OR into 32 bit values
    std::vector<std::uint32_t> IDs;
    IDs.reserve(tileCount);
    for (auto i = 0u; i < expectedSize - 3u; i += 4u)
    {
        std::uint32_t id = byteData[i] | byteData[i + 1] << 8 | byteData[i + 2] << 16 | byteData[i + 3] << 24;
        IDs.push_back(id);
    }

    return IDs;
}

bool TileLayer::parseChunks(const cJSON &chunkNode)
{
    //check for chunk nodes
    auto dataCount = 0;
    for(cJSON *child = chunkNode.child; child != nullptr; child = child->next)
    {
        std::string childName = child->string;
        if (childName == "chunk")
        {
            Chunk chunk;
            for(cJSON *chunkNode = child->child; chunkNode != nullptr; chunkNode = chunkNode->next) {
                std::string chunkNodeName = chunkNode->string;
                if(chunkNodeName == "x") {
                    chunk.position.x = int(chunkNode->valuedouble);
                } else if(chunkNodeName == "y") {
                    chunk.position.y = int(chunkNode->valuedouble);
                } else if(chunkNodeName == "width") {
                    chunk.size.x = int(chunkNode->valuedouble);
                } else if(chunkNodeName == "height") {
                    chunk.size.y = int(chunkNode->valuedouble);
                } else if(chunkNodeName == "data") {
                    chunk.dataNode = chunkNode;
                }
            }
            if(chunk.dataNode != nullptr) {
                auto IDs = parseTileIds(chunk.dataNode->valuestring, (chunk.size.x * chunk.size.y));

                if (!IDs.empty())
                {
                    createTiles(IDs, chunk.tiles);
                    m_chunks.push_back(chunk);
                    dataCount++;
                }
            }
        }
    }
    return dataCount != 0;
}

//private
void TileLayer::parseBase64(const cJSON& node)
{

    std::string data = node.valuestring;
    if (data.empty())
    {
        if (!parseChunks(node))
        {
            Logger::log("Layer " + getName() + " has no layer data. Layer skipped.", Logger::Type::Error);
            return;
        }
    }
    else
    {
        auto IDs = parseTileIds(data, m_tileCount);
        createTiles(IDs, m_tiles);
    }
}

void TileLayer::parseCSV(const cJSON& node)
{
    std::vector<uint32_t> tileIds;
    for(cJSON* idElem = node.child; idElem != nullptr; idElem = idElem->next) {
        tileIds.push_back(uint32_t(idElem->valuedouble));
    }

    if (tileIds.empty()) {
        Logger::log("Layer " + getName() + " has no layer data. Layer skipped.", Logger::Type::Error);
        return;
    } else {
        createTiles(tileIds, m_tiles);
    }
}

void TileLayer::createTiles(const std::vector<std::uint32_t>& IDs, std::vector<Tile>& destination)
{
    //LOG(IDs.size() != m_tileCount, "Layer tile count does not match expected size. Found: "
    //    + std::to_string(IDs.size()) + ", expected: " + std::to_string(m_tileCount));
    
    static const std::uint32_t mask = 0xf0000000;
    for (const auto& id : IDs)
    {
        destination.emplace_back();
        destination.back().flipFlags = ((id & mask) >> 28);
        destination.back().ID = id & ~mask;
    }
}

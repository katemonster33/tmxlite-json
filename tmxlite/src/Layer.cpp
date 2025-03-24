#include <tmxlite/Layer.hpp>
#include <tmxlite/ObjectGroup.hpp>
#include <tmxlite/TileLayer.hpp>
#include <tmxlite/ImageLayer.hpp>
#include <tmxlite/LayerGroup.hpp>
#include <tmxlite/Map.hpp>
#include <tmxlite/FreeFuncs.hpp>
#include <tmxlite/detail/Log.hpp>
#include <cJSON/cJSON.h>

bool tmx::Layer::parseChild(const cJSON& child, Map* map)
{
    std::string childName = child.string;
    if(childName == "name") {
        setName(child.valuestring);
    } else if(childName == "id") {
        m_id = int(child.valuedouble);
    } else if(childName == "type") {
        // skip this, we already know the type
    } else if(childName == "class") {
        setClass(child.valuestring);
    } else if(childName == "opacity") {
        setOpacity(float(child.valuedouble));
    } else if(childName == "offsetx") {
        m_offset.x = int(child.valuedouble);
    } else if(childName == "offsety") {
        m_offset.y = int(child.valuedouble);
    } else if(childName == "x" || childName == "y") {
        // skip these too, spec says always 0
    } else if(childName == "startx") {
        m_start.x = float(child.valuedouble);
    } else if(childName == "starty") {
        m_start.y = float(child.valuedouble);
    } else if(childName == "parallaxx") {
        m_parallaxFactor.x = float(child.valuedouble);
    } else if(childName == "parallaxy") {
        m_parallaxFactor.y = float(child.valuedouble);
    } else if(childName == "tintcolour") {
        m_tintColour = colourFromString(child.valuestring);
    } else if(childName == "properties") {
        m_properties = Property::readProperties(child);
    } else if(childName == "visible") {
        m_visible = child.type == cJSON_True;
    } else {
        return false;
    }
    return true;
}

std::unique_ptr<tmx::Layer> tmx::Layer::readLayer(const cJSON &node, tmx::Map *map)
{
    std::string name;
    for(cJSON *child = node.child; child != nullptr; child = child->next) {
        if(std::string(child->string) == "type") {
            name = child->valuestring;
        }
    }
    std::unique_ptr<tmx::Layer> output;
    if (name == "layer" || name == "tilelayer") {
        output.reset(new tmx::TileLayer(map->getTileCount().x * map->getTileCount().y));
    } else if (name == "objectgroup") {
        output.reset(new ObjectGroup());
    } else if (name == "imagelayer") {
        output.reset(new ImageLayer(map->getWorkingDirectory()));
    } else if (name == "group") {
        output.reset(new LayerGroup());
    } else {
        LOG("Unidentified name " + name + ": node skipped", Logger::Type::Warning);
    }
    if(output) {
        output->parse(node, map);
    }
    return output;
}

std::vector<std::unique_ptr<tmx::Layer>> tmx::Layer::readLayers(const struct cJSON &node, tmx::Map* map)
{
    std::vector<std::unique_ptr<tmx::Layer>> output;
    for(cJSON *child = node.child; child != nullptr; child = child->next) {
        auto outLayer = readLayer(*child, map);
        if(outLayer) {
            output.push_back(std::move(outLayer));
        }
    }
    return output;
}
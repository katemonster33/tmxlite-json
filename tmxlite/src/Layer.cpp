#include <tmxlite/Layer.hpp>
#include <tmxlite/FreeFuncs.hpp>
#include <cJSON.h>

bool tmx::Layer::parseChild(const cJSON& child, Map* map)
{
    std::string childName = child.string;
    if(childName == "name") {
        setName(child.valuestring);
    } else if(childName == "class") {
        setClass(child.valuestring);
    } else if(childName == "opacity") {
        setOpacity(float(child.valuedouble));
    } else if(childName == "offsetx") {
        m_offset.x = int(child.valuedouble);
    } else if(childName == "offsety") {
        m_offset.y = int(child.valuedouble);
    } else if(childName == "parallaxx") {
        m_parallaxFactor.x = float(child.valuedouble);
    } else if(childName == "parallaxy") {
        m_parallaxFactor.y = float(child.valuedouble);
    } else if(childName == "tintcolour") {
        m_tintColour = colourFromString(child.valuestring);
    } else if(childName == "properties") {
        for(cJSON *propertyNode = child.child; propertyNode != nullptr; propertyNode = propertyNode++) {
            m_properties.emplace_back();
            m_properties.back().parse(*propertyNode);
        }
    } else {
        return false;
    }
    return true;
}
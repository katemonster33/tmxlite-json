#include <cJSON.h>

#include <tmxlite/Parsable.hpp>
#include <tmxlite/Map.hpp>
#include <tmxlite/detail/Log.hpp>
#include <sstream>

bool Parsable::parse(const cJSON &node, tmx::Map *map)
{
    bool retval = true;
    for(cJSON *child = node.child; child != nullptr; child = child->next) {
        if(!parseChild(*child, map)) {
            std::stringstream logmsg;
            logmsg << "Failed to parse node: " << child->string;
            LOG(logmsg.str().c_str(), tmx::Logger::Type::Error);
            retval = false;
        }
    }
    return retval;
}
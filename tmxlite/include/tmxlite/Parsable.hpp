#pragma once

#include "Config.hpp"

namespace tmx
{
    class Map;
}

class TMXLITE_EXPORT_API Parsable
{
protected:
    virtual bool parseChild(const struct cJSON &child, tmx::Map* map) = 0;
public:
    virtual bool parse(const struct cJSON &node, tmx::Map* map);
};
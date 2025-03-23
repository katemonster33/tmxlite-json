#pragma once

namespace tmx
{
    class Map;
}

class Parsable
{
protected:
    virtual bool parseChild(const struct cJSON &child, tmx::Map* map) = 0;
public:
    virtual bool parse(const struct cJSON &node, tmx::Map* map);
};
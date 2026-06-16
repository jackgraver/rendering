#ifndef BLOCK_H
#define BLOCK_H

#include <glm/glm.hpp>

enum BLOCK_TYPE {
    AIR,
    DIRT,
    STONE,
};

inline glm::vec3 blockTypeColor(BLOCK_TYPE type) {
    switch (type) {
        case AIR:
            return glm::vec3(1.0f, 1.0f, 1.0f);
        case DIRT:
            return glm::vec3(0.39f, 0.25f, 0.09f);
        case STONE:
            return glm::vec3(0.66f, 0.66f, 0.66f);
    }

    return glm::vec3(1.0f, 1.0f, 1.0f);
}

struct Block {
    Block() = default;
    Block(BLOCK_TYPE type)
        : type(type) {
    }

    BLOCK_TYPE type = AIR;
};

#endif

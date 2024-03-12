#pragma once

#include "character-data.h"

namespace Web::Dom {

struct Comment : public CharacterData {
    virtual NodeType nodeType() override {
        return NodeType::COMMENT_NODE;
    }
};

} // namespace Web::Dom

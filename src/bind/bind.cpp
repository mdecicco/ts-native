#include <bind/bind.h>

namespace gjs {
    namespace bind {
        wrapped_class::~wrapped_class() {
            for (auto i = properties.begin();i != properties.end();++i) {
                delete i->getSecond();
            }
            properties.clear();
        }
    };
};
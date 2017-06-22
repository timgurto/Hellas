#include "testing.h"
#include "../client/Texture.h"

TEST_CASE("Use texture after removing all others"){
    static const std::string IMAGE_FILE = "testing/tiny.png";
    {
        Texture t(IMAGE_FILE);
    }
    {
        Texture t(IMAGE_FILE);
    }
}

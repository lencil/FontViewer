#include "FXLib.h"
#include "FXFace.h"
#include <iostream>

std::string getFontFile(const std::string & fileName) {
    return std::string(FX_RESOURCES_DIR) + "/Fonts/" + fileName;
}

std::string getUCDRoot() {
    return std::string(FX_RESOURCES_DIR) + "/UCD";
}

int main() {
    FXLib::init(getUCDRoot()); {
        auto face = FXFace::createFace(getFontFile("MyriadPro-Regular.otf"), 0);
        assert(face);

        assert(face->face());
        assert(face->index() == 0);
        assert(face->postscriptName() == "MyriadPro-Regular");
        assert(face->upem() == 1000);

        FXCMap cm = face->currentCMap();
        assert(cm.isUnicode());

        for (const FXCMap & cm : face->cmaps()) {
            std::cout << cm.description()  << ": " << std::endl;
            for (const auto & block : cm.blocks()) {
                std::cout << " - " << block->name() << ": " << block->size() << std::endl;
            };
        }

    } FXLib::finish();
    return 0;
}

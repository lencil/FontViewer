#include "FXCMap.h"
#include "FXUnicode.h"
#include "FXFTPrivate.h"
#include "FXFTNames.h"

namespace {
    constexpr FXChar UNDEFINED_CHAR_MARK = FXCharInvalid;
    constexpr FXChar EXTRA_CHARS_MARK = UNDEFINED_CHAR_MARK + 1;
}

std::vector<FXCMapPlatform> FXCMapPlatform::platforms_;
std::vector<FXPtr<FXCharBlock> > FXCMapPlatform::unicodeBlocks_;

const std::vector<FXCMapPlatform> &
FXCMapPlatform::availablePlatforms() {
    if (platforms_.empty()) {
        platforms_.push_back(TT_PLATFORM_APPLE_UNICODE);
        platforms_.push_back(TT_PLATFORM_MACINTOSH);
        platforms_.push_back(TT_PLATFORM_ISO);
        platforms_.push_back(TT_PLATFORM_MICROSOFT);
        platforms_.push_back(TT_PLATFORM_ADOBE);
    }
    return platforms_;
}

const FXCMapPlatform &
FXCMapPlatform::get(uint16_t platformID) {
    for (const auto & platform : availablePlatforms())
        if (platform.platformID_ == platformID)
            return platform;

    return get(TT_PLATFORM_APPLE_UNICODE);
}

const std::vector<FXPtr<FXCharBlock> > &
FXCMapPlatform::blocks(uint16_t encodingID) const {
    auto itr = blocksMap_.find(encodingID);
    if (itr != blocksMap_.end())
        return itr->second;


    static std::vector<FXPtr<FXCharBlock> > empty;
    return empty;
}

FXCMapPlatform::FXCMapPlatform(uint16_t platformID)
    : platformID_(platformID) {
    initEncodings();
}

void
FXCMapPlatform::initEncodings() {
    switch(platformID_) {
        case TT_PLATFORM_APPLE_UNICODE: initUnicodeEncoding(); break;
        case TT_PLATFORM_MACINTOSH: initMacintoshEncoding(); break;
        case TT_PLATFORM_ISO: initISOEncoding(); break;
        case TT_PLATFORM_MICROSOFT: initMicrosoftEncoding(); break;
        case TT_PLATFORM_ADOBE: initAdobeEncoding(); break;
        default: initUnicodeEncoding(); break;
    }
}

void
FXCMapPlatform::initUnicodeEncoding() {
    auto unicodeBlocks = getUnicodeBlocks();
    uint16_t encodings[] = {TT_APPLE_ID_DEFAULT,
                            TT_APPLE_ID_UNICODE_1_1,
                            TT_APPLE_ID_ISO_10646,
                            TT_APPLE_ID_UNICODE_2_0,
                            TT_APPLE_ID_UNICODE_32,
                            TT_APPLE_ID_VARIANT_SELECTOR,
                            TT_APPLE_ID_FULL_UNICODE};
    for (auto encoding : encodings)
        blocksMap_[encoding] = unicodeBlocks;
}

void
FXCMapPlatform::initMacintoshEncoding() {
    blocksMap_[TT_MAC_ID_ROMAN].push_back(std::make_shared<FXCharRangeBlock>(0, 255, "Mac Roman", false));
}

void
FXCMapPlatform::initISOEncoding() {
    initUnicodeEncoding();
}

void
FXCMapPlatform::initMicrosoftEncoding() {
    blocksMap_[TT_MS_ID_UNICODE_CS] = getUnicodeBlocks();
    blocksMap_[TT_MS_ID_UCS_4] = getUnicodeBlocks();
    blocksMap_[TT_MS_ID_SYMBOL_CS].push_back(std::make_shared<FXCharRangeBlock>(0xF020, 0xF0FF, "Windows Symbol"));
}

void
FXCMapPlatform::initAdobeEncoding() {
    blocksMap_[TT_ADOBE_ID_STANDARD].push_back(std::make_shared<FXCharRangeBlock>(0, 255, "Standard", false));
    blocksMap_[TT_ADOBE_ID_EXPERT].push_back(std::make_shared<FXCharRangeBlock>(0, 255, "Expert", false));
    blocksMap_[TT_ADOBE_ID_CUSTOM].push_back(std::make_shared<FXCharRangeBlock>(0, 255, "Custom", false));
    blocksMap_[TT_ADOBE_ID_LATIN_1].push_back(std::make_shared<FXCharRangeBlock>(0, 255, "Latin 1", false));
}

const std::vector<FXPtr<FXCharBlock> > &
FXCMapPlatform::getUnicodeBlocks() {
    if (unicodeBlocks_.size())
        return unicodeBlocks_;

    for (const FXUCDBlock & ucdBlock : FXUnicode::blocks()) 
        unicodeBlocks_.push_back(std::make_shared<FXCharRangeBlock>(
                                     ucdBlock.from,
                                     ucdBlock.to,
                                     ucdBlock.name,
                                     true));
    return unicodeBlocks_;
}

//////////////////////////////////////////////////////////////////////////////////////////
//            FXCMAP
FXCMap::FXCMap(FXFTFace face, uint16_t platformID, uint16_t encodingID)
    : face_(face)
    , platformID_(platformID)
    , encodingID_(encodingID) {
    initGlyphsMap();
}

uint16_t
FXCMap::platformID() const {
    return platformID_;
}

uint16_t
FXCMap::encodingID() const {
    return encodingID_;
}

std::string
FXCMap::platformName() const {
    return FXPlatformName(platformID_);
}
    
std::string
FXCMap::encodingName() const {
    return FXEncodingName(platformID_, encodingID_);
}

std::string
FXCMap::description() const {
    return FXPlatformName(platformID_) + " - " + FXEncodingName(platformID_, encodingID_);
}

bool
FXCMap::isUnicode() const {
    return platformID_ == TT_PLATFORM_APPLE_UNICODE
        || (platformID_ == TT_PLATFORM_MICROSOFT && (encodingID_ == TT_MS_ID_UNICODE_CS || encodingID_ == TT_MS_ID_UCS_4))
        ;
}

const std::vector<FXPtr<FXCharBlock> > & 
FXCMap::blocks() const {
    return FXCMapPlatform::get(platformID_).blocks(encodingID_);
}

FXVector<FXChar>
FXCMap::charsForGlyph(FXGlyphID gid) const {
    FXVector<FXChar> ret;

    FXChar c = glyphMap_[gid];
    if (c == UNDEFINED_CHAR_MARK)
        return ret;
    else if (c == EXTRA_CHARS_MARK) {
        auto itr = extraGlyphsMap_.find(gid);
        if (itr != extraGlyphsMap_.end())
            return itr->second;
        else
            return ret;
    }
    else {
        ret.push_back(c);
        return ret;
    }
}

void
FXCMap::initGlyphsMap() {
    glyphMap_ = FXVector<FXChar>(face_->num_glyphs, UNDEFINED_CHAR_MARK);
    FT_UInt gid = 0;
    FT_ULong ch = FT_Get_First_Char(face_, &gid);
    while (gid != 0) {
        FXChar c = glyphMap_[gid];
        if (c == UNDEFINED_CHAR_MARK)
            glyphMap_[gid] = ch;
        else {
            glyphMap_[gid] = EXTRA_CHARS_MARK;
            extraGlyphsMap_[gid].push_back(ch);
        }
        ch = FT_Get_Next_Char(face_, ch, &gid);
    }
}
    

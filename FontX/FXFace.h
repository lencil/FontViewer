#pragma once
#include "FX.h"
#include "FXCMap.h"
#include "FXGlyph.h"

struct FXFaceDescriptor {
    std::string  filePath;
    size_t       faceIndex;
};

struct FXFaceAttributes {
    size_t      index;
    std::string postscriptName;
    size_t      upem;
    size_t      glyphCount;
    
    std::map<std::string, std::string> familyNames;
    std::map<std::string, std::string> styleNames;
    std::map<std::string, std::string> fullNames;
};

constexpr double FXDefaultFontSize = 200;
constexpr double FXDefaultDPI      = 72;

double
pt2px(double p, double dpi = FXDefaultDPI);

double
px2pt(double p, double dpi = FXDefaultDPI);
    
class FXFace : public std::enable_shared_from_this<FXFace> {
public:
    static FXPtr<FXFace>
    createFace(const FXFaceDescriptor & descriptor);
    
    static FXPtr<FXFace>
    createFace(const std::string & filePath, size_t faceIndex);
    
public:
    FXFTFace
    face() const;
    
    size_t
    index() const;

    std::string
    postscriptName() const;

    size_t
    upem() const;

    size_t
    glyphCount() const;
    
    const FXFaceAttributes &
    attributes() const;

public:
    const std::vector<FXCMap> &
    cmaps() const;

    const FXCMap &
    currentCMap() const;

    size_t
    currentCMapIndex() const;

    bool
    selectCMap(size_t cmapIndex);

    bool
    selectCMap(const FXCMap & cmap);

public:
    FXGlyph
    glyph(FXChar c, bool isGID = false);

    /**
     * return the chars which maps to the gid in current cmap
     */
    const FXVector<FXChar> &
    charsForGlyph(FXGlyphID gid) const;
    
private:
    FXFace(const FXFaceDescriptor & descriptor);
    
    FXFace(const FXFace &) = delete;
    FXFace & operator=(const FXFace & ) = delete;

    bool
    init();

    bool
    initAttributes();

    bool
    initCMap();
protected:
    FXFaceDescriptor     desc_;
    FXFTFace             face_;
    FXFaceAttributes     atts_;
    std::vector<FXCMap>  cmaps_;
};

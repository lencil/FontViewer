#pragma once
#include "FX.h"

struct FXUCDBlock {
    FXChar    from;
    FXChar    to;
    FXString  name;

    bool
    contains(FXChar c) const {
        return c >= from && c <= to;
    }
};

struct FXCharCategory {
    int        type;      // UCharCategory
    FXString   abbreviation;
    FXString   fullName;
};

extern FXUCDBlock FXUCDInvalidBlock;

class FXUCD {
public:
    explicit FXUCD(const FXString & root);

protected:
    const FXVector<FXUCDBlock> &
    blocks() const;

    const FXUCDBlock &
    block(FXChar c) const;

    const FXVector<FXCharCategory> &
    categories() const;

    friend struct FXUnicode;
protected:
    FXString
    file(const FXString & name) const;
protected:
    FXString                          root_;
    mutable FXVector<FXUCDBlock>      blocks_;
    mutable FXVector<FXCharCategory>  categories_;
};

struct FXUnicode {
public:
    static void
    init(const FXString & ucdRoot);
    
    static FXPtr<FXUCD>
    ucd();

    static const FXVector<FXUCDBlock> &
    blocks();

    static const FXUCDBlock &
    block(FXChar c);
  
    static FXString
    name(FXChar c);

    static FXString
    script(FXChar c);

    static bool
    defined(FXChar c);

    static FXString
    age(FXChar c);

    static const FXCharCategory &
    category(FXChar c);

    static FXVector<FXChar>
    decomposition(FXChar c, bool nfkd = false);

    static FXVector<uint8_t>
    utf8(FXChar c);

    static FXVector<uint16_t>
    utf16(FXChar c);
    
private:
    static FXPtr<FXUCD>    ucd_;
};

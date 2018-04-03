#include <iostream>

#include "FXUCD.h"
#include "FXBoostPrivate.h"

namespace {
    class FXUCDFileReader {
    public:
        FXUCDFileReader(const FXString & filePath, bool ignoreBlankLine = true, FXChar commentChar = '#')
            : filePath_(filePath)
            , ignoreBlankLine_(ignoreBlankLine)
            , commentChar_(commentChar)
            , stream_(filePath_)
        {}

        bool
        readLine(FXString & line) {
            while(std::getline(stream_, line)) {
                boost::algorithm::trim(line);
                if (commentChar_ && line.size() && line[0] == commentChar_)
                    continue;
                if (ignoreBlankLine_ && line.empty())
                    continue;
                return true;
            }

            return false;
        }

        static bool
        parseLine(const FXString & line, FXCharRange & range, FXString & text) {
            FXVector<FXString> strs;
            boost::split(strs, line, boost::is_any_of(";"));
            if (strs.size() == 2) {
                if (parseCharRange(boost::algorithm::trim_copy(strs[0]), range)) {
                    text = boost::algorithm::trim_copy(strs[1]);
                    return true;
                }
            }
            return false;
        }

        static bool
        parseCharRange(const FXString & line, FXCharRange & range) {
            FXVector<FXString> strs;
            boost::split_regex(strs, line, boost::regex("\\.\\."));
            if (strs.size() == 1) {
                FXChar c;
                if (parseHexChar(strs[0], c)) {
                    range.from = range.to = c;
                    return true;
                }
            }
            else if (strs.size() == 2) {
                FXChar c1, c2;
                if (parseHexChar(strs[0], c1) && parseHexChar(strs[1], c2)) {
                    range.from = c1;
                    range.to = c2;
                    return true;
                }
            }
            return false;
        }

        static bool
        parseHexChar(const FXString & str, FXChar & c) {
            size_t pos = 0;
            FXChar result = std::stoul(str, &pos, 16);
            if (pos != str.length())
                return false;
            c = result;
            return true;
        }
        
    protected:
        FXString      filePath_;
        bool          ignoreBlankLine_;
        FXChar        commentChar_;
        std::ifstream stream_;
    };
}

FXUCDBlock FXUCDInvalidBlock {FXCharInvalid, FXCharInvalid, "Invalid Block"};

FXUCD::FXUCD(const FXString & root)
    : root_(root) {}

const FXVector<FXUCDBlock> &
FXUCD::blocks() const {
    if (blocks_.empty()) {
        // load blocks
        FXUCDFileReader reader(file("Blocks.txt"));
        FXString line;
        
        while (reader.readLine(line)) {
            FXCharRange range;
            FXString text;
            if (reader.parseLine(line, range, text)) {
                FXUCDBlock block {range.from, range.to, text};
                blocks_.push_back(block);
            }
        }
    }
    return blocks_;
}

const FXUCDBlock &
FXUCD::block(FXChar c) const {
    auto const & blks = blocks();
    auto itr = std::lower_bound(blks.begin(), blks.end(), c, [](const FXUCDBlock & block, FXChar c) {
        return block.from < c;
    });
    if ((itr != blks.end()) && (itr->contains(c)))
        return *itr;
    return FXUCDInvalidBlock;
}


FXString
FXUCD::file(const FXString & name) const {
    return BFS::pathJoin({root_, name});
}
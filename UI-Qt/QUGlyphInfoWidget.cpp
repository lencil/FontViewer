#include "FontX/FXUnicode.h"

#include "QUGlyphInfoWidget.h"
#include "QUConv.h"
#include "QUHtmlTemplate.h"
#include "QUEncoding.h"
#include "QUResource.h"

#include <QImage>
#include <QTextDocument>
#include <QDebug>

namespace {
    QMap<QString, QVariant>
    templateValues(const FXGlyph & glyph) {
        const FXChar c = glyph.character.value;
        
        QMap<QString, QVariant> map;
        map["NAME"] = toQString(glyph.name);
        map["CHAR"] = QUEncoding::charHexNotation(glyph.character);
        map["ID"] = glyph.gid;

        // Metrics
        map["WIDTH"] = glyph.metrics.width;
        map["HEIGHT"] = glyph.metrics.height;
        map["HORI_ADVANCE"] = glyph.metrics.horiAdvance;
        map["HORI_BEARING_X"] = glyph.metrics.horiBearingX;
        map["HORI_BEARING_Y"] = glyph.metrics.horiBearingY;
        map["VERT_ADVANCE"] = glyph.metrics.vertAdvance;
        map["VERT_BEARING_X"] = glyph.metrics.vertBearingX;
        map["VERT_BEARING_Y"] = glyph.metrics.vertBearingY;

        // Unicode
        bool isDefined = glyph.character.isUnicode() && FXUnicode::defined(c);
        map["UNICODE_NAME"] = isDefined? toQString(FXUnicode::name(c)): QString();
        map["UNICODE_BLOCK"] = isDefined? toQString(FXUnicode::block(c).name): QString();
        map["UNICODE_SCRIPT"] = isDefined? toQString(FXUnicode::script(c)) : QString();
        map["UNICODE_AGE"] = isDefined? toQString(FXUnicode::age(c)) : QString();
        map["UNICODE_GENERAL_CATEGORY"] = isDefined? toQString(FXUnicode::category(c).fullName): QString();

        QStringList decompositionLinks;
        for (FXChar d : FXUnicode::decomposition(c)) {
            decompositionLinks << QString("<a href=\"%1\">%2</a>")
                .arg(QUEncoding::charHexLink({d, FXGCharTypeUnicode}).toDisplayString())
                .arg(QUEncoding::charHexNotation({d, FXGCharTypeUnicode}));
        }
        
        map["UNICODE_DECOMPOSITION"] = isDefined? decompositionLinks.join(", "): QString();

        // Encoding
        QStringList utf8;
        for (auto b : FXUnicode::utf8(c))
            utf8 << QString("%1").arg(b, 2, 16, QChar('0')).toUpper();
        
        QStringList utf16;
        for (auto s : FXUnicode::utf16(c))
            utf16 << QString("%1").arg(s, 4, 16, QChar('0')).toUpper();

        map["UTF8"] = isDefined? utf8.join(" "): QString();
        map["UTF16"] = isDefined? utf16.join(" "): QString();
        return map;
    };
}

QUGlyphInfoWidget::QUGlyphInfoWidget(QWidget *parent)
    : QTextBrowser(parent)
    , document_(nullptr)
    , char_(FXGCharInvalid) {
    connect(this, &QTextBrowser::anchorClicked,
            this, &QUGlyphInfoWidget::onLinkClicked);
    setOpenLinks(false);
}

QUDocument *
QUGlyphInfoWidget::document() const {
    return document_;
}

void
QUGlyphInfoWidget::setQUDocument(QUDocument * document) {
    document_ = document;
}

void
QUGlyphInfoWidget::setChar(FXGChar c) {
    char_ = c;
    loadGlyph();
}

void
QUGlyphInfoWidget::loadGlyph() {
    
    FXGlyph glyph = document_->face()->glyph(char_);
    
    QTextDocument * qdoc = new QTextDocument;
    qdoc->addResource(QTextDocument::ImageResource,
                      QUrl("fv://glyph.png"), QVariant(placeGlyphImage(glyph, glyphEmSize())));
    
    setDocument(qdoc);
    
    QUHtmlTemplate html(QUResource::path("/Html/GlyphInfoTemplate.html"));
    setHtml(html.instantialize(templateValues(glyph)));
}
    
void
QUGlyphInfoWidget::onLinkClicked(const QUrl & link) {
    FXGChar c = QUEncoding::charFromLink(link);
    if (c != FXGCharInvalid)
        emit charLinkClicked(c);
}

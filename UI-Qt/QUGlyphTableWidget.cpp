#include <QItemDelegate>
#include <QStyledItemDelegate>
#include <QStyleOptionProgressBar>
#include <QStyleOptionFocusRect>
#include <QFile>
#include <QTextStream>
#include <QPainter>
#include "QUConv.h"
#include "QUEncoding.h"
#include "QUDocument.h"
#include "QUGlyphTableWidget.h"
#include "ui_QUGlyphTableWidget.h"

class QUGlyphTableModelColumn : public QObject {
public:
    explicit QUGlyphTableModelColumn(const QString & name, QObject * parent = nullptr)
        : QObject(parent)
        , name_(name)
        , useDecorationRole_(false) {}
        
    virtual QVariant
    header(int role) const {
        if (role == Qt::DisplayRole) 
            return name();
        return QVariant();;
    }
        
    virtual QVariant
    data(const FXGlyph & g, int role) const {
        if (useDecorationRole_ && role == Qt::DecorationRole)
            return value(g);
        
        if (role == Qt::DisplayRole) 
            return value(g);
        return QVariant();
    }

    virtual QString
    name() const {
        return name_;
    };

    virtual QVariant
    value(const FXGlyph & g) const = 0;

    QUGlyphTableModelColumn *
    useDecorationRole(bool value = true) {
        useDecorationRole_ = value;
        return this;
    }
    
protected:
    QString      name_;
    bool         useDecorationRole_;
};

class QUGlyphTableBitmapDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void 
    paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const {
        //return QStyledItemDelegate::paint(painter, option, index);
        painter->save();
        
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        QIcon::Mode mode = QIcon::Normal;
        if (!(opt.state & QStyle::State_Enabled))
            mode = QIcon::Disabled;
        else if (opt.state & QStyle::State_Selected)
            mode = QIcon::Selected;
        QIcon::State state = opt.state & QStyle::State_Open ? QIcon::On : QIcon::Off;
        
        QPixmap bk(opt.rect.size());
        bk.fill(Qt::white);
        QIcon bkIcon(bk);
        bkIcon.paint(painter, opt.rect, Qt::AlignHCenter, mode, state);
        
        opt.icon.paint(painter, opt.rect, Qt::AlignHCenter, mode, state);

        painter->restore();
    }
};

namespace {

    template <typename T>
    QVariant
    toVariant(T value) {
        return value;
    }

    QVariant
    toVariant(const FXString & str) {
        return toQString(str);
    }

    QVariant
    toVariant(const FXGChar & c) {
        if (c.isUnicode() && c.value != FXCharInvalid) {
            char32_t i = c.value;
            return QString::fromUcs4(&i, 1);
        }
        return QVariant();
    }

    QVariant
    toVariant(const FXBitmapARGB & bm) {
        return placeImage(toQImage(bm), glyphEmSize());
    }
    
    template <class PointerToMember>
    class QUGlyphBasicColumn : public QUGlyphTableModelColumn {
    public:
        QUGlyphBasicColumn(const QString & name, PointerToMember p, QObject * parent = nullptr)
            : QUGlyphTableModelColumn(name, parent)
            , p_(p)
        {}
        
        virtual QVariant
        value(const FXGlyph & g) const {
            return toVariant(g.*p_);
        };
    protected:
        PointerToMember p_;
    };
    
    template <class PointerToMember>
    class QUGlyphMetricColumn : public QUGlyphTableModelColumn {
    public:
        QUGlyphMetricColumn(const QString & name, PointerToMember p, QObject * parent = nullptr)
            : QUGlyphTableModelColumn(name, parent)
            , p_(p) {}

        virtual QVariant
        value(const FXGlyph & g) const {
            return toVariant(g.metrics.*p_);
        };
    protected:
        PointerToMember p_;
    };

    
    class QUGlyphCodePointColumn : public QUGlyphTableModelColumn {
    public:
        using QUGlyphTableModelColumn::QUGlyphTableModelColumn;
        QVariant
        value(const FXGlyph & g) const {
            if (g.character.value != FXCharInvalid)
                return QUEncoding::charHexNotation(g.character);
            return QVariant();
        }
    };
    
    template <typename PointerToMember>
    QUGlyphBasicColumn<PointerToMember> *
    makeBasicColumn(const QString & name, PointerToMember p, QObject * parent) {
        return new QUGlyphBasicColumn<PointerToMember>(name, p, parent);
    }

    
    template <typename PointerToMember>
    QUGlyphMetricColumn<PointerToMember> *
    makeMetricColumn(const QString & name, PointerToMember p, QObject * parent) {
        return new QUGlyphMetricColumn<PointerToMember>(name, p, parent);
    }

}

QUGlyphTableModel::QUGlyphTableModel(QUDocument * document, QObject * parent)
    : QAbstractTableModel(parent)
    , document_(document) {

    columns_.append(makeBasicColumn(tr("Index"), &FXGlyph::gid, this));
    columns_.append(makeBasicColumn(tr("Glyph"), &FXGlyph::bitmap, this)->useDecorationRole(true));
    columns_.append(new QUGlyphCodePointColumn(tr("Codepoint"), this));
    columns_.append(makeBasicColumn(tr("Char"), &FXGlyph::character, this));
    columns_.append(makeBasicColumn(tr("Name"), &FXGlyph::name, this));
    columns_.append(makeMetricColumn(tr("Width"), &FXGlyphMetrics::width, this));
    columns_.append(makeMetricColumn(tr("Height"), &FXGlyphMetrics::height, this));
    columns_.append(makeMetricColumn(tr("Hori Bearing X"), &FXGlyphMetrics::horiBearingX, this));
    columns_.append(makeMetricColumn(tr("Hori Bearing Y"), &FXGlyphMetrics::horiBearingY, this));
    columns_.append(makeMetricColumn(tr("Hori Advance"), &FXGlyphMetrics::horiAdvance, this));
    columns_.append(makeMetricColumn(tr("Vert Bearing X"), &FXGlyphMetrics::vertBearingX, this));
    columns_.append(makeMetricColumn(tr("Vert Bearing Y"), &FXGlyphMetrics::vertBearingY, this));
    columns_.append(makeMetricColumn(tr("Vert Advance"), &FXGlyphMetrics::vertAdvance, this));

    connect(document_, &QUDocument::cmapActivated,
            this, &QUGlyphTableModel::reset);
}

int
QUGlyphTableModel::rowCount(const QModelIndex & parent) const {
    return document_->face()->glyphCount();
}

int
QUGlyphTableModel::columnCount(const QModelIndex & parent) const {
    return columns_.size();
}
    
QVariant
QUGlyphTableModel::data(const QModelIndex & index, int role) const {
    int gid = index.row();
    FXGChar gc(FXGCharTypeGlyphID, gid);
    FXGlyph g = document_->face()->glyph(gc);
    return columns_[index.column()]->data(g, role);
}

QVariant
QUGlyphTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal) 
        return columns_[section]->header(role);
    return QVariant();
}

const QVector<QUGlyphTableModelColumn *> &
QUGlyphTableModel::columns() const {
    return columns_;
}

void
QUGlyphTableModel::reset() {
    beginResetModel();
    endResetModel();
}

bool
QUGlyphTableModel::exportToFile(const QString & filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream ts(&file);
    for (int i = 0; i < columns_.size(); ++ i) {
        if (i == 1) continue; // skip the glyph image column
        ts << columns_[i]->name() << ", ";
    }

    for (size_t i = 0; i < document_->face()->glyphCount(); ++ i) {
        FXGChar gc(FXGCharTypeGlyphID, i);
        FXGlyph g = document_->face()->glyph(gc);
        for (int i = 0; i < columns_.size(); ++ i) {
            if (i == 1) continue;
            QVariant v = columns_[i]->data(g, Qt::DisplayRole);
            ts << v.toString() << ", ";
        }
        ts << "\n";
    }
    return true;
}

QUGlyphTableWidget::QUGlyphTableWidget(QUDocument * document, QWidget *parent)
    : QWidget(parent)
    , ui_(new Ui::QUGlyphTableWidget)
    , document_(document) {
    ui_->setupUi(this);

    ui_->tableView->setModel(new QUGlyphTableModel(document, this));
    ui_->tableView->verticalHeader()->hide();
    ui_->tableView->setItemDelegateForColumn(1, new QUGlyphTableBitmapDelegate(this));
}

QUGlyphTableWidget::~QUGlyphTableWidget() {
    delete ui_;
}
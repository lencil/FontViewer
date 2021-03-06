#pragma once

#include <QWidget>
#include <QAbstractTableModel>

class QUDocument;

namespace Ui {
    class QUGlyphTableWidget;
}

class QUGlyphTableModelColumn;

class QUGlyphTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit QUGlyphTableModel(QUDocument * document, QObject * parent = nullptr);
    int
    rowCount(const QModelIndex & parent) const;

    int
    columnCount(const QModelIndex & parent) const;
    
    QVariant
    data(const QModelIndex & index, int role) const;
    
    QVariant
    headerData(int section, Qt::Orientation orientation, int role) const;

    const QVector<QUGlyphTableModelColumn *> &
    columns() const;

    FXGlyph
    glyphAt(int row) const;
public slots:
    void
    reset();

    bool
    exportToFile(const QString & filePath) const;
    
protected:
    QUDocument     * document_;
    QVector<QUGlyphTableModelColumn *> columns_;
};

class QUGlyphTableWidget : public QWidget {
    Q_OBJECT

public:
    explicit QUGlyphTableWidget(QUDocument * document, QWidget *parent = nullptr);
    ~QUGlyphTableWidget();
                         
public slots:
    void
    exportToFile();

private slots:
    void
    gotoGlyphAtIndex(const QModelIndex & index);
private:
    Ui::QUGlyphTableWidget * ui_;
    QUDocument             * document_;
    QUGlyphTableModel      * model_;
};

#include <QAbstractItemView>
#include <QEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QApplication>
#include <QFileInfo>
#include <QLineEdit>
#include <QTimer>
#include <QSortFilterProxyModel>
#include "FontX/FXFaceDatabase.h"
#include "QUFontManager.h"
#include "QUFontComboBox.h"
#include "QUConv.h"

QUFontListModel::QUFontListModel(QObject * parent)
    : QAbstractListModel(parent) {
}

int
QUFontListModel::rowCount(const QModelIndex & parent) const {
    return (int)db()->faceCount();
}
    
QVariant
QUFontListModel::data(const QModelIndex & index, int role) const {
    if (!index.isValid())
        return QVariant();
    if (index.row() >= db()->faceCount())
        return QVariant();
    if (role == Qt::DisplayRole) 
        return displayName(index.row());
    else
        return QVariant();
    
}

bool
QUFontListModel::acceptRow(const QString & filter, int row) const {
    if (filter.isEmpty())
        return true;
    
    if (displayName(row).contains(filter, Qt::CaseInsensitive))
        return true;

    auto const & atts = attributes(row);

    auto searchInNames = [](const FXMap<FXString, FXString> & names, const QString & name) {
        for (const auto it : names) {
            if (toQString(it.second).contains(name, Qt::CaseInsensitive))
                return true;
        }
        return false;
    };

    return searchInNames(atts.names.localizedFamilyNames(), filter) ||
        searchInNames(atts.names.localizedStyleNames(), filter) ||
        searchInNames(atts.names.localizedPostscriptNames(), filter);
}

const FXFaceAttributes &
QUFontListModel::attributes(size_t index) const {
    return db()->faceAttributes(index);
}

QString
QUFontListModel::displayName(size_t index) const {
    auto const & attrs = attributes(index);
    QString familyName = toQString(attrs.names.familyName());
    QString styleName = toQString(attrs.names.styleName());
    if (!familyName.isEmpty())
        return QString("%1 - %2").arg(familyName, styleName);
    else
        return QFileInfo(toQString(attrs.desc.filePath)).fileName();
    
}

FXPtr<FXFaceDatabase>
QUFontListModel::db() const {
    return QUFontManager::get().db();
}

bool
QUSortFilterFontListModel::lessThan(const QModelIndex & left, const QModelIndex & right) const {
    QVariant d0 = fontListModel()->data(left, Qt::DisplayRole);
    QVariant d1 = fontListModel()->data(right, Qt::DisplayRole);
    return d0 < d1;
    return true;
}

bool
QUSortFilterFontListModel::filterAcceptsRow(int sourceRow, const QModelIndex & sourceParent) const {
    if (filter_.isEmpty())
        return true;
    return fontListModel()->acceptRow(filter_, sourceRow);
}

void
QUSortFilterFontListModel::filter(const QString & text) {
    filter_ = text;
    invalidateFilter();
}

QUFontListModel *
QUSortFilterFontListModel::fontListModel() const {
    return qobject_cast<QUFontListModel *>(sourceModel());
}

QUFontComboBox::QUFontComboBox(QWidget * parent)
    : QComboBox(parent) {
    QSortFilterProxyModel * proxy = new QUSortFilterFontListModel(this);
    proxy->setSourceModel(new QUFontListModel(this));
    setModel(proxy);
    proxy->sort(0);
    setEditable(true);
    setCompleter(nullptr);
    setMaxVisibleItems(20);
    
    connect(this, QOverload<int>::of(&QComboBox::activated),
            this, &QUFontComboBox::onFontSelected);

    connect(lineEdit(), &QLineEdit::textEdited,
            this, &QUFontComboBox::onLineEdited);

    view()->installEventFilter(this);
}

QUFontURI
QUFontComboBox::selectedFont() const {
    int row = currentSourceIndex().row();
    auto desc = QUFontManager::get().db()->faceDescriptor(row);
    auto atts = QUFontManager::get().db()->faceAttributes(row);
    atts.names.familyName();
    QUFontURI uri{toQString(desc.filePath), desc.index};
    return uri;
}

QUSortFilterFontListModel *
QUFontComboBox::proxyModel() const {
    return qobject_cast<QUSortFilterFontListModel*>(model());
}

QModelIndex
QUFontComboBox::currentProxyIndex() const {
    return model()->index(currentIndex(), modelColumn());   
}
    
QModelIndex
QUFontComboBox::currentSourceIndex() const {
    return proxyModel()->mapToSource(currentProxyIndex());
}

void
QUFontComboBox::showPopup() {
    QComboBox::showPopup();
    qreal y = mapToGlobal(rect().topLeft()).y();
    QWidget *popup = this->findChild<QFrame*>(); 
    popup->move(popup->x(),popup->y() + (popup->y() > y? 8 : -4));
}

bool
QUFontComboBox::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        QKeyEvent* newEvent = new QKeyEvent(keyEvent->type(),
                                            keyEvent->key(),
                                            keyEvent->modifiers(), 
                                            keyEvent->text(),
                                            keyEvent->isAutoRepeat(),
                                            keyEvent->count());

        QFocusEvent* focusEvent = new QFocusEvent(QEvent::FocusIn, Qt::OtherFocusReason);
        QCoreApplication::postEvent(lineEdit(), focusEvent);
        QCoreApplication::postEvent(lineEdit(), newEvent);
    }
    return false;
}

void
QUFontComboBox::onFontSelected(int ) {
    QModelIndex index = currentProxyIndex();
    QVariant data = model()->data(index, Qt::DisplayRole);
    lineEdit()->setText(data.toString());
}

void
QUFontComboBox::onLineEdited(const QString & text) {
    proxyModel()->filter(text);
    lineEdit()->setText(text);
    showPopup();
}

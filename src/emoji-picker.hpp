// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QDialog>
#include <QStringList>
#include <functional>
#include <QJsonObject>
class QLineEdit;
class QComboBox;
class QListView;
class QLabel;
namespace dst {
struct EmojiEntry {QString emoji,group,name,keywords;QJsonObject names;};
const QList<EmojiEntry> &emojiCatalog();
class EmojiModel;
class EmojiPicker final:public QDialog {
public:
 EmojiPicker(QStringList recent,std::function<void(QString)> selected,QWidget *parent);
private:
 void filter();
 QStringList recent_;
 std::function<void(QString)> selected_;
 QLineEdit *search_;
 QComboBox *category_;
 QListView *list_;
 QLabel *count_;
 EmojiModel *model_;
};
}

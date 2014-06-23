/**************************************************************************
**
** Copyright (c) 2014 Carel Combrink
**
** This file is part of the SpellChecker Plugin, a Qt Creator plugin.
**
** The SpellChecker Plugin is free software: you can redistribute it and/or 
** modify it under the terms of the GNU Lesser General Public License as 
** published by the Free Software Foundation, either version 3 of the 
** License, or (at your option) any later version.
** 
** The SpellChecker Plugin is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public License
** along with the SpellChecker Plugin.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#ifndef SPELLCHECKER_OUTPUTPANE_H
#define SPELLCHECKER_OUTPUTPANE_H

#include "Word.h"

#include <coreplugin/ioutputpane.h>

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace SpellChecker {
namespace Internal {

class SpellingMistakesModel;

class OutputPanePrivate;
class OutputPane : public Core::IOutputPane
{
    Q_OBJECT
public:
    OutputPane(SpellingMistakesModel* model, QObject *parent = 0);
    ~OutputPane();

    QWidget *outputWidget(QWidget *parent);
    QList<QWidget*> toolBarWidgets() const;
    QString displayName() const;
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool visible);
    void setFocus();
    bool hasFocus() const;
    bool canFocus() const;
    bool canNavigate() const;
    bool canNext() const;
    bool canPrevious() const;
    void goToNext();
    void goToPrev();
    
signals:
    
public slots:
private slots:
    void updateMistakesCount();
    void mistakeSelected(const QModelIndex& index);
    void wordUnderCursorMistake(bool isMistake, const Word& word);

private:
    OutputPanePrivate* const d;
    
};

} // namespace Internal
} // namespace SpellChecker

#endif // SPELLCHECKER_OUTPUTPANE_H

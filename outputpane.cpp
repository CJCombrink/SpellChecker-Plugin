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

#include "outputpane.h"
#include "spellcheckercore.h"
#include "spellingmistakesmodel.h"
#include "spellcheckerconstants.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QTreeView>
#include <QHeaderView>
#include <QToolButton>
#include <QApplication>
#include <QFileInfo>

class SpellChecker::Internal::OutputPanePrivate {
public:
    SpellingMistakesModel* model;
    QTreeView *treeView;
    QList<QWidget *> toolbarWidgets;
    QToolButton* buttonIgnore;
    QToolButton* buttonAdd;
    QToolButton* buttonSuggest;

    OutputPanePrivate()
    {}
    ~OutputPanePrivate() {}
};
//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------

using namespace SpellChecker;
using namespace SpellChecker::Internal;

OutputPane::OutputPane(SpellingMistakesModel *model, QObject *parent) :
    IOutputPane(parent),
    d(new OutputPanePrivate())
{
    Q_ASSERT(model != NULL);
    d->model = model;

    /* Create the tree view for the model */
    d->treeView = new QTreeView();
    d->treeView->setRootIsDecorated(false);
    d->treeView->setFrameStyle(QFrame::NoFrame);
    d->treeView->setSortingEnabled(true);
    d->treeView->setModel(d->model);
    d->treeView->setAttribute(Qt::WA_MacShowFocusRect, false);

    QHeaderView *header = d->treeView->header();
    header->setResizeMode(Constants::MISTAKE_COLUMN_IDX, QHeaderView::ResizeToContents);
    header->setResizeMode(Constants::MISTAKE_COLUMN_WORD, QHeaderView::ResizeToContents);
    header->setResizeMode(Constants::MISTAKE_COLUMN_SUGGESTIONS, QHeaderView::Interactive);
    header->setResizeMode(Constants::MISTAKE_COLUMN_FILE, QHeaderView::ResizeToContents);
    header->setResizeMode(Constants::MISTAKE_COLUMN_LINE, QHeaderView::ResizeToContents);
    header->setResizeMode(Constants::MISTAKE_COLUMN_COLUMN, QHeaderView::ResizeToContents);
    header->setStretchLastSection(false);
    header->setMovable(false);

    /* Create the toolbar buttons */
    d->buttonSuggest = new QToolButton();
    d->buttonSuggest->setIcon(QIcon(QLatin1String(":/texteditor/images/refactormarker.png")));
    d->buttonSuggest->setText(tr("Give Suggestions"));
    d->buttonSuggest->setToolTip(tr("Give suggestions for the word"));
    d->toolbarWidgets.push_back(d->buttonSuggest);
    connect(d->buttonSuggest, SIGNAL(clicked()), SpellCheckerCore::instance(), SLOT(giveSuggestionsForWordUnderCursor()));

    d->buttonIgnore = new QToolButton();
    d->buttonIgnore->setText(tr("-"));
    d->buttonIgnore->setToolTip(tr("Ignore the word"));
    d->toolbarWidgets.push_back(d->buttonIgnore);
    connect(d->buttonIgnore, SIGNAL(clicked()), SpellCheckerCore::instance(), SLOT(ignoreWordUnderCursor()));

    d->buttonAdd = new QToolButton();
    d->buttonAdd->setText(tr("+"));
    d->buttonAdd->setToolTip(tr("Add the word to the user dictionary"));
    d->toolbarWidgets.push_back(d->buttonAdd);
    connect(d->buttonAdd, SIGNAL(clicked()), SpellCheckerCore::instance(), SLOT(addWordUnderCursor()));

    SpellCheckerCore* core = SpellCheckerCore::instance();
    connect(core, SIGNAL(wordUnderCursorMistake(bool,Word)), this, SLOT(wordUnderCursorMistake(bool,Word)));

    connect(d->model, SIGNAL(mistakesUpdated()), SIGNAL(navigateStateUpdate()));
    connect(d->model, SIGNAL(mistakesUpdated()), this, SLOT(updateMistakesCount()));
    connect(d->treeView, SIGNAL(clicked(QModelIndex)), this, SLOT(mistakeSelected(QModelIndex)));
}
//--------------------------------------------------

OutputPane::~OutputPane()
{
    delete d->treeView;
    foreach(QWidget* widget, d->toolbarWidgets) {
        delete widget;
    }

    d->model = NULL;
    delete d;
}
//--------------------------------------------------

QWidget *OutputPane::outputWidget(QWidget *parent)
{
    Q_UNUSED(parent);
    return d->treeView;
}
//--------------------------------------------------

QList<QWidget *> OutputPane::toolBarWidgets() const
{
    return d->toolbarWidgets;
}
//--------------------------------------------------

QString OutputPane::displayName() const
{
    return tr(Constants::OUTPUT_PANE_TITLE);
}
//--------------------------------------------------

int OutputPane::priorityInStatusBar() const
{
    return 1;
}
//--------------------------------------------------

void OutputPane::clearContents()
{
}
//--------------------------------------------------

void OutputPane::visibilityChanged(bool visible)
{
    Q_UNUSED(visible);
}
//--------------------------------------------------

void OutputPane::setFocus()
{
    d->treeView->setFocus();
}
//--------------------------------------------------

bool OutputPane::hasFocus() const
{
    return d->treeView->hasFocus();
}
//--------------------------------------------------

bool OutputPane::canFocus() const
{
    return true;
}
//--------------------------------------------------

bool OutputPane::canNavigate() const
{
    return true;
}
//--------------------------------------------------

bool OutputPane::canNext() const
{
    return d->treeView->model()->rowCount() > 1;
}
//--------------------------------------------------

bool OutputPane::canPrevious() const
{
    return d->treeView->model()->rowCount() > 1;
}
//--------------------------------------------------

void OutputPane::goToNext()
{
    QModelIndex currentIndex;
    QModelIndexList currentSelectedList = d->treeView->selectionModel()->selectedIndexes();
    if(currentSelectedList.isEmpty() == false)
        currentIndex = currentSelectedList.first();

    QModelIndex nextIndex = d->treeView->indexBelow(currentIndex);
    if(nextIndex.isValid() == false) {
        nextIndex = d->treeView->model()->index(0, 0);
    }

    d->treeView->selectionModel()->select(nextIndex, QItemSelectionModel::SelectCurrent);
    mistakeSelected(nextIndex);
}
//--------------------------------------------------

void OutputPane::goToPrev()
{
    QModelIndex currentIndex;
    QModelIndexList currentSelectedList = d->treeView->selectionModel()->selectedIndexes();
    if(currentSelectedList.isEmpty() == false)
        currentIndex = currentSelectedList.first();

    QModelIndex previousIndex = d->treeView->indexAbove(currentIndex);
    if(previousIndex.isValid() == false) {
        previousIndex = d->treeView->model()->index(d->treeView->model()->rowCount() - 1, 0);
    }

    d->treeView->selectionModel()->select(previousIndex, QItemSelectionModel::SelectCurrent);
    mistakeSelected(previousIndex);
}
//--------------------------------------------------

void OutputPane::updateMistakesCount()
{
    setBadgeNumber(d->model->rowCount());
}
//--------------------------------------------------

void OutputPane::mistakeSelected(const QModelIndex &index)
{
    int row = index.row();
    QString word = index.sibling(row, Constants::MISTAKE_COLUMN_WORD).data().toString();
    QString fileName = index.sibling(row, Constants::MISTAKE_COLUMN_FILE).data().toString();
    int line = index.sibling(row, Constants::MISTAKE_COLUMN_LINE).data().toInt();
    int column = index.sibling(row, Constants::MISTAKE_COLUMN_COLUMN).data().toInt();

    if(QFileInfo(fileName).exists() == true) {
        Core::IEditor *editor = Core::EditorManager::openEditor(fileName);
        editor->gotoLine(line, column - 1);
    }
}
//--------------------------------------------------

void OutputPane::wordUnderCursorMistake(bool isMistake, const Word &word)
{
    Q_UNUSED(word);
    d->buttonSuggest->setEnabled(isMistake);
    d->buttonIgnore->setEnabled(isMistake);
    d->buttonAdd->setEnabled(isMistake);
}
//--------------------------------------------------

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

#include "NavigationWidget.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QHeaderView>
#include <QPainter>

using namespace SpellChecker::Internal;

SpellingMistakeDelegate::SpellingMistakeDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{

}
//--------------------------------------------------

void SpellingMistakeDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);
    QStyledItemDelegate::paint(painter, option, index);
    QFontMetrics fm(opt.font);
    painter->save();

    QString fileName   = index.data(ProjectMistakesModel::COLUMN_FILE).toString();
    QString nrMistakes = index.data(ProjectMistakesModel::COLUMN_MISTAKES_TOTAL).toString();
    bool inStartupProject = index.data(ProjectMistakesModel::COLUMN_FILE_IN_STARTUP).toBool();
    QString nrLiterals = index.data(ProjectMistakesModel::COLUMN_LITERAL_COUNT).toString();
    nrLiterals.append(QLatin1String("\""));
    nrLiterals.prepend(QLatin1String("\""));
    if(inStartupProject == false) {
        painter->setPen(Qt::lightGray);
    }
    /* Write the File Name */
    painter->drawText(6, 2 + opt.rect.top() + fm.ascent(), fileName);
    /* Write the number of mistakes */
    painter->drawText(opt.rect.right() - fm.width(nrMistakes) - 50 , 2 + opt.rect.top() + fm.ascent(), nrMistakes);
    /* Draw the number of String Literal Mistakes. */
    painter->setPen(Qt::darkGreen);
    painter->drawText(opt.rect.right() - fm.width(nrLiterals) - 6 , 2 + opt.rect.top() + fm.ascent(), nrLiterals);
    painter->restore();
}
//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------

class SpellChecker::Internal::NavigationWidgetPrivate {
public:
    NavigationWidgetPrivate() {}
    ProjectMistakesModel* model;
};
//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------
NavigationWidget::NavigationWidget(ProjectMistakesModel *model) :
    d(new SpellChecker::Internal::NavigationWidgetPrivate())
{
    d->model = model;
    setModel(model);
    setItemDelegate(new SpellingMistakeDelegate(this));
    setFrameStyle(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFocusPolicy(Qt::NoFocus);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)), this, SLOT(updateCurrentItem(Core::IEditor*)));
}
//--------------------------------------------------

NavigationWidget::~NavigationWidget()
{
    delete d;
}
//--------------------------------------------------

void NavigationWidget::updateCurrentItem(Core::IEditor *editor)
{
    if(editor == NULL) {
        clearSelection();
        return;
    }
    QString fileName = editor->document()->filePath();
    int idx = d->model->indexOfFile(fileName);
    QModelIndex modelIndex = model()->index(idx, 0);
    if(modelIndex.isValid() == false) {
        clearSelection();
        return;
    }
    setCurrentIndex(modelIndex);
    selectionModel()->select(currentIndex(), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    scrollTo(currentIndex());
}
//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------

NavigationWidgetFactory::NavigationWidgetFactory(ProjectMistakesModel *model) :
    d_model(model)
{
    setDisplayName(NavigationWidget::tr("Spelling Mistakes"));
    setPriority(600);
    setId("SpellingMistakes");
}
//--------------------------------------------------

Core::NavigationView NavigationWidgetFactory::createWidget()
{
    NavigationWidget *widget = new NavigationWidget(d_model);
    connect(widget, SIGNAL(clicked(QModelIndex)), d_model, SLOT(fileSelected(QModelIndex)));
    Core::NavigationView view;
    view.widget = widget;
    return view;
}
//--------------------------------------------------

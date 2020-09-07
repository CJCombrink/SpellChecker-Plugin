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

#pragma once

#include "spellcheckercoresettings.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QListWidgetItem;
QT_END_NAMESPACE

namespace SpellChecker {
namespace Internal {

namespace Ui {
class SpellCheckerCoreOptionsWidget;
} // namespace Ui

class SpellCheckerCoreOptionsWidget
  : public QWidget
{
  Q_OBJECT

public:
  SpellCheckerCoreOptionsWidget( const SpellCheckerCoreSettings* const settings, QWidget* parent = 0 );
  ~SpellCheckerCoreOptionsWidget();

  const SpellCheckerCoreSettings& settings();
  /*! Function to apply the settings in widgets added to the Core Options Widget.
   * This function will in turn cause the applyCurrentSetSettings() signal to be
   * emitted. */
  void applySettings();
signals:
  /*! Signal to notify other added options widgets that the specified settings can be applied. */
  void applyCurrentSetSettings();
public slots:
  /*! \brief Slot called when there was a error on one of the options pages.
   * \param optionsPage Name of the options page.
   * \param error Error that occurred on the page. */
  void optionsPageError( const QString& optionsPage, const QString& error );

private slots:
  void on_comboBoxSpellChecker_currentIndexChanged( const QString& arg1 );
  void on_toolButtonAddProject_clicked();
  void on_toolButtonRemoveProject_clicked();
  void listWidgetItemChanged( QListWidgetItem* item );

private:
  void updateWithSettings( const SpellCheckerCoreSettings* const settings );
  Ui::SpellCheckerCoreOptionsWidget* ui;
  SpellCheckerCoreSettings m_settings;
  QStringList m_projectsToIgnore;
  QWidget* m_currentCheckerOptionsWidget; /*! Pointer to keep track of the current
                                           *  options widget that is shown on the
                                           *  options page. This is needed to remove
                                           *  the options if the checker changes. */
};

} // Internal
} // SpellChecker

/**************************************************************************
**
** Copyright (c) 2026 Marcel Petrick
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

#include "../../idocumentparser.h"

#include <projectexplorer/projectexplorer.h>

namespace SpellChecker {
namespace QmlSpellChecker {
namespace Internal {

class QmlDocumentParserPrivate;

/*! \brief The QML document parser.
 *
 * This parser extracts spell-checkable words from QML comments and string
 * literals. It follows the same parser contract as the C++ parser by reacting
 * to current editor changes, project changes, settings changes and open
 * document edits.
 */
class QmlDocumentParser
  : public SpellChecker::IDocumentParser
{
  Q_OBJECT
public:
  explicit QmlDocumentParser( QObject* parent = nullptr );
  ~QmlDocumentParser() override;

  QString displayName() override;
  Core::IOptionsPage* optionsPage() override;

protected:
  /*! \brief Update the current editor path and reparse it if it is a QML file.
   * \param[in] editorFilePath Path of the current editor. */
  void setCurrentEditor( const QString& editorFilePath ) override;
  /*! \brief Update the active project and reparse project QML files.
   * \param[in] activeProject Active Qt Creator project. */
  void setActiveProject( ProjectExplorer::Project* activeProject ) override;
  /*! \brief Parse newly added QML files and forget removed project files.
   * \param[in] filesAdded Project files that were added.
   * \param[in] filesRemoved Project files that were removed. */
  void updateProjectFiles( QStringSet filesAdded, QStringSet filesRemoved ) override;

private slots:
  /*! \brief Reparse files after QML parser or core settings changed. */
  void settingsChanged();
  /*! \brief Handle completion of a background QML parse job. */
  void futureFinished();
  /*! \brief Disconnect and cancel work during Qt Creator shutdown. */
  void aboutToQuit();

private:
  /*! \brief Cancel outstanding work and parse all QML files in the project. */
  void reparseProject();
  /*! \brief Parse one QML file if settings allow it.
   * \param[in] fileName File to parse. */
  void parseFile( const QString& fileName );
  /*! \brief Debounce reparsing for edited open QML documents.
   * \param[in] fileName File to schedule for parsing. */
  void scheduleParseFile( const QString& fileName );
  /*! \brief Start watching an open QML document for content changes.
   * \param[in] document Document to watch. */
  void watchDocument( Core::IDocument* document );
  /*! \brief Stop watching a document that was closed.
   * \param[in] document Document to unwatch. */
  void unwatchDocument( Core::IDocument* document );
  /*! \brief Query whether the file should currently be parsed.
   * \param[in] fileName File to check.
   * \return true when the QML parser should parse the file. */
  bool shouldParseDocument( const QString& fileName ) const;

  QmlDocumentParserPrivate* const d;
};

} // namespace Internal
} // namespace QmlSpellChecker
} // namespace SpellChecker

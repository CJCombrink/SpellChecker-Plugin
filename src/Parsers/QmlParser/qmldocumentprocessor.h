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

#include "../../Word.h"
#include "qmlparsersettings.h"

#include <QFuture>
#include <QObject>

namespace SpellChecker {
namespace QmlSpellChecker {
namespace Internal {

/*! \brief Background processor for a QML document.
 *
 * The processor owns a snapshot of QML source text and extracts words from
 * comments and string literals. It is intentionally independent from Qt Creator
 * editor objects so the scanner can be unit tested without starting Qt Creator.
 */
class QmlDocumentProcessor
  : public QObject
{
  Q_OBJECT
  QmlDocumentProcessor( const QmlDocumentProcessor& ) = delete;
  QmlDocumentProcessor& operator=( const QmlDocumentProcessor& ) = delete;
public:
  using Watcher = QFutureWatcher<WordList>;
  using WatcherPtr = Watcher *;
  using Promise = QPromise<WordList>;

  /*! \brief Constructor.
   * \param[in] fileName File name associated with the source snapshot.
   * \param[in] source Source text to parse.
   * \param[in] settings Parser settings to apply. */
  QmlDocumentProcessor( const QString& fileName, const QString& source, const QmlParserSettings& settings );
  ~QmlDocumentProcessor() override;

  /*! \brief Process function run by QtConcurrent.
   * \param[inout] promise Future promise used to report the extracted words. */
  void process( Promise& promise );

  /*! \brief Parse QML source text into spell-checkable words.
   * \param[in] fileName File name to store on each returned word.
   * \param[in] source Source text to scan.
   * \param[in] settings Parser settings to apply.
   * \return Words extracted from comments and string literals. */
  static WordList parseSource( const QString& fileName, const QString& source, const QmlParserSettings& settings );

private:
  QString m_fileName;
  QString m_source;
  QmlParserSettings m_settings;
};

} // namespace Internal
} // namespace QmlSpellChecker
} // namespace SpellChecker

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

#include "../../src/Parsers/QmlParser/qmldocumentprocessor.h"

#include <QtTest>

using namespace SpellChecker;
using namespace SpellChecker::QmlSpellChecker::Internal;

namespace {

/*! \brief Convert the plugin WordList container into an ordered QList for tests.
 * \param[in] wordList Words returned by the parser.
 * \return Words copied into a QList. */
QList<Word> allWords( const WordList& wordList )
{
  QList<Word> words;
  for( const Word& word: wordList ) {
    words << word;
  }
  return words;
}

/*! \brief Query whether a parsed word list contains a word.
 * \param[in] words Words to search.
 * \param[in] text Text to find.
 * \return true when a word with matching text exists. */
bool containsWord( const QList<Word>& words, const QString& text )
{
  return std::any_of( words.cbegin(), words.cend(), [&text]( const Word& word ) {
    return word.text == text;
  } );
}

/*! \brief Find a word by text.
 * \param[in] words Words to search.
 * \param[in] text Text to find.
 * \return Matching word, or a default word if no match exists. */
Word findWord( const QList<Word>& words, const QString& text )
{
  const auto iter = std::find_if( words.cbegin(), words.cend(), [&text]( const Word& word ) {
    return word.text == text;
  } );
  return iter == words.cend() ? Word{} : *iter;
}

} // namespace

/*! \brief Verifies extraction and filtering by the QML document processor.
 *
 * This class tests QML comments and literals without requiring a running Qt
 * Creator instance.
 */
class QmlDocumentProcessorTest
  : public QObject
{
  Q_OBJECT

private slots:
  void extractsCommentsAndStringLiterals();
  void ignoresQmlCodeTokens();
  void supportsMultilineCommentsAndTemplateLiterals();
  void extractsInlineCommentAfterStringLiteral();
  void filtersCommonFalsePositives();
  void honorsCheckModeSettings();
};

void QmlDocumentProcessorTest::extractsCommentsAndStringLiterals()
{
  QmlParserSettings settings;
  const QString source = QStringLiteral(
    "Item {\n"
    "    text: \"Hello visible wurld\"\n"
    "    // Commant here\n"
    "}\n" );

  const QList<Word> words = allWords( QmlDocumentProcessor::parseSource( QStringLiteral( "Main.qml" ), source, settings ) );

  QVERIFY( containsWord( words, QStringLiteral( "Hello" ) ) );
  QVERIFY( containsWord( words, QStringLiteral( "visible" ) ) );
  QVERIFY( containsWord( words, QStringLiteral( "wurld" ) ) );
  QVERIFY( containsWord( words, QStringLiteral( "Commant" ) ) );
  QVERIFY( containsWord( words, QStringLiteral( "here" ) ) );

  const Word hello = findWord( words, QStringLiteral( "Hello" ) );
  QCOMPARE( hello.lineNumber, 2 );
  QCOMPARE( hello.columnNumber, 12 );
  QCOMPARE( hello.inComment, false );

  const Word comment = findWord( words, QStringLiteral( "Commant" ) );
  QCOMPARE( comment.lineNumber, 3 );
  QCOMPARE( comment.columnNumber, 8 );
  QCOMPARE( comment.inComment, true );
}

void QmlDocumentProcessorTest::ignoresQmlCodeTokens()
{
  QmlParserSettings settings;
  const QString source = QStringLiteral(
    "import QtQuick\n"
    "Rectangle {\n"
    "    id: rootItem\n"
    "    property string internalName: model.displayName\n"
    "    text: qsTr(\"Visible label\")\n"
    "}\n" );

  const QList<Word> words = allWords( QmlDocumentProcessor::parseSource( QStringLiteral( "Main.qml" ), source, settings ) );

  QVERIFY( containsWord( words, QStringLiteral( "Visible" ) ) );
  QVERIFY( containsWord( words, QStringLiteral( "label" ) ) );
  QVERIFY( !containsWord( words, QStringLiteral( "Rectangle" ) ) );
  QVERIFY( !containsWord( words, QStringLiteral( "root" ) ) );
  QVERIFY( !containsWord( words, QStringLiteral( "Item" ) ) );
  QVERIFY( !containsWord( words, QStringLiteral( "internal" ) ) );
  QVERIFY( !containsWord( words, QStringLiteral( "Name" ) ) );
}

void QmlDocumentProcessorTest::supportsMultilineCommentsAndTemplateLiterals()
{
  QmlParserSettings settings;
  const QString source = QStringLiteral(
    "Item {\n"
    "    /* First commant\n"
    "       second line */\n"
    "    property string value: `Template literal text`\n"
    "}\n" );

  const QList<Word> words = allWords( QmlDocumentProcessor::parseSource( QStringLiteral( "Main.qml" ), source, settings ) );

  QVERIFY( containsWord( words, QStringLiteral( "First" ) ) );
  QVERIFY( containsWord( words, QStringLiteral( "commant" ) ) );
  QVERIFY( containsWord( words, QStringLiteral( "second" ) ) );
  QVERIFY( containsWord( words, QStringLiteral( "Template" ) ) );
  QVERIFY( containsWord( words, QStringLiteral( "literal" ) ) );

  const Word second = findWord( words, QStringLiteral( "second" ) );
  QCOMPARE( second.lineNumber, 3 );
  QCOMPARE( second.inComment, true );
}

void QmlDocumentProcessorTest::extractsInlineCommentAfterStringLiteral()
{
  QmlParserSettings settings;
  const QString source = QStringLiteral(
    "Window {\n"
    "    color: \"#2c3e50\" // Darker background for a polisehed appiarance.\n"
    "}\n" );

  const QList<Word> words = allWords( QmlDocumentProcessor::parseSource( QStringLiteral( "Main.qml" ), source, settings ) );

  QVERIFY( containsWord( words, QStringLiteral( "Darker" ) ) );
  QVERIFY( containsWord( words, QStringLiteral( "background" ) ) );
  QVERIFY( containsWord( words, QStringLiteral( "polisehed" ) ) );
  QVERIFY( containsWord( words, QStringLiteral( "appiarance" ) ) );
  QVERIFY( !containsWord( words, QStringLiteral( "#2c3e50" ) ) );

  const Word misspelled = findWord( words, QStringLiteral( "polisehed" ) );
  QCOMPARE( misspelled.lineNumber, 2 );
  QCOMPARE( misspelled.inComment, true );
}

void QmlDocumentProcessorTest::filtersCommonFalsePositives()
{
  QmlParserSettings settings;
  const QString source = QStringLiteral(
    "Item {\n"
    "    text: \"Visit https://example.org/path and test@example.org plus camelCase_word 1234 #ffaa00 OK\"\n"
    "}\n" );

  const QList<Word> words = allWords( QmlDocumentProcessor::parseSource( QStringLiteral( "Main.qml" ), source, settings ) );

  QVERIFY( containsWord( words, QStringLiteral( "Visit" ) ) );
  QVERIFY( containsWord( words, QStringLiteral( "camel" ) ) );
  QVERIFY( containsWord( words, QStringLiteral( "Case" ) ) );
  QVERIFY( containsWord( words, QStringLiteral( "word" ) ) );
  QVERIFY( !containsWord( words, QStringLiteral( "https://example.org/path" ) ) );
  QVERIFY( !containsWord( words, QStringLiteral( "test@example.org" ) ) );
  QVERIFY( !containsWord( words, QStringLiteral( "1234" ) ) );
  QVERIFY( !containsWord( words, QStringLiteral( "#ffaa00" ) ) );
  QVERIFY( !containsWord( words, QStringLiteral( "OK" ) ) );
}

void QmlDocumentProcessorTest::honorsCheckModeSettings()
{
  QmlParserSettings settings;
  settings.whatToCheck = QmlParserSettings::CheckComments;

  const QString source = QStringLiteral(
    "Item {\n"
    "    text: \"Literalword\"\n"
    "    // Commentword\n"
    "}\n" );

  QList<Word> words = allWords( QmlDocumentProcessor::parseSource( QStringLiteral( "Main.qml" ), source, settings ) );
  QVERIFY( containsWord( words, QStringLiteral( "Commentword" ) ) );
  QVERIFY( !containsWord( words, QStringLiteral( "Literalword" ) ) );

  settings.whatToCheck = QmlParserSettings::CheckStringLiterals;
  words = allWords( QmlDocumentProcessor::parseSource( QStringLiteral( "Main.qml" ), source, settings ) );
  QVERIFY( !containsWord( words, QStringLiteral( "Commentword" ) ) );
  QVERIFY( containsWord( words, QStringLiteral( "Literalword" ) ) );
}

QTEST_GUILESS_MAIN( QmlDocumentProcessorTest )
#include "tst_qmldocumentprocessor.moc"

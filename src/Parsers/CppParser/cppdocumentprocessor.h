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

#include "../../Word.h"
#include "cppparsersettings.h"

#include <cplusplus/CppDocument.h>

#include <QFuture>

namespace CPlusPlus {
class Overview;
} // namespace CPlusPlus

namespace SpellChecker {
namespace CppSpellChecker {
namespace Internal {

/*! \brief The Word Tokens structure
 *
 * This structure is returned by the parseToken() function and contains
 * the list of words that were extracted from a token (comment, literal,
 * etc.). The structure also contains the hash of the token so that it can
 * be checked when the same file is parsed again. There is no need to store
 * the actual token since the hash comparison should be good enough to
 * check for changes.
 *
 * The \a line and \a column are stored for the token so that if a token did not
 * change, but it moved, the words that came from that token can just be
 * moved as needed without the need to do any string processing and parsing.
 *
 * The \a newHash flag keeps track if the words were extracted in a
 * previous pass or not, meaning that they were already processed and does not
 * need to be processed further. */
struct WordTokens
{
  enum class Type {
    Comment = 0,
    Doxygen,
    Literal,
    Identifier,
  };

  HashWords::key_type hash;
  int32_t line   = 0;
  int32_t column = 0;
  QString string;
  WordList words;
  bool newHash = true;
  Type type;
};

class CppDocumentProcessorPrivate;
/*! \brief The C++ Document Processor class.
 *
 * This processor class use QtConcurrent to process a CPlusPlus::Document::Ptr
 * in in the background and uses a QFuture to report the result.
 *
 * The processor extracts words and literals from the document and this is
 * then passed to the spellChecker.
 *
 * This processor does some handling of settings, but only those that are
 * significant while working with the document. */
class CppDocumentProcessor
  : public QObject
{
  Q_OBJECT
  /*! \brief Deleted copy constructor */
  CppDocumentProcessor( const CppDocumentProcessor& ) = delete;
  /*! \brief Deleted assignment operator */
  CppDocumentProcessor& operator==( const CppDocumentProcessor& ) = delete;
public:
  /*! \brief Structure for the result type that the future will return. */
  struct ResultType
  {
    HashWords wordHashes; /*!< List of hashes extracted along with words from the hash. */
    WordList words;       /*!< Word tokens that were extracted by the processor. */
  };
  /*! \brief Alias for the Watcher type. */
  using Watcher = QFutureWatcher<ResultType>;
  /*! \brief Alias for the Watcher Pointer type. */
  using WatcherPtr = Watcher *;
  /*! \brief Promise alias to simplify some typing and management. */
  using Promise = QPromise<ResultType>;

  /*! \brief Constructor
   *
   * Construct the processor for he given document, hashes for a speedup and
   * the settings that should be applied.
   * \param documentPointer Shared ownership of the document pointer to prevent
   *    it from getting deleted while the processor still runs.
   * \param hashWords List of hashes that should be used to optimise the parsing.
   * \param cppSettings Settings that should be applied. */
  CppDocumentProcessor( CPlusPlus::Document::Ptr documentPointer, const HashWords& hashWords, const CppParserSettings& cppSettings );
  /*! Destructor. */
  ~CppDocumentProcessor();
  /*! \brief Process function that the thread will run with the future that will
   * report the result. */
  void process( Promise& promise );

private:
  QStringSet getWordsThatAppearInSource() const;
  QStringSet getListOfWordsFromSourceRecursive( const CPlusPlus::Symbol* symbol, const CPlusPlus::Overview& overview ) const;
  QStringSet getPossibleNamesFromString( const QString& string ) const;

  /*! \brief Parse a Token retrieved from the Translation Unit of the document.
   *
   * Since both Comments and String Literals are tokens, the common code to extract the
   * words was added to a function to only work on a token.
   *
   * A hash is passed to the function that contains a hash and a set of words
   * that were previously extracted from a token with that hash. The function
   * uses that information to check if the token that must be processed now
   * was processed before (the new hash should match a hash in the map). If
   * a hash was processed before, then the parser can just re-use all the words
   * that came from the token previously without the need to do any string
   * processing on the token. This hash also contains information as to where
   * the token was the previous time, so that if it just moved the words can
   * be updated accordingly. Benchmarks showed that for large files or files
   * with a lot of tokens this had a big speedup. On smaller files the effect
   * was not as much but on smaller files this effect is negligible compared
   * to the speedup on large files.
   *
   * \param[in] token Translation Unit Token that should be split up into words that
   *              should be checked.
   * \param[in] type If the token is a Comment, Doxygen Documentation or a
   *              String Literal. This is captured to go along with the
   *              word so that the tables and displays upstream can indicate
   *              the difference between a comment and a literal. This gets
   *              forwarded to the extractWordsFromString() function where it
   *              is used to extract words.
   * \return WordTokens structure containing enough information to be useful to
   *              the caller. */
  WordTokens parseToken( const CPlusPlus::Token& token, WordTokens::Type type ) const;
  /*! \brief Extract Words from the given string.
   *
   * This function takes a string, either a comment or a string literal and
   * breaks the string into words or tokens that should later be checked
   * for spelling mistakes.
   * \param[in] string String that must be broken up into words.
   * \param[in] stringStart Start of the string.
   * \param[in] type If the string is a Comment, Doxygen Documentation or a
   *              String Literal. If the string is Doxygen docs then the
   *              function will also try to remove doxygen tags from the words
   *              extracted. This reduce the number of words returned that
   *              gets handled later on, and it does not rely on a setting,
   *              it must be done always to remove noise.
   * \return Words that were extracted from the string. */
  WordList extractWordsFromString(const QString& string, int32_t stringStart, WordTokens::Type type ) const;
  /*! \brief Check if the end of a possible word was reached.
   *
   * Utility function to check if the character at the given position is the
   * end of a word. The end of a word for example is a space There
   * are some handling of dots and other characters that determine if the
   * position is the end of the word.
   *
   * \todo Check if isEndOfCurrentWord can not be re-implemented
   * using an iterator instead of an index. This would possibly require
   * rework in the calling function as well, but might be cleaner. */
  bool isEndOfCurrentWord( const QString& comment, int currentPos ) const;
  /*! \brief Parse all macros in the document and extract string literals.
   *
   * Only macros that are functions and have arguments that are string literals
   * are considered. This is important since QStringLiteral() is a macro, and
   * there are also other macros that takes in literals as arguments. */
  QVector<WordTokens> parseMacros() const;

  /*! \brief Custom type for the checkHash() return type.
   *
   * If C++17 is used this should be replaced by a std::optional. */
  using TmpOptional = std::pair<bool, WordTokens>;
  /*! \brief Optimisation function to check a hash.
   *
   * An optimisation is done where a token string is extracted and
   * then the hash associated with that string and the words that
   * were extracted from that string is stored for the next pass
   * of the same file.
   *
   * If a hash is known, the token is not processed and the words
   * that were extracted in the previous pass will just get used
   * as-is.
   *
   * Also, the line and column number of the hash is stored
   * to check for trivial cases where the hash just moved.
   * This information is then used to move the words based
   * on the movement of the hash.
   *
   * This has the added benefit that if the same string is found
   * multiple times in the same file, it can just re-use the words
   * without any more processing on the second string. The usefulness
   * of this is probably not much since strings should not normally repeat.
   * People should use the DRY principal... */
  TmpOptional checkHash( WordTokens tokens, uint32_t hash ) const;

  friend CppDocumentProcessorPrivate;
  CppDocumentProcessorPrivate* const d;
};

} // namespace Internal
} // namespace CppSpellChecker
} // namespace SpellChecker

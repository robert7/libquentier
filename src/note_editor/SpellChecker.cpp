#include <quentier/note_editor/SpellChecker.h>
#include <quentier/utility/FileIOProcessorAsync.h>
#include <quentier/types/Account.h>
#include "SpellChecker_p.h"

namespace quentier {

SpellChecker::SpellChecker(FileIOProcessorAsync * pFileIOProcessorAsync,
                           const Account & account, QObject * parent,
                           const QString & userDictionaryPath) :
    QObject(parent),
    d_ptr(new SpellCheckerPrivate(pFileIOProcessorAsync, account, this, userDictionaryPath))
{
    QObject::connect(d_ptr, QNSIGNAL(SpellCheckerPrivate,ready),
                     this, QNSIGNAL(SpellChecker,ready));
}

QVector<QPair<QString,bool> > SpellChecker::listAvailableDictionaries() const
{
    Q_D(const SpellChecker);
    return d->listAvailableDictionaries();
}

void SpellChecker::setAccount(const Account & account)
{
    Q_D(SpellChecker);
    d->setAccount(account);
}

void SpellChecker::enableDictionary(const QString & language)
{
    Q_D(SpellChecker);
    d->enableDictionary(language);
}

void SpellChecker::disableDictionary(const QString & language)
{
    Q_D(SpellChecker);
    d->disableDictionary(language);
}

bool SpellChecker::checkSpell(const QString & word) const
{
    Q_D(const SpellChecker);
    return d->checkSpell(word);
}

QStringList SpellChecker::spellCorrectionSuggestions(const QString & misSpelledWord) const
{
    Q_D(const SpellChecker);
    return d->spellCorrectionSuggestions(misSpelledWord);
}

void SpellChecker::addToUserWordlist(const QString & word)
{
    Q_D(SpellChecker);
    d->addToUserWordlist(word);
}

void SpellChecker::removeFromUserWordList(const QString & word)
{
    Q_D(SpellChecker);
    d->removeFromUserWordList(word);
}

void SpellChecker::ignoreWord(const QString & word)
{
    Q_D(SpellChecker);
    d->ignoreWord(word);
}

void SpellChecker::removeWord(const QString & word)
{
    Q_D(SpellChecker);
    d->removeWord(word);
}

bool SpellChecker::isReady() const
{
    Q_D(const SpellChecker);
    return d->isReady();
}

} // namespace quentier

/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "SpellCheckerDictionariesFinder.h"
#include <quentier/logging/QuentierLogger.h>
#include <QStringList>
#include <QFileInfo>
#include <QDirIterator>

namespace quentier {

#define WRAP(x) \
    << QStringLiteral(x).toUpper()

SpellCheckerDictionariesFinder::SpellCheckerDictionariesFinder(const QSharedPointer<QAtomicInt> & pStopFlag, QObject * parent) :
    QObject(parent),
    m_pStopFlag(pStopFlag),
    m_files(),
    m_localeList(QSet<QString>()
#include "localeList.inl"
    )
{}

#undef WRAP

void SpellCheckerDictionariesFinder::run()
{
    QNDEBUG(QStringLiteral("SpellCheckerDictionariesFinder::run"));

    m_files.clear();
    QStringList fileFilters;
    fileFilters << QStringLiteral("*.dic") << QStringLiteral("*.aff");

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#define CHECK_AND_STOP() \
    if (!m_pStopFlag.isNull() && (m_pStopFlag->load() != 0)) { \
        QNDEBUG(QStringLiteral("Aborting the operation as stop flag is non-zero")); \
        return; \
    }
#else
#define CHECK_AND_STOP() \
    if (!m_pStopFlag.isNull() && (int(*m_pStopFlag) != 0)) { \
        QNDEBUG(QStringLiteral("Aborting the operation as stop flag is non-zero")); \
        return; \
    }
#endif

    QFileInfoList rootDirs = QDir::drives();
    const int numRootDirs = rootDirs.size();
    for(int i = 0; i < numRootDirs; ++i)
    {
        CHECK_AND_STOP()

        const QFileInfo & rootDirInfo = rootDirs[i];

        if (Q_UNLIKELY(!rootDirInfo.isDir())) {
            QNTRACE(QStringLiteral("Skipping non-dir ") << rootDirInfo.absoluteDir());
            continue;
        }

        QDirIterator it(rootDirInfo.absolutePath(), fileFilters, QDir::Files, QDirIterator::Subdirectories);
        while(it.hasNext())
        {
            CHECK_AND_STOP()

            QString nextDirName = it.next();
            QNTRACE(QStringLiteral("Next dir name = ") << nextDirName);

            QFileInfo fileInfo = it.fileInfo();
            if (!fileInfo.isReadable()) {
                QNTRACE(QStringLiteral("Skipping non-readable file ") << fileInfo.absoluteFilePath());
                continue;
            }

            QString fileNameSuffix = fileInfo.completeSuffix();
            bool isDicFile = false;
            if (fileNameSuffix == QStringLiteral("dic")) {
                isDicFile = true;
            }
            else if (fileNameSuffix != QStringLiteral("aff")) {
                QNTRACE(QStringLiteral("Skipping file not actually matching the filter: ") << fileInfo.absoluteFilePath());
                continue;
            }

            QString dictionaryName = fileInfo.baseName();
            if (!m_localeList.contains(dictionaryName.toUpper())) {
                QNTRACE(QStringLiteral("Skipping dictionary which doesn't appear to correspond to any locale: ") + dictionaryName);
                continue;
            }

            QPair<QString, QString> & pair = m_files[dictionaryName];
            if (isDicFile) {
                QNTRACE(QStringLiteral("Adding dic file ") << fileInfo.absoluteFilePath());
                pair.first = fileInfo.absoluteFilePath();
            }
            else {
                QNTRACE(QStringLiteral("Adding aff file ") << fileInfo.absoluteFilePath());
                pair.second = fileInfo.absoluteFilePath();
            }
        }
    }

    // Filter out any incomplete pair of dic & aff files
    for(auto it = m_files.begin(); it != m_files.end(); )
    {
        CHECK_AND_STOP()

        const QPair<QString, QString> & pair = it.value();
        if (pair.first.isEmpty() || pair.second.isEmpty()) {
            QNTRACE(QStringLiteral("Skipping the incomplete pair of dic/aff files: dic file path = ")
                    << pair.first << QStringLiteral("; aff file path = ") << pair.second);
            it = m_files.erase(it);
            continue;
        }

        ++it;
    }

    QNDEBUG(QStringLiteral("Found ") << m_files.size() << QStringLiteral(" valid dictionaries"));
    Q_EMIT foundDictionaries(m_files);
}

} // namespace quentier

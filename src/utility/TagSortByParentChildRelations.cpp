/*
 * Copyright 2017 Dmitry Ivanov
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

#include "TagSortByParentChildRelationsHelpers.hpp"
#include "tag_topological_sort/TagDirectedGraphDepthFirstSearch.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

template <class T>
bool sortTagsByParentChildRelationsImpl(QList<T> & tagList, ErrorString & errorDescription)
{
    if (QuentierIsLogLevelActive(LogLevel::TraceLevel))
    {
        QString log;
        QTextStream strm(&log);
        strm << QStringLiteral("Tags list before performing the topological sort: ");
        for(auto it = tagList.constBegin(), end = tagList.constEnd(); it != end; ++it) {
            strm << *it << QStringLiteral(", ");
        }
        strm.flush();
        QNTRACE(log);
    }

    if (tagList.isEmpty() || (tagList.size() == 1)) {
        // Don't need to process the single item list in any way
        return true;
    }

    // The problem of sorting tags by parent-child relations can be viewed as a problem of performing
    // a topological sort in a directed acyclic graph; for parentless tags we can consider their parent guid
    // to be just an empty string i.e. the parent of parentless tags has empty guid

    bool allTagsHaveGuids = true;
    for(auto it = tagList.constBegin(), end = tagList.constEnd(); it != end; ++it)
    {
        if (!tagHasGuid(*it)) {
            allTagsHaveGuids = false;
            QNDEBUG(QStringLiteral("Not all tags have guids, won't use guids to track parent-child relations"));
            break;
        }
    }

    if (!allTagsHaveGuids)
    {
        bool allTagsHaveLocalUids = true;
        for(auto it = tagList.constBegin(), end = tagList.constEnd(); it != end; ++it)
        {
            if (!tagHasLocalUid(*it)) {
                allTagsHaveLocalUids = false;
                break;
            }
        }

        if (!allTagsHaveLocalUids) {
            errorDescription.setBase(QT_TRANSLATE_NOOP("sortTagsByParentChildRelationsImpl",
                                                       "Can't synchronize tags: all tags must have either guids or local uids to be sorted by parent-child relations"));
            return false;
        }
    }

    TagDirectedGraph graph;
    for(auto it = tagList.constBegin(), end = tagList.constEnd(); it != end; ++it)
    {
        if (allTagsHaveGuids && tagHasGuid(*it)) {
            QString guid = tagGuid(*it);
            QString parentTagGuid = tagParentGuid(*it);
            graph.addChild(parentTagGuid, guid);
        }
        else if (tagHasLocalUid(*it)) {
            QString localUid = tagLocalUid(*it);
            QString parentTagLocalUid = tagParentLocalUid(*it);
            QNTRACE(QStringLiteral("Adding tag local uid ") << localUid << QStringLiteral(" and tag parent local uid ")
                    << parentTagLocalUid << QStringLiteral(" to the graph"));
            graph.addChild(parentTagLocalUid, localUid);
        }
        else {
            QNTRACE(QStringLiteral("Skipping tag without either guid or local uid: ") << *it);
        }
    }

    TagDirectedGraphDepthFirstSearch dfs(graph);
    if (Q_UNLIKELY(dfs.hasCycle()))
    {
        errorDescription.setBase(QT_TRANSLATE_NOOP("sortTagsByParentChildRelationsImpl",
                                                   "Can't synchronize tags: detected cycle of parent-child relations between tags"));
        errorDescription.details() = QStringLiteral("cycled tag guids: ");
        QStack<QString> stack = dfs.cycle();
        while(!stack.isEmpty()) {
            errorDescription.details() += stack.pop() + QStringLiteral(", ");
            stack.pop();
        }

        return false;
    }

    QStack<QString> order = dfs.tagIdsInReversePostOrder();
    QList<T> resultList;
    resultList.reserve(tagList.size());
    while(!order.isEmpty())
    {
        QString id = order.pop();
        if (id.isEmpty()) {
            continue;
        }

        auto it = tagList.end();
        if (allTagsHaveGuids) {
            it = std::find_if(tagList.begin(), tagList.end(), CompareItemByGuid<T>(id));
        }
        else {
            it = std::find_if(tagList.begin(), tagList.end(), CompareItemByLocalUid<T>(id));
        }

        if (Q_UNLIKELY(it == tagList.end())) {
            QNDEBUG(QStringLiteral("Skipping the tag guid or local uid not found within the original set "
                                   "(probably the guid of some parent tag not present within the sorted subset): ") << id);
            continue;
        }

        resultList << *it;
        Q_UNUSED(tagList.erase(it))
    }

    tagList = resultList;

    if (QuentierIsLogLevelActive(LogLevel::TraceLevel))
    {
        QString log;
        QTextStream strm(&log);
        strm << QStringLiteral("Tags list after performing the topological sort: ");
        for(auto it = tagList.constBegin(), end = tagList.constEnd(); it != end; ++it) {
            strm << *it << QStringLiteral(", ");
        }
        strm.flush();
        QNTRACE(log);
    }

    return true;
}

bool sortTagsByParentChildRelations(QList<qevercloud::Tag> & tagList, ErrorString & errorDescription)
{
    return sortTagsByParentChildRelationsImpl(tagList, errorDescription);
}

bool sortTagsByParentChildRelations(QList<Tag> & tagList, ErrorString errorDescription)
{
    return sortTagsByParentChildRelationsImpl(tagList, errorDescription);
}

}

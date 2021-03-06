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

#ifndef LIB_QUENTIER_TESTS_LOCAL_STORAGE_MANAGER_TESTS_H
#define LIB_QUENTIER_TESTS_LOCAL_STORAGE_MANAGER_TESTS_H

#include <QtGlobal>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManager)
QT_FORWARD_DECLARE_CLASS(SavedSearch)
QT_FORWARD_DECLARE_CLASS(LinkedNotebook)
QT_FORWARD_DECLARE_CLASS(Tag)
QT_FORWARD_DECLARE_CLASS(Resource)
QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(Notebook)
QT_FORWARD_DECLARE_CLASS(User)

namespace test {

bool TestSavedSearchAddFindUpdateExpungeInLocalStorage(QString & errorDescription);

bool TestLinkedNotebookAddFindUpdateExpungeInLocalStorage(QString & errorDescription);

bool TestTagAddFindUpdateExpungeInLocalStorage(QString & errorDescription);

bool TestResourceAddFindUpdateExpungeInLocalStorage(QString & errorDescription);

bool TestNoteFindUpdateDeleteExpungeInLocalStorage(QString & errorDescription);

bool TestNotebookFindUpdateDeleteExpungeInLocalStorage(QString & errorDescription);

bool TestUserAddFindUpdateDeleteExpungeInLocalStorage(QString & errorDescription);

bool TestSequentialUpdatesInLocalStorage(QString & errorDescription);

bool TestAccountHighUsnInLocalStorage(QString & errorDescription);

bool TestAddingNoteWithoutLocalUid(QString & errorDescription);

bool TestNoteTagIdsComplementWhenAddingAndUpdatingNote(QString & errorDescription);

} // namespace test
} // namespace quentier

#endif // LIB_QUENTIER_TESTS_LOCAL_STORAGE_MANAGER_TESTS_H

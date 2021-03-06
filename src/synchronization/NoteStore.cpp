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

#include "NoteStore.h"
#include "ExceptionHandlingHelpers.h"
#include <quentier/types/Notebook.h>
#include <quentier/types/Note.h>
#include <quentier/types/Resource.h>
#include <quentier/types/Tag.h>
#include <quentier/types/SavedSearch.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/QuentierCheckPtr.h>

namespace quentier {

#define SET_EDAM_USER_EXCEPTION_ERROR(userException) \
    errorDescription.setBase(QT_TRANSLATE_NOOP("NoteStore", "caught EDAM user exception")); \
    errorDescription.details() = QStringLiteral("error code"); \
    errorDescription.details() += ToString(userException.errorCode); \
    if (!userException.exceptionData().isNull()) { \
        errorDescription.details() += QStringLiteral(": "); \
        errorDescription.details() += userException.exceptionData()->errorMessage; \
    }

NoteStore::NoteStore(QSharedPointer<qevercloud::NoteStore> pQecNoteStore, QObject * parent) :
    QObject(parent),
    m_pQecNoteStore(pQecNoteStore),
    m_noteGuidByAsyncResultPtr()
{
    QUENTIER_CHECK_PTR(m_pQecNoteStore)
}

NoteStore::~NoteStore()
{
    stop();
}

void NoteStore::stop()
{
    QNDEBUG(QStringLiteral("NoteStore::stop"));

    for(auto it = m_noteGuidByAsyncResultPtr.begin(), end = m_noteGuidByAsyncResultPtr.end(); it != end; ++it)
    {
        qevercloud::AsyncResult * pAsyncResult = it.key();
        if (Q_UNLIKELY(!pAsyncResult)) {
            continue;
        }

        QObject::disconnect(pAsyncResult, QNSIGNAL(qevercloud::AsyncResult,finished,QVariant,QSharedPointer<EverCloudExceptionData>),
                            this, QNSLOT(NoteStore,onGetNoteAsyncFinished,QVariant,QSharedPointer<EverCloudExceptionData>));
    }

    m_noteGuidByAsyncResultPtr.clear();
}

QSharedPointer<qevercloud::NoteStore> NoteStore::getQecNoteStore()
{
    return m_pQecNoteStore;
}

QString NoteStore::noteStoreUrl() const
{
    return m_pQecNoteStore->noteStoreUrl();
}

void NoteStore::setNoteStoreUrl(const QString & noteStoreUrl)
{
    m_pQecNoteStore->setNoteStoreUrl(noteStoreUrl);
}

QString NoteStore::authenticationToken() const
{
    return m_pQecNoteStore->authenticationToken();
}

void NoteStore::setAuthenticationToken(const QString & authToken)
{
    m_pQecNoteStore->setAuthenticationToken(authToken);
}

qint32 NoteStore::createNotebook(Notebook & notebook, ErrorString & errorDescription,
                                 qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken)
{
    try
    {
        notebook.qevercloudNotebook() = m_pQecNoteStore->createNotebook(notebook.qevercloudNotebook(), linkedNotebookAuthToken);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserExceptionForNotebook(notebook, userException,
                                                   UserExceptionSource::Creation,
                                                   errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }
    CATCH_GENERIC_EXCEPTIONS_NO_RET()

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::updateNotebook(Notebook & notebook, ErrorString & errorDescription,
                                 qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken)
{
    try
    {
        qint32 usn = m_pQecNoteStore->updateNotebook(notebook.qevercloudNotebook(), linkedNotebookAuthToken);
        notebook.setUpdateSequenceNumber(usn);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserExceptionForNotebook(notebook, userException,
                                                   UserExceptionSource::Update,
                                                   errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        processEdamNotFoundException(notFoundException, errorDescription);
    }
    CATCH_GENERIC_EXCEPTIONS_NO_RET()

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::createNote(Note & note, ErrorString & errorDescription, qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken)
{
    try
    {
        qevercloud::Note noteMetadata = m_pQecNoteStore->createNote(note.qevercloudNote(), linkedNotebookAuthToken);
        QNDEBUG(QStringLiteral("Note metadata returned from createNote method: ") << noteMetadata);

        if (noteMetadata.guid.isSet()) {
            note.setGuid(noteMetadata.guid.ref());
        }

        if (noteMetadata.updateSequenceNum.isSet()) {
            note.setUpdateSequenceNumber(noteMetadata.updateSequenceNum);
        }

        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserExceptionForNote(note, userException, UserExceptionSource::Creation,
                                               errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }
    CATCH_GENERIC_EXCEPTIONS_NO_RET()

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::updateNote(Note & note, ErrorString & errorDescription, qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken)
{
    try
    {
        qevercloud::Note noteMetadata = m_pQecNoteStore->updateNote(note.qevercloudNote(), linkedNotebookAuthToken);
        QNDEBUG(QStringLiteral("Note metadata returned from updateNote method: ") << noteMetadata);

        if (noteMetadata.guid.isSet()) {
            note.setGuid(noteMetadata.guid.ref());
        }

        if (noteMetadata.updateSequenceNum.isSet()) {
            note.setUpdateSequenceNumber(noteMetadata.updateSequenceNum);
        }

        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserExceptionForNote(note, userException, UserExceptionSource::Update,
                                               errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        processEdamNotFoundException(notFoundException, errorDescription);
    }
    CATCH_GENERIC_EXCEPTIONS_NO_RET()

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::createTag(Tag & tag, ErrorString & errorDescription, qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken)
{
    try
    {
        tag.qevercloudTag() = m_pQecNoteStore->createTag(tag.qevercloudTag(), linkedNotebookAuthToken);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserExceptionForTag(tag, userException,
                                              UserExceptionSource::Creation,
                                              errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }
    CATCH_GENERIC_EXCEPTIONS_NO_RET()

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::updateTag(Tag & tag, ErrorString & errorDescription, qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken)
{
    try
    {
        qint32 usn = m_pQecNoteStore->updateTag(tag.qevercloudTag(), linkedNotebookAuthToken);
        tag.setUpdateSequenceNumber(usn);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserExceptionForTag(tag, userException,
                                              UserExceptionSource::Update,
                                              errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        processEdamNotFoundException(notFoundException, errorDescription);
    }
    CATCH_GENERIC_EXCEPTIONS_NO_RET()

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::createSavedSearch(SavedSearch & savedSearch, ErrorString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        savedSearch.qevercloudSavedSearch() = m_pQecNoteStore->createSearch(savedSearch.qevercloudSavedSearch());
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        processEdamUserExceptionForSavedSearch(savedSearch, userException,
                                               UserExceptionSource::Creation,
                                               errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }
    CATCH_GENERIC_EXCEPTIONS_NO_RET()

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::updateSavedSearch(SavedSearch & savedSearch, ErrorString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        qint32 usn = m_pQecNoteStore->updateSearch(savedSearch.qevercloudSavedSearch());
        savedSearch.setUpdateSequenceNumber(usn);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        processEdamUserExceptionForSavedSearch(savedSearch, userException,
                                               UserExceptionSource::Update,
                                               errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        processEdamNotFoundException(notFoundException, errorDescription);
    }
    CATCH_GENERIC_EXCEPTIONS_NO_RET()

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::getSyncState(qevercloud::SyncState & syncState, ErrorString & errorDescription,
                               qint32 & rateLimitSeconds)
{
    try
    {
        syncState = m_pQecNoteStore->getSyncState();
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        SET_EDAM_USER_EXCEPTION_ERROR(userException)
        return userException.errorCode;
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription, rateLimitSeconds);
    }
    CATCH_GENERIC_EXCEPTIONS_NO_RET()

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::getSyncChunk(const qint32 afterUSN, const qint32 maxEntries,
                               const qevercloud::SyncChunkFilter & filter,
                               qevercloud::SyncChunk & syncChunk,
                               ErrorString & errorDescription, qint32 & rateLimitSeconds)
{
    QNDEBUG(QStringLiteral("NoteStore::getSyncChunk: after USN = ") << afterUSN
            << QStringLiteral(", max entries = ") << maxEntries
            << QStringLiteral(", sync chunk filter = ") << filter);

    try
    {
        syncChunk = m_pQecNoteStore->getFilteredSyncChunk(afterUSN, maxEntries, filter);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserExceptionForGetSyncChunk(userException, afterUSN,
                                                       maxEntries, errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }
    CATCH_GENERIC_EXCEPTIONS_NO_RET()

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::getLinkedNotebookSyncState(const qevercloud::LinkedNotebook & linkedNotebook,
                                             const QString & authToken, qevercloud::SyncState & syncState,
                                             ErrorString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        syncState = m_pQecNoteStore->getLinkedNotebookSyncState(linkedNotebook, authToken);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        SET_EDAM_USER_EXCEPTION_ERROR(userException)
        return userException.errorCode;
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        errorDescription.setBase(QT_TR_NOOP("caught EDAM not found exception, could not find "
                                            "linked notebook to get the sync state for"));
        if (!notFoundException.exceptionData().isNull()) {
            errorDescription.details() += ToString(notFoundException.exceptionData()->errorMessage);
        }

        return qevercloud::EDAMErrorCode::UNKNOWN;
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription, rateLimitSeconds);
    }
    CATCH_GENERIC_EXCEPTIONS_NO_RET()

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::getLinkedNotebookSyncChunk(const qevercloud::LinkedNotebook & linkedNotebook,
                                             const qint32 afterUSN, const qint32 maxEntries,
                                             const QString & linkedNotebookAuthToken,
                                             const bool fullSyncOnly, qevercloud::SyncChunk & syncChunk,
                                             ErrorString & errorDescription, qint32 & rateLimitSeconds)
{
    QNDEBUG(QStringLiteral("NoteStore::getLinkedNotebookSyncChunk: linked notebook: ") << linkedNotebook
            << QStringLiteral("\nAfter USN = ") << afterUSN << QStringLiteral(", max entries = ") << maxEntries
            << QStringLiteral(", full sync only = ") << (fullSyncOnly ? QStringLiteral("true") : QStringLiteral("false")));

    try
    {
        syncChunk = m_pQecNoteStore->getLinkedNotebookSyncChunk(linkedNotebook, afterUSN,
                                                                maxEntries, fullSyncOnly,
                                                                linkedNotebookAuthToken);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserExceptionForGetSyncChunk(userException, afterUSN,
                                                       maxEntries, errorDescription);
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        errorDescription.setBase(QT_TR_NOOP("caught EDAM not found exception while attempting to "
                                            "download the sync chunk for linked notebook"));
        if (!notFoundException.exceptionData().isNull())
        {
            const QString & errorMessage = notFoundException.exceptionData()->errorMessage;
            if (errorMessage == QStringLiteral("LinkedNotebook")) {
                errorDescription.appendBase(QT_TR_NOOP("the provided information "
                                                       "doesn't match any valid notebook"));
            }
            else if (errorMessage == QStringLiteral("LinkedNotebook.uri")) {
                errorDescription.appendBase(QT_TR_NOOP("the provided public URI doesn't "
                                                       "match any valid notebook"));
            }
            else if (errorMessage == QStringLiteral("SharedNotebook.id")) {
                errorDescription.appendBase(QT_TR_NOOP("the provided information indicates "
                                                       "the shared notebook no longer exists"));
            }
            else {
                errorDescription.appendBase(QT_TR_NOOP("unknown error"));
                errorDescription.details() = errorMessage;
            }
        }

        return qevercloud::EDAMErrorCode::UNKNOWN;
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription, rateLimitSeconds);
    }
    CATCH_GENERIC_EXCEPTIONS_NO_RET()

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::getNote(const bool withContent, const bool withResourcesData,
                          const bool withResourcesRecognition, const bool withResourceAlternateData,
                          Note & note, ErrorString & errorDescription, qint32 & rateLimitSeconds)
{
    QNDEBUG(QStringLiteral("NoteStore::getNote: with content = ") << (withContent ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", with resources data = ") << (withResourcesData ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", with resources recognition = ") << (withResourcesRecognition ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", with resources alternate data = ") << (withResourceAlternateData ? QStringLiteral("true") : QStringLiteral("false")));

    if (!note.hasGuid()) {
        errorDescription.setBase(QT_TR_NOOP("can't get note: note's guid is empty"));
        return qevercloud::EDAMErrorCode::UNKNOWN;
    }

    try
    {
        note.qevercloudNote() = m_pQecNoteStore->getNote(note.guid(), withContent, withResourcesData,
                                                         withResourcesRecognition, withResourceAlternateData);
        note.setLocal(false);
        note.setDirty(false);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserExceptionForGetNote(note.qevercloudNote(), userException, errorDescription);
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        processEdamNotFoundException(notFoundException, errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription, rateLimitSeconds);
    }
    CATCH_GENERIC_EXCEPTIONS_NO_RET()

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

bool NoteStore::getNoteAsync(const bool withContent, const bool withResourceData, const bool withResourcesRecognition,
                             const bool withResourceAlternateData, const bool withSharedNotes,
                             const bool withNoteAppDataValues, const bool withResourceAppDataValues,
                             const bool withNoteLimits, const QString & noteGuid, const QString & authToken,
                             ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("NoteStore::getNoteAsync: with content = ") << (withContent ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", with resource data = ") << (withResourceData ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", with resource recognition = ") << (withResourcesRecognition ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", with resource alternate data = ") << (withResourceAlternateData ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", with shared notes = ") << (withSharedNotes ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", with note app data values = ") << (withNoteAppDataValues ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", with resource app data values = ") << (withResourceAppDataValues ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", with note limits = ") << (withNoteLimits ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", note guid = ") << noteGuid);

    if (Q_UNLIKELY(noteGuid.isEmpty())) {
        errorDescription.setBase(QT_TR_NOOP("Detected the attempt to get full note's data for empty note guid"));
        return false;
    }

    qevercloud::NoteResultSpec noteResultSpec;
    noteResultSpec.includeContent = withContent;
    noteResultSpec.includeResourcesData = withResourceData;
    noteResultSpec.includeResourcesRecognition = withResourcesRecognition;
    noteResultSpec.includeResourcesAlternateData = withResourceAlternateData;
    noteResultSpec.includeSharedNotes = withSharedNotes;
    noteResultSpec.includeNoteAppDataValues = withNoteAppDataValues;
    noteResultSpec.includeResourceAppDataValues = withResourceAppDataValues;
    noteResultSpec.includeAccountLimits = withNoteLimits;
    QNTRACE(QStringLiteral("Note result spec: ") << noteResultSpec);

    qevercloud::AsyncResult * pAsyncResult = m_pQecNoteStore->getNoteWithResultSpecAsync(noteGuid, noteResultSpec, authToken);
    if (Q_UNLIKELY(!pAsyncResult)) {
        errorDescription.setBase(QT_TR_NOOP("Can't get full note data: "
                                            "internal error, QEverCloud library returned "
                                            "null pointer to asynchronous result object"));
        return false;
    }

    m_noteGuidByAsyncResultPtr[pAsyncResult] = noteGuid;

    QObject::connect(pAsyncResult, QNSIGNAL(qevercloud::AsyncResult,finished,QVariant,QSharedPointer<EverCloudExceptionData>),
                     this, QNSLOT(NoteStore,onGetNoteAsyncFinished,QVariant,QSharedPointer<EverCloudExceptionData>));
    return true;
}

qint32 NoteStore::getResource(const bool withDataBody, const bool withRecognitionDataBody,
                              const bool withAlternateDataBody, const bool withAttributes,
                              const QString & authToken, Resource & resource, ErrorString & errorDescription,
                              qint32 & rateLimitSeconds)
{
    QNDEBUG(QStringLiteral("NoteStore::getResource: with data body = ") << (withDataBody ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", with recognition data body = ") << (withRecognitionDataBody ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", with alternate data body = ") << (withAlternateDataBody ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", with attributes = ") << (withAttributes ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", resource guid = ") << (resource.hasGuid() ? resource.guid() : QStringLiteral("<not set>")));

    if (!resource.hasGuid()) {
        errorDescription.setBase(QT_TR_NOOP("can't get resource: resource's guid is empty"));
        return qevercloud::EDAMErrorCode::UNKNOWN;
    }

    try
    {
        resource.qevercloudResource() = m_pQecNoteStore->getResource(resource.guid(), withDataBody, withRecognitionDataBody,
                                                                     withAttributes, withAlternateDataBody, authToken);
        resource.setLocal(false);
        resource.setDirty(false);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserExceptionForGetResource(resource.qevercloudResource(), userException, errorDescription);
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        processEdamNotFoundException(notFoundException, errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription, rateLimitSeconds);
    }
    CATCH_GENERIC_EXCEPTIONS_NO_RET()

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

bool NoteStore::getResourceAsync(const bool withDataBody, const bool withRecognitionDataBody,
                                 const bool withAlternateDataBody, const bool withAttributes,
                                 const QString & resourceGuid, const QString & authToken, ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("NoteStore::getResourceAsync: with data body = ") << (withDataBody ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", with recognition data body = ") << (withRecognitionDataBody ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", with alternate data body = ") << (withAlternateDataBody ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", with attributes = ") << (withAttributes ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", resource guid = ") << resourceGuid);

    if (Q_UNLIKELY(resourceGuid.isEmpty())) {
        errorDescription.setBase(QT_TR_NOOP("Detected the attempt to get full resource's data for empty resource guid"));
        return false;
    }

    qevercloud::AsyncResult * pAsyncResult = m_pQecNoteStore->getResourceAsync(resourceGuid, withDataBody, withRecognitionDataBody,
                                                                               withAttributes, withAlternateDataBody, authToken);
    if (Q_UNLIKELY(!pAsyncResult)) {
        errorDescription.setBase(QT_TR_NOOP("Can't get full resource data: internal error, QEverCloud library returned "
                                            "null pointer to asynchronous result object"));
        return false;
    }

    m_resourceGuidByAsyncResultPtr[pAsyncResult] = resourceGuid;

    QObject::connect(pAsyncResult, QNSIGNAL(qevercloud::AsyncResult,finished,QVariant,QSharedPointer<EverCloudExceptionData>),
                     this, QNSLOT(NoteStore,onGetResourceAsyncFinished,QVariant,QSharedPointer<EverCloudExceptionData>));
    return true;
}

qint32 NoteStore::authenticateToSharedNotebook(const QString & shareKey, qevercloud::AuthenticationResult & authResult,
                                               ErrorString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        authResult = m_pQecNoteStore->authenticateToSharedNotebook(shareKey);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        if (userException.errorCode == qevercloud::EDAMErrorCode::DATA_REQUIRED) {
            errorDescription.setBase(QT_TR_NOOP("no valid authentication token for current user"));
        }
        else if (userException.errorCode == qevercloud::EDAMErrorCode::PERMISSION_DENIED) {
            errorDescription.setBase(QT_TR_NOOP("share requires login, and another username has already been bound to this notebook"));
        }
        else {
            errorDescription.setBase(QT_TR_NOOP("unexpected EDAM user exception"));
            errorDescription.details() = QStringLiteral("error code = ");
            errorDescription.details() += ToString(userException.errorCode);
        }

        return userException.errorCode;
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        // NOTE: this means that shared notebook no longer exists. It can happen with
        // shared/linked notebooks from time to time so it shouldn't be really considered
        // an error. Instead, the method would return empty auth result which should indicate
        // the fact of missing shared notebook to the caller
        Q_UNUSED(notFoundException)
        authResult = qevercloud::AuthenticationResult();
        return 0;
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        if (systemException.errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
        {
            if (!systemException.rateLimitDuration.isSet()) {
                errorDescription.setBase(QT_TR_NOOP("QEverCloud error: RATE_LIMIT_REACHED exception "
                                                    "was caught but rateLimitDuration is not set"));
                return qevercloud::EDAMErrorCode::UNKNOWN;
            }

            rateLimitSeconds = systemException.rateLimitDuration;
        }
        else if (systemException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT)
        {
            errorDescription.setBase(QT_TR_NOOP("invalid share key"));
        }
        else if (systemException.errorCode == qevercloud::EDAMErrorCode::INVALID_AUTH)
        {
            errorDescription.setBase(QT_TR_NOOP("bad signature of share key"));
        }
        else
        {
            errorDescription.setBase(QT_TR_NOOP("unexpected EDAM system exception"));
            errorDescription.details() = QStringLiteral("error code = ");
            errorDescription.details() += ToString(systemException.errorCode);
        }

        return systemException.errorCode;
    }
    CATCH_GENERIC_EXCEPTIONS_NO_RET()

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

void NoteStore::onGetNoteAsyncFinished(QVariant result, QSharedPointer<EverCloudExceptionData> exceptionData)
{
    QNDEBUG(QStringLiteral("NoteStore::onGetNoteAsyncFinished"));

    QString noteGuid;

    qevercloud::AsyncResult * pAsyncResult = qobject_cast<qevercloud::AsyncResult*>(sender());
    if (pAsyncResult)
    {
        auto it = m_noteGuidByAsyncResultPtr.find(pAsyncResult);
        if (it != m_noteGuidByAsyncResultPtr.end()) {
            noteGuid = it.value();
            Q_UNUSED(m_noteGuidByAsyncResultPtr.erase(it))
        }
        else {
            QNDEBUG(QStringLiteral("Couldn't find the note guid by async result ptr"));
            return;
        }
    }
    else
    {
        QNDEBUG(QStringLiteral("Couldn't get non-NULL pointer to AsyncResult, hence can't get the note guid "
                               "to which the result corresponds"));
        return;
    }

    qevercloud::Note note;
    note.guid = noteGuid;

    ErrorString errorDescription;
    qint32 errorCode = 0;
    qint32 rateLimitSeconds = -1;

    if (!exceptionData.isNull())
    {
        QNDEBUG(QStringLiteral("Error: ") << exceptionData->errorMessage);

        try
        {
            exceptionData->throwException();
        }
        catch(const qevercloud::EDAMUserException & userException)
        {
            errorCode = processEdamUserExceptionForGetNote(note, userException, errorDescription);
        }
        catch(const qevercloud::EDAMNotFoundException & notFoundException)
        {
            processEdamNotFoundException(notFoundException, errorDescription);
            errorCode = qevercloud::EDAMErrorCode::UNKNOWN;
        }
        catch(const qevercloud::EDAMSystemException & systemException)
        {
            errorCode = processEdamSystemException(systemException, errorDescription, rateLimitSeconds);
        }
        CATCH_GENERIC_EXCEPTIONS_IMPL(errorCode = qevercloud::EDAMErrorCode::UNKNOWN)

        Q_EMIT getNoteAsyncFinished(errorCode, note, rateLimitSeconds, errorDescription);
        return;
    }

    note = result.value<qevercloud::Note>();
    Q_EMIT getNoteAsyncFinished(errorCode, note, rateLimitSeconds, errorDescription);
}

void NoteStore::onGetResourceAsyncFinished(QVariant result, QSharedPointer<EverCloudExceptionData> exceptionData)
{
    QNDEBUG(QStringLiteral("NoteStore::onGetResourceAsyncFinished"));

    QString resourceGuid;

    qevercloud::AsyncResult * pAsyncResult = qobject_cast<qevercloud::AsyncResult*>(sender());
    if (pAsyncResult)
    {
        auto it = m_resourceGuidByAsyncResultPtr.find(pAsyncResult);
        if (it != m_resourceGuidByAsyncResultPtr.end()) {
            resourceGuid = it.value();
            Q_UNUSED(m_resourceGuidByAsyncResultPtr.erase(it))
        }
        else {
            QNDEBUG(QStringLiteral("Couldn't find the resource guid by async result ptr"));
            return;
        }
    }
    else
    {
        QNDEBUG(QStringLiteral("Couldn't get non-NULL pointer to AsyncResult, hence can't get the resource guid "
                               "to which the result corresponds"));
        return;
    }

    qevercloud::Resource resource;
    resource.guid = resourceGuid;

    ErrorString errorDescription;
    qint32 errorCode = 0;
    qint32 rateLimitSeconds = -1;

    if (!exceptionData.isNull())
    {
        QNDEBUG(QStringLiteral("Error: ") << exceptionData->errorMessage);

        try
        {
            exceptionData->throwException();
        }
        catch(const qevercloud::EDAMUserException & userException)
        {
            errorCode = processEdamUserExceptionForGetResource(resource, userException, errorDescription);
        }
        catch(const qevercloud::EDAMNotFoundException & notFoundException)
        {
            processEdamNotFoundException(notFoundException, errorDescription);
            errorCode = qevercloud::EDAMErrorCode::UNKNOWN;
        }
        catch(const qevercloud::EDAMSystemException & systemException)
        {
            errorCode = processEdamSystemException(systemException, errorDescription, rateLimitSeconds);
        }
        CATCH_GENERIC_EXCEPTIONS_IMPL(errorCode = qevercloud::EDAMErrorCode::UNKNOWN)

        Q_EMIT getResourceAsyncFinished(errorCode, resource, rateLimitSeconds, errorDescription);
        return;
    }

    resource = result.value<qevercloud::Resource>();
    Q_EMIT getResourceAsyncFinished(errorCode, resource, rateLimitSeconds, errorDescription);
}

qint32 NoteStore::processEdamUserExceptionForTag(const Tag & tag, const qevercloud::EDAMUserException & userException,
                                                 const NoteStore::UserExceptionSource::type & source,
                                                 ErrorString & errorDescription) const
{
    bool thrownOnCreation = (source == UserExceptionSource::Creation);

    const auto exceptionData = userException.exceptionData();

    if (userException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT)
    {
        if (thrownOnCreation) {
            errorDescription.setBase(QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to create a tag"));
        }
        else {
            errorDescription.setBase(QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to update a tag"));
        }

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription.details() = exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Tag.name"))
        {
            if (tag.hasName()) {
                errorDescription.appendBase(QT_TR_NOOP("invalid length or pattern of tag's name"));
                errorDescription.details() = tag.name();
            }
            else {
                errorDescription.appendBase(QT_TR_NOOP("tag has no name"));
            }
        }
        else if (userException.parameter.ref() == QStringLiteral("Tag.parentGuid"))
        {
            if (tag.hasParentGuid()) {
                errorDescription.appendBase(QT_TR_NOOP("malformed parent guid of tag"));
                errorDescription.details() = tag.parentGuid();
            }
            else {
                errorDescription.appendBase(QT_TR_NOOP("error code indicates malformed parent guid but it is empty"));
            }
        }
        else
        {
            errorDescription.appendBase(QT_TR_NOOP("unexpected parameter"));
            errorDescription.details() = userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::DATA_CONFLICT)
    {
        if (thrownOnCreation) {
            errorDescription.setBase(QT_TR_NOOP("DATA_CONFLICT exception during the attempt to create a tag"));
        }
        else {
            errorDescription.setBase(QT_TR_NOOP("DATA_CONFLICT exception during the attempt to update a tag"));
        }

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription.details() = exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Tag.name"))
        {
            if (tag.hasName()) {
                errorDescription.appendBase(QT_TR_NOOP("invalid length or pattern of tag's name"));
                errorDescription.details() = tag.name();
            }
            else {
                errorDescription.appendBase(QT_TR_NOOP("tag has no name"));
            }
        }

        if (!thrownOnCreation && (userException.parameter.ref() == QStringLiteral("Tag.parentGuid")))
        {
            if (tag.hasParentGuid()) {
                errorDescription.appendBase(QT_TR_NOOP("can't set parent for tag: circular "
                                                       "parent-child correlation detected"));
                errorDescription.details() = tag.parentGuid();
            }
            else {
                errorDescription.appendBase(QT_TR_NOOP("error code indicates the problem with "
                                                       "circular parent-child correlation "
                                                       "but tag's parent guid is empty"));
            }
        }
        else
        {
            errorDescription.appendBase(QT_TR_NOOP("unexpected parameter"));
            errorDescription.details() = userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (thrownOnCreation && (userException.errorCode == qevercloud::EDAMErrorCode::LIMIT_REACHED))
    {
        errorDescription.setBase(QT_TR_NOOP("LIMIT_REACHED exception during the attempt to create a tag"));

        if (userException.parameter.isSet() && (userException.parameter.ref() == QStringLiteral("Tag"))) {
            errorDescription.appendBase(QT_TR_NOOP("already at max number of tags, please remove some of them"));
        }

        return userException.errorCode;
    }
    else if (!thrownOnCreation && (userException.errorCode == qevercloud::EDAMErrorCode::PERMISSION_DENIED))
    {
        errorDescription.setBase(QT_TR_NOOP("PERMISSION_DENIED exception during the attempt to update a tag"));

        if (userException.parameter.isSet() && (userException.parameter.ref() == QStringLiteral("Tag")))
        {
            errorDescription.appendBase(QT_TR_NOOP("user doesn't own the tag, it can't be updated"));
            if (tag.hasName()) {
                errorDescription.details() = tag.name();
            }
        }

        return userException.errorCode;
    }

    return processUnexpectedEdamUserException(QStringLiteral("tag"), userException, source, errorDescription);
}

qint32 NoteStore::processEdamUserExceptionForSavedSearch(const SavedSearch & search,
                                                         const qevercloud::EDAMUserException & userException,
                                                         const NoteStore::UserExceptionSource::type & source,
                                                         ErrorString & errorDescription) const
{
    bool thrownOnCreation = (source == UserExceptionSource::Creation);

    const auto exceptionData = userException.exceptionData();

    if (userException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT)
    {
        if (thrownOnCreation) {
            errorDescription.setBase(QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to create a saved search"));
        }
        else {
            errorDescription.setBase(QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to update a saved search"));
        }

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription.details() = exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("SavedSearch.name"))
        {
            if (search.hasName()) {
                errorDescription.appendBase(QT_TR_NOOP("invalid length or pattern of saved search's name"));
                errorDescription.details() = search.name();
            }
            else {
                errorDescription.appendBase(QT_TR_NOOP("saved search has no name"));
            }
        }
        else if (userException.parameter.ref() == QStringLiteral("SavedSearch.query"))
        {
            if (search.hasQuery()) {
                errorDescription.appendBase(QT_TR_NOOP("invalid length of saved search's query"));
                errorDescription.details() = QString::number(search.query().length());
                QNWARNING(errorDescription << QStringLiteral(", query: ") << search.query());
            }
            else {
                errorDescription.appendBase(QT_TR_NOOP("saved search has no query"));
            }
        }
        else
        {
            errorDescription.appendBase(QT_TR_NOOP("unexpected parameter"));
            errorDescription.details() = userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::DATA_CONFLICT)
    {
        if (thrownOnCreation) {
            errorDescription.setBase(QT_TR_NOOP("DATA_CONFLICT exception during the attempt to create a saved search"));
        }
        else {
            errorDescription.setBase(QT_TR_NOOP("DATA_CONFLICT exception during the attempt to update a saved search"));
        }

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription.details() = exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("SavedSearch.name"))
        {
            if (search.hasName()) {
                errorDescription.appendBase(QT_TR_NOOP("saved search's name is already in use"));
                errorDescription.details() = search.name();
            }
            else {
                errorDescription.appendBase(QT_TR_NOOP("saved search has no name"));
            }
        }
        else
        {
            errorDescription.appendBase(QT_TR_NOOP("unexpected parameter"));
            errorDescription.details() = userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (thrownOnCreation && (userException.errorCode == qevercloud::EDAMErrorCode::LIMIT_REACHED))
    {
        errorDescription.setBase(QT_TR_NOOP("LIMIT_REACHED exception during the attempt to create saved search: "
                                            "already at max number of saved searches"));
        return userException.errorCode;
    }
    else if (!thrownOnCreation && (userException.errorCode == qevercloud::EDAMErrorCode::PERMISSION_DENIED))
    {
        errorDescription.setBase(QT_TR_NOOP("PERMISSION_DENIED exception during the attempt to update saved search: "
                                            "user doesn't own saved search"));
        return userException.errorCode;
    }

    return processUnexpectedEdamUserException(QStringLiteral("saved search"), userException,
                                              source, errorDescription);
}

qint32 NoteStore::processEdamUserExceptionForGetSyncChunk(const qevercloud::EDAMUserException & userException,
                                                          const qint32 afterUSN, const qint32 maxEntries,
                                                          ErrorString & errorDescription) const
{
    const auto exceptionData = userException.exceptionData();

    if (userException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT)
    {
        errorDescription.setBase(QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to get sync chunk"));

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription.details() = exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("afterUSN")) {
            errorDescription.appendBase(QT_TR_NOOP("afterUSN is negative"));
            errorDescription.details() = QString::number(afterUSN);
        }
        else if (userException.parameter.ref() == QStringLiteral("maxEntries")) {
            errorDescription.appendBase(QT_TR_NOOP("maxEntries is less than 1"));
            errorDescription.details() = QString::number(maxEntries);
        }
        else {
            errorDescription.appendBase(QT_TR_NOOP("unexpected parameter"));
            errorDescription.details() = userException.parameter.ref();
        }
    }
    else
    {
        errorDescription.setBase(QT_TR_NOOP("Unknown EDAM user exception on attempt to get sync chunk"));
        if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
            errorDescription.details() = exceptionData->errorMessage;
        }
    }

    return userException.errorCode;
}

qint32 NoteStore::processEdamUserExceptionForGetNote(const qevercloud::Note & note, const qevercloud::EDAMUserException & userException,
                                                     ErrorString & errorDescription) const
{
    Q_UNUSED(note)  // Maybe it'd be actually used in future

    const auto exceptionData = userException.exceptionData();

    if (userException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT)
    {
        errorDescription.setBase(QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to get note"));

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription.details() = exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Note.guid")) {
            errorDescription.appendBase(QT_TR_NOOP("note's guid is missing"));
        }
        else {
            errorDescription.appendBase(QT_TR_NOOP("unexpected parameter"));
            errorDescription.details() = userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::PERMISSION_DENIED)
    {
        errorDescription.setBase(QT_TR_NOOP("PERMISSION_DENIED exception during the attempt to get note"));

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription.details() = exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Note")) {
            errorDescription.appendBase(QT_TR_NOOP("note is not owned by user"));
        }
        else {
            errorDescription.appendBase(QT_TR_NOOP("unexpected parameter"));
            errorDescription.details() = userException.parameter.ref();
        }

        return userException.errorCode;
    }

    errorDescription.setBase(QT_TR_NOOP("Unexpected EDAM user exception on attempt to get note"));
    errorDescription.details() = QStringLiteral("error code = ");
    errorDescription.details() += ToString(userException.errorCode);

    if (userException.parameter.isSet()) {
        errorDescription.details() += QStringLiteral("; parameter: ");
        errorDescription.details() += userException.parameter.ref();
    }

    return userException.errorCode;
}

qint32 NoteStore::processEdamUserExceptionForGetResource(const qevercloud::Resource & resource,
                                                         const qevercloud::EDAMUserException & userException,
                                                         ErrorString & errorDescription) const
{
    Q_UNUSED(resource)  // Maybe it'd be actually used in future

    const auto exceptionData = userException.exceptionData();

    if (userException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT)
    {
        errorDescription.setBase(QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to get resource"));

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription.details() = exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Resource.guid")) {
            errorDescription.appendBase(QT_TR_NOOP("resource's guid is missing"));
        }
        else {
            errorDescription.appendBase(QT_TR_NOOP("unexpected parameter"));
            errorDescription.details() = userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::PERMISSION_DENIED)
    {
        errorDescription.setBase(QT_TR_NOOP("PERMISSION_DENIED exception during the attempt to get resource"));

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription.details() = exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Resource")) {
            errorDescription.appendBase(QT_TR_NOOP("resource is not owned by user"));
        }
        else {
            errorDescription.appendBase(QT_TR_NOOP("unexpected parameter"));
            errorDescription.details() = userException.parameter.ref();
        }

        return userException.errorCode;
    }

    errorDescription.setBase(QT_TR_NOOP("Unexpected EDAM user exception on attempt to get resource"));
    errorDescription.details() = QStringLiteral("error code = ");
    errorDescription.details() += ToString(userException.errorCode);

    if (userException.parameter.isSet()) {
        errorDescription.details() += QStringLiteral("; parameter: ");
        errorDescription.details() += userException.parameter.ref();
    }

    return userException.errorCode;
}

qint32 NoteStore::processEdamUserExceptionForNotebook(const Notebook & notebook,
                                                      const qevercloud::EDAMUserException & userException,
                                                      const NoteStore::UserExceptionSource::type & source,
                                                      ErrorString & errorDescription) const
{
    bool thrownOnCreation = (source == UserExceptionSource::Creation);

    const auto exceptionData = userException.exceptionData();

    if (userException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT)
    {
        if (thrownOnCreation) {
            errorDescription.setBase(QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to create a notebook"));
        }
        else {
            errorDescription.setBase(QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to update a notebook"));
        }

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription.details() = exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Notebook.name"))
        {
            if (notebook.hasName()) {
                errorDescription.appendBase(QT_TR_NOOP("invalid length or pattern of notebook's name"));
                errorDescription.details() = notebook.name();
            }
            else {
                errorDescription.appendBase(QT_TR_NOOP("notebook has no name"));
            }
        }
        else if (userException.parameter.ref() == QStringLiteral("Notebook.stack"))
        {
            if (notebook.hasStack()) {
                errorDescription.appendBase(QT_TR_NOOP("invalid length or pattern of notebook's stack"));
                errorDescription.details() = notebook.stack();
            }
            else {
                errorDescription.appendBase(QT_TR_NOOP("notebook has no stack"));
            }
        }
        else if (userException.parameter.ref() == QStringLiteral("Publishing.uri"))
        {
            if (notebook.hasPublishingUri()) {
                errorDescription.appendBase(QT_TR_NOOP("invalid publishing uri for notebook"));
                errorDescription.details() = notebook.publishingUri();
            }
            else {
                errorDescription.appendBase(QT_TR_NOOP("notebook has no publishing uri"));
            }
        }
        else if (userException.parameter.ref() == QStringLiteral("Publishing.publicDescription"))
        {
            if (notebook.hasPublishingPublicDescription()) {
                errorDescription.appendBase(QT_TR_NOOP("public description for notebook is too long"));
                errorDescription.details() = notebook.publishingPublicDescription();
            }
            else {
                errorDescription.appendBase(QT_TR_NOOP("notebook has no public description"));
            }
        }
        else
        {
            errorDescription.appendBase(QT_TR_NOOP("unexpected parameter"));
            errorDescription.details() = userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::DATA_CONFLICT)
    {
        if (thrownOnCreation) {
            errorDescription.setBase(QT_TR_NOOP("DATA_CONFLICT exception during the attempt to create a notebook"));
        }
        else {
            errorDescription.setBase(QT_TR_NOOP("DATA_CONFLICT exception during the attempt to update a notebook"));
        }

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription.details() = exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Notebook.name"))
        {
            if (notebook.hasName()) {
                errorDescription.appendBase(QT_TR_NOOP("notebook's name is already in use"));
                errorDescription.details() = notebook.name();
            }
            else {
                errorDescription.appendBase(QT_TR_NOOP("notebook has no name"));
            }
        }
        else if (userException.parameter.ref() == QStringLiteral("Publishing.uri"))
        {
            if (notebook.hasPublishingUri()) {
                errorDescription.appendBase(QT_TR_NOOP("notebook's publishing uri is already in use"));
                errorDescription.details() = notebook.publishingUri();
            }
            else {
                errorDescription.appendBase(QT_TR_NOOP("notebook has no publishing uri"));
            }
        }
        else
        {
            errorDescription.appendBase(QT_TR_NOOP("unexpected parameter"));
            errorDescription.details() = userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (thrownOnCreation && (userException.errorCode == qevercloud::EDAMErrorCode::LIMIT_REACHED))
    {
        errorDescription.setBase(QT_TR_NOOP("LIMIT_REACHED exception during the attempt to create notebook"));

        if (userException.parameter.isSet() && (userException.parameter.ref() == QStringLiteral("Notebook"))) {
            errorDescription.appendBase(QT_TR_NOOP("already at max number of notebooks, please remove some of them"));
        }

        return userException.errorCode;
    }

    return processUnexpectedEdamUserException(QStringLiteral("notebook"), userException, source, errorDescription);
}

qint32 NoteStore::processEdamUserExceptionForNote(const Note & note, const qevercloud::EDAMUserException & userException,
                                                  const NoteStore::UserExceptionSource::type & source,
                                                  ErrorString & errorDescription) const
{
    bool thrownOnCreation = (source == UserExceptionSource::Creation);

    const auto exceptionData = userException.exceptionData();

    if (userException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT)
    {
        if (thrownOnCreation) {
            errorDescription.setBase(QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to create a note"));
        }
        else {
            errorDescription.setBase(QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to update a note"));
        }

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription.details() = exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Note.title"))
        {
            if (note.hasTitle()) {
                errorDescription.appendBase(QT_TR_NOOP("invalid length or pattetn of note's title"));
                errorDescription.details() = note.title();
            }
            else {
                errorDescription.appendBase(QT_TR_NOOP("note has no title"));
            }
        }
        else if (userException.parameter.ref() == QStringLiteral("Note.content"))
        {
            if (note.hasContent()) {
                errorDescription.appendBase(QT_TR_NOOP("invalid length for note's ENML content"));
                errorDescription.details() = QString::number(note.content().length());
                QNWARNING(errorDescription << QStringLiteral(", note's content: ") << note.content());
            }
            else {
                errorDescription.appendBase(QT_TR_NOOP("note has no content"));
            }
        }
        else if (userException.parameter.ref().startsWith(QStringLiteral("NoteAttributes.")))
        {
            if (note.hasNoteAttributes()) {
                errorDescription.appendBase(QT_TR_NOOP("invalid note attributes"));
                QNWARNING(errorDescription << QStringLiteral(": ") << note.noteAttributes());
            }
            else {
                errorDescription.appendBase(QT_TR_NOOP("note has no attributes"));
            }
        }
        else if (userException.parameter.ref().startsWith(QStringLiteral("ResourceAttributes.")))
        {
            errorDescription.appendBase(QT_TR_NOOP("invalid resource attributes for some of note's resources"));
            QNWARNING(errorDescription << QStringLiteral(", note: ") << note);
        }
        else if (userException.parameter.ref() == QStringLiteral("Resource.mime"))
        {
            errorDescription.appendBase(QT_TR_NOOP("invalid mime type for some of note's resources"));
            QNWARNING(errorDescription << QStringLiteral(", note: ") << note);
        }
        else if (userException.parameter.ref() == QStringLiteral("Tag.name"))
        {
            errorDescription.appendBase(QT_TR_NOOP("Note.tagNames was provided and one of the specified tags "
                                                   "had invalid length or pattern"));
            QNWARNING(errorDescription << QStringLiteral(", note: ") << note);
        }
        else {
            errorDescription.appendBase(QT_TR_NOOP("unexpected parameter"));
            errorDescription.details() = userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::DATA_CONFLICT)
    {
        if (thrownOnCreation) {
            errorDescription.setBase(QT_TR_NOOP("DATA_CONFLICT exception during the attempt to create a note"));
        }
        else {
            errorDescription.setBase(QT_TR_NOOP("DATA_CONFLICT exception during the attempt to update a note"));
        }

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription.details() = exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Note.deleted")) {
            errorDescription.appendBase(QT_TR_NOOP("deletion timestamp is set on active note"));
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::DATA_REQUIRED)
    {
        if (thrownOnCreation) {
            errorDescription.setBase(QT_TR_NOOP("DATA_REQUIRED exception during the attempt to create a note"));
        }
        else {
            errorDescription.setBase(QT_TR_NOOP("DATA_REQUIRED exception during the attempt to update a note"));
        }

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription.details() = exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Resource.data")) {
            errorDescription.appendBase(QT_TR_NOOP("data body for some of note's resources is missing"));
            QNWARNING(errorDescription << QStringLiteral(", note: ") << note);
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::ENML_VALIDATION)
    {
        if (thrownOnCreation) {
            errorDescription.setBase(QT_TR_NOOP("ENML_VALIDATION exception during the attempt to create a note"));
        }
        else {
            errorDescription.setBase(QT_TR_NOOP("ENML_VALIDATION exception during the attempt to update a note"));
        }

        errorDescription.appendBase(QT_TR_NOOP("note's content doesn't validate against DTD"));
        QNWARNING(errorDescription << QStringLiteral(", note: ") << note);
        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::LIMIT_REACHED)
    {
        if (thrownOnCreation) {
            errorDescription.setBase(QT_TR_NOOP("LIMIT_REACHED exception during the attempt to create a note"));
        }
        else {
            errorDescription.setBase(QT_TR_NOOP("LIMIT_REACHED exception during the attempt to update a note"));
        }

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription.details() = exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (thrownOnCreation && (userException.parameter.ref() == QStringLiteral("Note")))
        {
            errorDescription.appendBase(QT_TR_NOOP("already at maximum number of notes per account"));
        }
        else if (userException.parameter.ref() == QStringLiteral("Note.size"))
        {
            errorDescription.appendBase(QT_TR_NOOP("total note size is too large"));
        }
        else if (userException.parameter.ref() == QStringLiteral("Note.resources"))
        {
            errorDescription.appendBase(QT_TR_NOOP("too many resources on note"));
        }
        else if (userException.parameter.ref() == QStringLiteral("Note.tagGuids"))
        {
            errorDescription.appendBase(QT_TR_NOOP("too many tags on note"));
        }
        else if (userException.parameter.ref() == QStringLiteral("Resource.data.size"))
        {
            errorDescription.appendBase(QT_TR_NOOP("one of note's resource's data is too large"));
        }
        else if (userException.parameter.ref().startsWith(QStringLiteral("NoteAttribute.")))
        {
            errorDescription.appendBase(QT_TR_NOOP("note attributes string is too large"));
            if (note.hasNoteAttributes()) {
                QNWARNING(errorDescription << QStringLiteral(", note attributes: ") << note.noteAttributes());
            }
        }
        else if (userException.parameter.ref().startsWith(QStringLiteral("ResourceAttribute.")))
        {
            errorDescription.appendBase(QT_TR_NOOP("one of note's resources has too "
                                                   "large resource attributes string"));
            QNWARNING(errorDescription << QStringLiteral(", note: ") << note);
        }
        else if (userException.parameter.ref() == QStringLiteral("Tag"))
        {
            errorDescription.appendBase(QT_TR_NOOP("Note.tagNames was provided, and the required new tags "
                                                   "would exceed the maximum number per account"));
            QNWARNING(errorDescription << QStringLiteral(", note: ") << note);
        }
        else
        {
            errorDescription.appendBase(QT_TR_NOOP("unexpected parameter"));
            errorDescription.details() = userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::PERMISSION_DENIED)
    {
        if (thrownOnCreation) {
            errorDescription.setBase(QT_TR_NOOP("PERMISSION_DENIED exception during the attempt to create a note"));
        }
        else {
            errorDescription.setBase(QT_TR_NOOP("PERMISSION_DENIED exception during the attempt to update a note"));
        }

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription.details() = exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Note.notebookGuid"))
        {
            errorDescription.appendBase(QT_TR_NOOP("note's notebook is not owned by user"));
            if (note.hasNotebookGuid()) {
                QNWARNING(errorDescription << QStringLiteral(", notebook guid: ") << note.notebookGuid());
            }
        }
        else if (!thrownOnCreation && (userException.parameter.ref() == QStringLiteral("Note")))
        {
            errorDescription.appendBase(QT_TR_NOOP("note is not owned by user"));
        }
        else
        {
            errorDescription.appendBase(QT_TR_NOOP("unexpected parameter"));
            errorDescription.details() = userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::QUOTA_REACHED)
    {
        if (thrownOnCreation) {
            errorDescription.setBase(QT_TR_NOOP("QUOTA_REACHED exception during the attempt to create a note"));
        }
        else {
            errorDescription.setBase(QT_TR_NOOP("QUOTA_REACHED exception during the attempt to update a note"));
        }

        errorDescription.appendBase(QT_TR_NOOP("note exceeds upload quota limit"));
        return userException.errorCode;
    }

    return processUnexpectedEdamUserException(QStringLiteral("note"), userException, source, errorDescription);
}

qint32 NoteStore::processUnexpectedEdamUserException(const QString & typeName,
                                                     const qevercloud::EDAMUserException & userException,
                                                     const NoteStore::UserExceptionSource::type & source,
                                                     ErrorString & errorDescription) const
{
    bool thrownOnCreation = (source == UserExceptionSource::Creation);

    if (thrownOnCreation) {
        errorDescription.setBase(QT_TR_NOOP("Unexpected EDAM user exception on attempt to create data element"));
    }
    else {
        errorDescription.setBase(QT_TR_NOOP("Unexpected EDAM user exception on attempt to update data element"));
    }

    errorDescription.details() = typeName;
    errorDescription.details() += QStringLiteral(", error code = ");
    errorDescription.details() += ToString(userException.errorCode);

    if (userException.parameter.isSet()) {
        errorDescription.details() += QStringLiteral(", parameter = ");
        errorDescription.details() += userException.parameter.ref();
    }

    return userException.errorCode;
}

qint32 NoteStore::processEdamSystemException(const qevercloud::EDAMSystemException & systemException,
                                             ErrorString & errorDescription, qint32 & rateLimitSeconds) const
{
    rateLimitSeconds = -1;

    if (systemException.errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
    {
        if (!systemException.rateLimitDuration.isSet())
        {
            errorDescription.setBase(QT_TR_NOOP("Evernote API rate limit exceeded but no "
                                                "rate limit duration is available"));
        }
        else
        {
            errorDescription.setBase(QT_TR_NOOP("Evernote API rate limit exceeded, retry in"));
            errorDescription.details() = QString::number(systemException.rateLimitDuration.ref());
            errorDescription.details() += QStringLiteral(" sec");
            rateLimitSeconds = systemException.rateLimitDuration.ref();
        }
    }
    else
    {
        errorDescription.setBase(QT_TR_NOOP("Caught EDAM system exception"));
        errorDescription.details() = QStringLiteral("error code = ");
        errorDescription.details() += ToString(systemException.errorCode);

        if (systemException.message.isSet() && !systemException.message->isEmpty()) {
            errorDescription.details() += QStringLiteral(": ");
            errorDescription.details() += systemException.message.ref();
        }
    }

    return systemException.errorCode;
}

void NoteStore::processEdamNotFoundException(const qevercloud::EDAMNotFoundException & notFoundException,
                                             ErrorString & errorDescription) const
{
    errorDescription.setBase(QT_TR_NOOP("Note store could not find data element"));

    if (notFoundException.identifier.isSet() && !notFoundException.identifier->isEmpty()) {
        errorDescription.details() = notFoundException.identifier.ref();
    }

    if (notFoundException.key.isSet() && !notFoundException.key->isEmpty()) {
        errorDescription.details() = notFoundException.key.ref();
    }
}

} // namespace quentier

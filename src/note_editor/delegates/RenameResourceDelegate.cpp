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

#include "RenameResourceDelegate.h"
#include "../NoteEditor_p.h"
#include "../GenericResourceImageManager.h"
#include "../dialogs/RenameResourceDialog.h"
#include <quentier/logging/QuentierLogger.h>
#include <QScopedPointer>
#include <QBuffer>
#include <QImage>

namespace quentier {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page()); \
    if (Q_UNLIKELY(!page)) { \
        ErrorString error(QT_TR_NOOP("Can't rename the attachment: no note editor page")); \
        QNWARNING(error); \
        Q_EMIT notifyError(error); \
        return; \
    }

RenameResourceDelegate::RenameResourceDelegate(const Resource & resource, NoteEditorPrivate & noteEditor,
                                               GenericResourceImageManager * pGenericResourceImageManager,
                                               QHash<QByteArray, QString> & genericResourceImageFilePathsByResourceHash,
                                               const bool performingUndo) :
    m_noteEditor(noteEditor),
    m_pGenericResourceImageManager(pGenericResourceImageManager),
    m_genericResourceImageFilePathsByResourceHash(genericResourceImageFilePathsByResourceHash),
    m_resource(resource),
    m_oldResourceName(resource.displayName()),
    m_newResourceName(),
    m_shouldGetResourceNameFromDialog(true),
    m_performingUndo(performingUndo),
    m_pNote(noteEditor.notePtr())
#ifdef QUENTIER_USE_QT_WEB_ENGINE
    ,
    m_genericResourceImageWriterRequestId()
#endif
{}

void RenameResourceDelegate::start()
{
    QNDEBUG(QStringLiteral("RenameResourceDelegate::start"));

    if (m_noteEditor.isModified()) {
        QObject::connect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                         this, QNSLOT(RenameResourceDelegate,onOriginalPageConvertedToNote,Note));
        m_noteEditor.convertToNote();
    }
    else {
        doStart();
    }
}

void RenameResourceDelegate::startWithPresetNames(const QString & oldResourceName, const QString & newResourceName)
{
    QNDEBUG(QStringLiteral("RenameResourceDelegate::startWithPresetNames: old resource name = ") << oldResourceName
            << QStringLiteral(", new resource name = ") << newResourceName);

    m_oldResourceName = oldResourceName;
    m_newResourceName = newResourceName;
    m_shouldGetResourceNameFromDialog = false;

    start();
}

void RenameResourceDelegate::onOriginalPageConvertedToNote(Note note)
{
    QNDEBUG(QStringLiteral("RenameResourceDelegate::onOriginalPageConvertedToNote"));

    Q_UNUSED(note)

    QObject::disconnect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(RenameResourceDelegate,onOriginalPageConvertedToNote,Note));

    doStart();
}

#define CHECK_NOTE_ACTUALITY() \
    if (m_noteEditor.notePtr() != m_pNote) { \
        ErrorString error(QT_TR_NOOP("The note set to the note editor was changed during the attachment renaming, the action was not completed")); \
        QNDEBUG(error); \
        Q_EMIT notifyError(error); \
        return; \
    }

void RenameResourceDelegate::doStart()
{
    QNDEBUG(QStringLiteral("RenameResourceDelegate::doStart"));

    CHECK_NOTE_ACTUALITY()

    if (Q_UNLIKELY(!m_resource.hasDataHash())) {
        ErrorString error(QT_TR_NOOP("Can't rename the attachment: the data hash is missing"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    if (m_shouldGetResourceNameFromDialog)
    {
        raiseRenameResourceDialog();
    }
    else
    {
        m_resource.setDisplayName(m_newResourceName);

#ifdef QUENTIER_USE_QT_WEB_ENGINE
        buildAndSaveGenericResourceImage();
#else
        Q_EMIT finished(m_oldResourceName, m_newResourceName, m_resource, m_performingUndo);
#endif
    }
}

void RenameResourceDelegate::raiseRenameResourceDialog()
{
    QNDEBUG(QStringLiteral("RenameResourceDelegate::raiseRenameResourceDialog"));

    QScopedPointer<RenameResourceDialog> pRenameResourceDialog(new RenameResourceDialog(m_oldResourceName, &m_noteEditor));
    pRenameResourceDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(pRenameResourceDialog.data(), QNSIGNAL(RenameResourceDialog,accepted,QString),
                     this, QNSLOT(RenameResourceDelegate,onRenameResourceDialogFinished,QString));
    QNTRACE(QStringLiteral("Will exec rename resource dialog now"));
    int res = pRenameResourceDialog->exec();
    if (res == QDialog::Rejected) {
        QNTRACE(QStringLiteral("Cancelled renaming the resource"));
        Q_EMIT cancelled();
    }
}

void RenameResourceDelegate::onRenameResourceDialogFinished(QString newResourceName)
{
    QNDEBUG(QStringLiteral("RenameResourceDelegate::onRenameResourceDialogFinished: new resource name = ") << newResourceName);

    if (newResourceName.isEmpty()) {
        QNTRACE(QStringLiteral("New resource name is empty, treating it as cancellation"));
        Q_EMIT cancelled();
        return;
    }

    if (newResourceName == m_oldResourceName) {
        QNTRACE(QStringLiteral("The new resource name is equal to the old one, treating it as cancellation"));
        Q_EMIT cancelled();
        return;
    }

    m_newResourceName = newResourceName;
    m_resource.setDisplayName(m_newResourceName);
    m_noteEditor.replaceResourceInNote(m_resource);

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    buildAndSaveGenericResourceImage();
#else
    Q_EMIT finished(m_oldResourceName, m_newResourceName, m_resource, m_performingUndo);
#endif
}

#ifdef QUENTIER_USE_QT_WEB_ENGINE
void RenameResourceDelegate::buildAndSaveGenericResourceImage()
{
    QNDEBUG(QStringLiteral("RenameResourceDelegate::buildAndSaveGenericResourceImage"));

    CHECK_NOTE_ACTUALITY()

    QImage resourceImage = m_noteEditor.buildGenericResourceImage(m_resource);

    QByteArray imageData;
    QBuffer buffer(&imageData);
    Q_UNUSED(buffer.open(QIODevice::WriteOnly));
    resourceImage.save(&buffer, "PNG");

    m_genericResourceImageWriterRequestId = QUuid::createUuid();

    QNDEBUG(QStringLiteral("Emitting request to write generic resource image for resource with local uid ")
            << m_resource.localUid() << QStringLiteral(", request id ") << m_genericResourceImageWriterRequestId
            << QStringLiteral(", note local uid = ") << m_pNote->localUid());

    QObject::connect(this, QNSIGNAL(RenameResourceDelegate,saveGenericResourceImageToFile,QString,QString,QByteArray,QString,QByteArray,QString,QUuid),
                     m_pGenericResourceImageManager, QNSLOT(GenericResourceImageManager,onGenericResourceImageWriteRequest,QString,QString,QByteArray,QString,QByteArray,QString,QUuid));
    QObject::connect(m_pGenericResourceImageManager, QNSIGNAL(GenericResourceImageManager,genericResourceImageWriteReply,bool,QByteArray,QString,ErrorString,QUuid),
                     this, QNSLOT(RenameResourceDelegate,onGenericResourceImageWriterFinished,bool,QByteArray,QString,ErrorString,QUuid));

    Q_EMIT saveGenericResourceImageToFile(m_pNote->localUid(), m_resource.localUid(), imageData, QStringLiteral("png"),
                                          m_resource.dataHash(), m_resource.displayName(), m_genericResourceImageWriterRequestId);
}

void RenameResourceDelegate::onGenericResourceImageWriterFinished(bool success, QByteArray resourceHash, QString filePath,
                                                                  ErrorString errorDescription, QUuid requestId)
{
    if (requestId != m_genericResourceImageWriterRequestId) {
        return;
    }

    QNDEBUG(QStringLiteral("RenameResourceDelegate::onGenericResourceImageWriterFinished: success = ")
            << (success ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral(", resource hash = ")
            << resourceHash.toHex() << QStringLiteral(", file path = ") << filePath << QStringLiteral(", error description = ")
            << errorDescription << QStringLiteral(", request id = ") << requestId);

    QObject::disconnect(this, QNSIGNAL(RenameResourceDelegate,saveGenericResourceImageToFile,QString,QString,QByteArray,QString,QByteArray,QString,QUuid),
                        m_pGenericResourceImageManager, QNSLOT(GenericResourceImageManager,onGenericResourceImageWriteRequest,QString,QString,QByteArray,QString,QByteArray,QString,QUuid));
    QObject::disconnect(m_pGenericResourceImageManager, QNSIGNAL(GenericResourceImageManager,genericResourceImageWriteReply,bool,QByteArray,QString,QString,QUuid),
                        this, QNSLOT(RenameResourceDelegate,onGenericResourceImageWriterFinished,bool,QByteArray,QString,QString,QUuid));

    if (Q_UNLIKELY(!success)) {
        ErrorString error(QT_TR_NOOP("Can't rename generic resource: can't write generic resource image to file"));
        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    m_genericResourceImageFilePathsByResourceHash[resourceHash] = filePath;

    QString javascript = QStringLiteral("updateImageResourceSrc('") + QString::fromLocal8Bit(resourceHash.toHex()) +
                         QStringLiteral("', '") + filePath + QStringLiteral("');");

    GET_PAGE()
    page->executeJavaScript(javascript, JsCallback(*this, &RenameResourceDelegate::onGenericResourceImageUpdated));
}

void RenameResourceDelegate::onGenericResourceImageUpdated(const QVariant & data)
{
    QNDEBUG(QStringLiteral("RenameResourceDelegate::onGenericResourceImageUpdated"));

    Q_UNUSED(data)

    Q_EMIT finished(m_oldResourceName, m_newResourceName, m_resource, m_performingUndo);
}

#endif // QUENTIER_USE_QT_WEB_ENGINE

} // namespace quentier


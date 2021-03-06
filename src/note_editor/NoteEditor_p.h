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

#ifndef LIB_QUENTIER_NOTE_EDITOR_NOTE_EDITOR_P_H
#define LIB_QUENTIER_NOTE_EDITOR_NOTE_EDITOR_P_H

#include "ResourceInfo.h"
#include "NoteEditorPage.h"
#include <quentier/note_editor/INoteEditorBackend.h>
#include <quentier/note_editor/NoteEditor.h>
#include <quentier/enml/DecryptedTextManager.h>
#include <quentier/enml/ENMLConverter.h>
#include <quentier/utility/EncryptionManager.h>
#include <quentier/utility/StringUtils.h>
#include <quentier/types/ErrorString.h>
#include <quentier/types/ResourceRecognitionIndices.h>
#include <QObject>
#include <QMimeType>
#include <QFont>
#include <QColor>
#include <QImage>
#include <QUndoStack>
#include <QPointer>
#include <QScopedPointer>
#include <QMimeData>

#ifdef QUENTIER_USE_QT_WEB_ENGINE
#include <QWebEngineView>
typedef QWebEngineView WebView;
typedef QWebEnginePage WebPage;
#else
#include "NoteEditorPluginFactory.h"
#include <QWebView>
typedef QWebView WebView;
typedef QWebPage WebPage;
#endif

QT_FORWARD_DECLARE_CLASS(QByteArray)
QT_FORWARD_DECLARE_CLASS(QMimeType)
QT_FORWARD_DECLARE_CLASS(QImage)
QT_FORWARD_DECLARE_CLASS(QThread)

#ifdef QUENTIER_USE_QT_WEB_ENGINE
QT_FORWARD_DECLARE_CLASS(QWebChannel)
QT_FORWARD_DECLARE_CLASS(QWebSocketServer)
QT_FORWARD_DECLARE_CLASS(WebSocketClientWrapper)
#endif

namespace quentier {

QT_FORWARD_DECLARE_CLASS(ResourceInfoJavaScriptHandler)
QT_FORWARD_DECLARE_CLASS(ResourceFileStorageManager)
QT_FORWARD_DECLARE_CLASS(FileIOProcessorAsync)
QT_FORWARD_DECLARE_CLASS(TextCursorPositionJavaScriptHandler)
QT_FORWARD_DECLARE_CLASS(ContextMenuEventJavaScriptHandler)
QT_FORWARD_DECLARE_CLASS(TableResizeJavaScriptHandler)
QT_FORWARD_DECLARE_CLASS(PageMutationHandler)
QT_FORWARD_DECLARE_CLASS(ToDoCheckboxOnClickHandler)
QT_FORWARD_DECLARE_CLASS(ToDoCheckboxAutomaticInsertionHandler)
QT_FORWARD_DECLARE_CLASS(GenericResourceImageManager)
QT_FORWARD_DECLARE_CLASS(RenameResourceDelegate)
QT_FORWARD_DECLARE_CLASS(SpellChecker)
QT_FORWARD_DECLARE_CLASS(SpellCheckerDynamicHelper)
QT_FORWARD_DECLARE_CLASS(ResizableImageJavaScriptHandler)
QT_FORWARD_DECLARE_CLASS(ActionsWatcher)

#ifdef QUENTIER_USE_QT_WEB_ENGINE
QT_FORWARD_DECLARE_CLASS(EnCryptElementOnClickHandler)
QT_FORWARD_DECLARE_CLASS(GenericResourceOpenAndSaveButtonsOnClickHandler)
QT_FORWARD_DECLARE_CLASS(GenericResourceImageJavaScriptHandler)
QT_FORWARD_DECLARE_CLASS(HyperlinkClickJavaScriptHandler)
QT_FORWARD_DECLARE_CLASS(WebSocketWaiter)
#endif

class Q_DECL_HIDDEN NoteEditorPrivate: public WebView,
                                       public INoteEditorBackend
{
    Q_OBJECT
public:
    explicit NoteEditorPrivate(NoteEditor & noteEditor);
    virtual ~NoteEditorPrivate();

#ifndef QUENTIER_USE_QT_WEB_ENGINE
    QVariant execJavascriptCommandWithResult(const QString & command);
    QVariant execJavascriptCommandWithResult(const QString & command, const QString & args);
#endif

    void execJavascriptCommand(const QString & command);
    void execJavascriptCommand(const QString & command, const QString & args);

Q_SIGNALS:
    void contentChanged();
    void noteModified();
    void notifyError(ErrorString error);

    void inAppNoteLinkClicked(QString userId, QString shardId, QString noteGuid);
    void inAppNoteLinkPasteRequested(QString url, QString userId, QString shardId, QString noteGuid);

    void convertedToNote(Note note);
    void cantConvertToNote(ErrorString error);

    void noteEditorHtmlUpdated(QString html);

    void currentNoteChanged(Note note);

    void spellCheckerNotReady();
    void spellCheckerReady();

    void noteLoaded();

    // Signals to notify anyone interested of the formatting at the current cursor position
    void textBoldState(bool state);
    void textItalicState(bool state);
    void textUnderlineState(bool state);
    void textStrikethroughState(bool state);
    void textAlignLeftState(bool state);
    void textAlignCenterState(bool state);
    void textAlignRightState(bool state);
    void textAlignFullState(bool state);
    void textInsideOrderedListState(bool state);
    void textInsideUnorderedListState(bool state);
    void textInsideTableState(bool state);

    void textFontFamilyChanged(QString fontFamily);
    void textFontSizeChanged(int fontSize);

    void insertTableDialogRequested();

public:
    // Force the conversion from ENML to HTML
    void updateFromNote();

    // Resets the note's HTML to the given one
    void setNoteHtml(const QString & html);

    const Account * accountPtr() const;

    const Resource attachResourceToNote(const QByteArray & data, const QByteArray & dataHash,
                                        const QMimeType & mimeType, const QString & filename,
                                        const QString & sourceUrl = QString());
    void addResourceToNote(const Resource & resource);
    void removeResourceFromNote(const Resource & resource);
    void replaceResourceInNote(const Resource & resource);
    void setNoteResources(const QList<Resource> & resources);

    QImage buildGenericResourceImage(const Resource & resource);
    void saveGenericResourceImage(const Resource & resource, const QImage & image);
    void provideSrcForGenericResourceImages();
    void setupGenericResourceOnClickHandler();

    void updateResource(const QString & resourceLocalUid, const QByteArray & previousResourceHash,
                        Resource updatedResource);

    Note * notePtr() { return m_pNote.data(); }
    void setModified();

    bool isPageEditable() const { return m_isPageEditable; }

    QString noteEditorPagePath() const;
    const QString & imageResourcesStoragePath() const { return m_noteEditorImageResourcesStoragePath; }
    const QString & resourceLocalFileStoragePath() const { return m_resourceLocalFileStorageFolder; }
    const QString & genericResourceImageFileStoragePath() const { return m_genericResourceImageFileStoragePath; }

    void setRenameResourceDelegateSubscriptions(RenameResourceDelegate & delegate);

    void removeSymlinksToImageResourceFile(const QString & resourceLocalUid);
    QString createSymlinkToImageResourceFile(const QString & fileStoragePath, const QString & localUid,
                                             ErrorString & errorDescription);

    void onDropEvent(QDropEvent * pEvent);
    void dropFile(const QString & filepath);

    quint64 GetFreeEncryptedTextId() { return m_lastFreeEnCryptIdNumber++; }
    quint64 GetFreeDecryptedTextId() { return m_lastFreeEnDecryptedIdNumber++; }

    void refreshMisSpelledWordsList();
    void applySpellCheck(const bool applyToSelection = false);
    void removeSpellCheck();
    void enableDynamicSpellCheck();
    void disableDynamicSpellCheck();

    virtual bool isNoteLoaded() const Q_DECL_OVERRIDE;

    virtual QString selectedText() const Q_DECL_OVERRIDE;
    virtual bool hasSelection() const Q_DECL_OVERRIDE;

    bool searchHighlightEnabled() const;
    void setSearchHighlight(const QString & textToFind, const bool matchCase, const bool force = false) const;
    void highlightRecognizedImageAreas(const QString & textToFind, const bool matchCase) const;

    virtual bool spellCheckEnabled() const Q_DECL_OVERRIDE;

    virtual bool isModified() const Q_DECL_OVERRIDE;

    qint64 noteResourcesSize() const;
    qint64 noteContentSize() const;
    qint64 noteSize() const;

public Q_SLOTS:
    virtual void initialize(FileIOProcessorAsync & fileIOProcessorAsync,
                            SpellChecker & spellChecker,
                            const Account & account) Q_DECL_OVERRIDE;
    virtual QObject * object() Q_DECL_OVERRIDE { return this; }
    virtual QWidget * widget() Q_DECL_OVERRIDE { return this; }
    virtual void setAccount(const Account & account) Q_DECL_OVERRIDE;
    virtual void setUndoStack(QUndoStack * pUndoStack) Q_DECL_OVERRIDE;

    virtual void setBlankPageHtml(const QString & html) Q_DECL_OVERRIDE;

    virtual void undo() Q_DECL_OVERRIDE;
    virtual void redo() Q_DECL_OVERRIDE;
    virtual void cut() Q_DECL_OVERRIDE;
    virtual void copy() Q_DECL_OVERRIDE;
    virtual void paste() Q_DECL_OVERRIDE;
    virtual void pasteUnformatted() Q_DECL_OVERRIDE;
    virtual void selectAll() Q_DECL_OVERRIDE;
    virtual void formatSelectionAsSourceCode() Q_DECL_OVERRIDE;
    virtual void fontMenu() Q_DECL_OVERRIDE;
    virtual void textBold() Q_DECL_OVERRIDE;
    virtual void textItalic() Q_DECL_OVERRIDE;
    virtual void textUnderline() Q_DECL_OVERRIDE;
    virtual void textStrikethrough() Q_DECL_OVERRIDE;
    virtual void textHighlight() Q_DECL_OVERRIDE;
    virtual void alignLeft() Q_DECL_OVERRIDE;
    virtual void alignCenter() Q_DECL_OVERRIDE;
    virtual void alignRight() Q_DECL_OVERRIDE;
    virtual void alignFull() Q_DECL_OVERRIDE;

    virtual void findNext(const QString & text, const bool matchCase) const Q_DECL_OVERRIDE;
    virtual void findPrevious(const QString & text, const bool matchCase) const Q_DECL_OVERRIDE;

    virtual void replace(const QString & textToReplace, const QString & replacementText, const bool matchCase) Q_DECL_OVERRIDE;
    virtual void replaceAll(const QString & textToReplace, const QString & replacementText, const bool matchCase) Q_DECL_OVERRIDE;

    void onReplaceJavaScriptDone(const QVariant & data);

    virtual void insertToDoCheckbox() Q_DECL_OVERRIDE;
    virtual void insertInAppNoteLink(const QString & userId, const QString & shardId,
                                     const QString & noteGuid, const QString & linkText) Q_DECL_OVERRIDE;
    virtual void setSpellcheck(const bool enabled) Q_DECL_OVERRIDE;
    virtual void setFont(const QFont & font) Q_DECL_OVERRIDE;
    virtual void setFontHeight(const int height) Q_DECL_OVERRIDE;
    virtual void setFontColor(const QColor & color) Q_DECL_OVERRIDE;
    virtual void setBackgroundColor(const QColor & color) Q_DECL_OVERRIDE;
    virtual void insertHorizontalLine() Q_DECL_OVERRIDE;
    virtual void increaseFontSize() Q_DECL_OVERRIDE;
    virtual void decreaseFontSize() Q_DECL_OVERRIDE;
    virtual void increaseIndentation() Q_DECL_OVERRIDE;
    virtual void decreaseIndentation() Q_DECL_OVERRIDE;
    virtual void insertBulletedList() Q_DECL_OVERRIDE;
    virtual void insertNumberedList() Q_DECL_OVERRIDE;
    virtual void insertTableDialog() Q_DECL_OVERRIDE;
    virtual void insertFixedWidthTable(const int rows, const int columns, const int widthInPixels) Q_DECL_OVERRIDE;
    virtual void insertRelativeWidthTable(const int rows, const int columns, const double relativeWidth) Q_DECL_OVERRIDE;
    virtual void insertTableRow() Q_DECL_OVERRIDE;
    virtual void insertTableColumn() Q_DECL_OVERRIDE;
    virtual void removeTableRow() Q_DECL_OVERRIDE;
    virtual void removeTableColumn() Q_DECL_OVERRIDE;
    virtual void addAttachmentDialog() Q_DECL_OVERRIDE;
    virtual void saveAttachmentDialog(const QByteArray & resourceHash) Q_DECL_OVERRIDE;
    virtual void saveAttachmentUnderCursor() Q_DECL_OVERRIDE;
    virtual void openAttachment(const QByteArray & resourceHash) Q_DECL_OVERRIDE;
    virtual void openAttachmentUnderCursor() Q_DECL_OVERRIDE;
    virtual void copyAttachment(const QByteArray & resourceHash) Q_DECL_OVERRIDE;
    virtual void copyAttachmentUnderCursor() Q_DECL_OVERRIDE;
    virtual void removeAttachment(const QByteArray & resourceHash) Q_DECL_OVERRIDE;
    virtual void removeAttachmentUnderCursor() Q_DECL_OVERRIDE;
    virtual void renameAttachment(const QByteArray & resourceHash) Q_DECL_OVERRIDE;
    virtual void renameAttachmentUnderCursor() Q_DECL_OVERRIDE;
    virtual void rotateImageAttachment(const QByteArray & resourceHash, const Rotation::type rotationDirection) Q_DECL_OVERRIDE;
    virtual void rotateImageAttachmentUnderCursor(const Rotation::type rotationDirection) Q_DECL_OVERRIDE;

    void rotateImageAttachmentUnderCursorClockwise();
    void rotateImageAttachmentUnderCursorCounterclockwise();

    virtual void encryptSelectedText() Q_DECL_OVERRIDE;

    virtual void decryptEncryptedTextUnderCursor() Q_DECL_OVERRIDE;
    virtual void decryptEncryptedText(QString encryptedText, QString cipher, QString keyLength, QString hint, QString enCryptIndex) Q_DECL_OVERRIDE;

    virtual void hideDecryptedTextUnderCursor() Q_DECL_OVERRIDE;
    virtual void hideDecryptedText(QString encryptedText, QString decryptedText, QString cipher, QString keyLength, QString hint, QString enDecryptedIndex) Q_DECL_OVERRIDE;

    virtual void editHyperlinkDialog() Q_DECL_OVERRIDE;
    virtual void copyHyperlink() Q_DECL_OVERRIDE;
    virtual void removeHyperlink() Q_DECL_OVERRIDE;

    virtual void onNoteLoadCancelled() Q_DECL_OVERRIDE;

    virtual bool print(QPrinter & printer, ErrorString & errorDescription) Q_DECL_OVERRIDE;
    virtual bool exportToPdf(const QString & absoluteFilePath, ErrorString & errorDescription) Q_DECL_OVERRIDE;
    virtual bool exportToEnex(const QStringList & tagNames,
                              QString & enex, ErrorString & errorDescription) Q_DECL_OVERRIDE;

    virtual void setNoteAndNotebook(const Note & note, const Notebook & notebook) Q_DECL_OVERRIDE;
    virtual void clear() Q_DECL_OVERRIDE;
    virtual void setFocusToEditor() Q_DECL_OVERRIDE;
    virtual void convertToNote() Q_DECL_OVERRIDE;

    void undoPageAction();
    void redoPageAction();

    void flipEnToDoCheckboxState(const quint64 enToDoIdNumber);

// private signals:
Q_SIGNALS:
    void saveResourceToStorage(QString noteLocalUid, QString resourceLocalUid, QByteArray data, QByteArray dataHash,
                               QString preferredFileSuffix, QUuid requestId, bool isImage);
    void readResourceFromStorage(QString fileStoragePath, QString localUid, QUuid requestId);
    void openResourceFile(QString absoluteFilePath);
    void writeNoteHtmlToFile(QString absoluteFilePath, QByteArray html, QUuid requestId, bool append);
    void saveResourceToFile(QString absoluteFilePath, QByteArray resourceData, QUuid requestId, bool append);
    void saveGenericResourceImageToFile(QString noteLocalUid, QString resourceLocalUid, QByteArray resourceImageData,
                                        QString resourceFileSuffix, QByteArray resourceActualHash,
                                        QString resourceDisplayName, QUuid requestId);

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    void htmlReadyForPrinting();
#endif

private Q_SLOTS:
    void onTableResized();

    void onFoundSelectedHyperlinkId(const QVariant & hyperlinkData,
                                    const QVector<QPair<QString, QString> > & extraData);
    void onFoundHyperlinkToCopy(const QVariant & hyperlinkData,
                                const QVector<QPair<QString, QString> > & extraData);

    void onNoteLoadFinished(bool ok);
    void onContentChanged();

    void onResourceSavedToStorage(QUuid requestId, QByteArray dataHash, QString fileStoragePath,
                                  int errorCode, ErrorString errorDescription);
    void onResourceFileChanged(QString resourceLocalUid, QString fileStoragePath);
    void onResourceFileReadFromStorage(QUuid requestId, QByteArray data, QByteArray dataHash,
                                       int errorCode, ErrorString errorDescription);

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    void onGenericResourceImageSaved(bool success, QByteArray resourceActualHash,
                                     QString filePath, ErrorString errorDescription,
                                     QUuid requestId);

    void onHyperlinkClicked(QString url);

    void onWebSocketReady();
#else
    void onHyperlinkClicked(QUrl url);
#endif

    void onToDoCheckboxClicked(quint64 enToDoCheckboxId);
    void onToDoCheckboxClickHandlerError(ErrorString error);

    void onToDoCheckboxInserted(const QVariant & data, const QVector<QPair<QString,QString> > & extraData);
    void onToDoCheckboxAutomaticInsertion();
    void onToDoCheckboxAutomaticInsertionUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData);

    void onJavaScriptLoaded();

    void onOpenResourceRequest(const QByteArray & resourceHash);
    void onSaveResourceRequest(const QByteArray & resourceHash);

    void contextMenuEvent(QContextMenuEvent * pEvent) Q_DECL_OVERRIDE;
    void onContextMenuEventReply(QString contentType, QString selectedHtml, bool insideDecryptedTextFragment,
                                 QStringList extraData, quint64 sequenceNumber);

    void onTextCursorPositionChange();

    void onTextCursorBoldStateChanged(bool state);
    void onTextCursorItalicStateChanged(bool state);
    void onTextCursorUnderlineStateChanged(bool state);
    void onTextCursorStrikethgouthStateChanged(bool state);
    void onTextCursorAlignLeftStateChanged(bool state);
    void onTextCursorAlignCenterStateChanged(bool state);
    void onTextCursorAlignRightStateChanged(bool state);
    void onTextCursorAlignFullStateChanged(bool state);
    void onTextCursorInsideOrderedListStateChanged(bool state);
    void onTextCursorInsideUnorderedListStateChanged(bool state);
    void onTextCursorInsideTableStateChanged(bool state);

    void onTextCursorOnImageResourceStateChanged(bool state, QByteArray resourceHash);
    void onTextCursorOnNonImageResourceStateChanged(bool state, QByteArray resourceHash);
    void onTextCursorOnEnCryptTagStateChanged(bool state, QString encryptedText, QString cipher, QString length);

    void onTextCursorFontNameChanged(QString fontName);
    void onTextCursorFontSizeChanged(int fontSize);

    void onWriteFileRequestProcessed(bool success, ErrorString errorDescription, QUuid requestId);

    void onSpellCheckCorrectionAction();
    void onSpellCheckIgnoreWordAction();
    void onSpellCheckAddWordToUserDictionaryAction();

    void onSpellCheckCorrectionActionDone(const QVariant & data, const QVector<QPair<QString,QString> > & extraData);
    void onSpellCheckCorrectionUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData);
    void onSpellCheckerDynamicHelperUpdate(QStringList words);

    void onSpellCheckerReady();

    void onImageResourceResized(bool pushUndoCommand);

    void onSelectionFormattedAsSourceCode(const QVariant & response, const QVector<QPair<QString,QString> > & extraData);

    // Slots for delegates
    void onAddResourceDelegateFinished(Resource addedResource, QString resourceFileStoragePath);
    void onAddResourceDelegateError(ErrorString error);
    void onAddResourceUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData);

    void onRemoveResourceDelegateFinished(Resource removedResource);
    void onRemoveResourceDelegateError(ErrorString error);
    void onRemoveResourceUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData);

    void onRenameResourceDelegateFinished(QString oldResourceName, QString newResourceName,
                                          Resource resource, bool performingUndo);
    void onRenameResourceDelegateCancelled();
    void onRenameResourceDelegateError(ErrorString error);

    void onImageResourceRotationDelegateFinished(QByteArray resourceDataBefore, QByteArray resourceHashBefore,
                                                 QByteArray resourceRecognitionDataBefore, QByteArray resourceRecognitionDataHashBefore,
                                                 Resource resourceAfter, INoteEditorBackend::Rotation::type rotationDirection);
    void onImageResourceRotationDelegateError(ErrorString error);

    void onHideDecryptedTextFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData);
    void onHideDecryptedTextUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData);

    void onEncryptSelectedTextDelegateFinished();
    void onEncryptSelectedTextDelegateCancelled();
    void onEncryptSelectedTextDelegateError(ErrorString error);
    void onEncryptSelectedTextUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData);

    void onDecryptEncryptedTextDelegateFinished(QString encryptedText, QString cipher, size_t length, QString hint,
                                                QString decryptedText, QString passphrase, bool rememberForSession,
                                                bool decryptPermanently);
    void onDecryptEncryptedTextDelegateCancelled();
    void onDecryptEncryptedTextDelegateError(ErrorString error);
    void onDecryptEncryptedTextUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData);

    void onAddHyperlinkToSelectedTextDelegateFinished();
    void onAddHyperlinkToSelectedTextDelegateCancelled();
    void onAddHyperlinkToSelectedTextDelegateError(ErrorString error);
    void onAddHyperlinkToSelectedTextUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData);

    void onEditHyperlinkDelegateFinished();
    void onEditHyperlinkDelegateCancelled();
    void onEditHyperlinkDelegateError(ErrorString error);
    void onEditHyperlinkUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData);

    void onRemoveHyperlinkDelegateFinished();
    void onRemoveHyperlinkDelegateError(ErrorString error);
    void onRemoveHyperlinkUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData);

    void onInsertHtmlDelegateFinished(QList<Resource> addedResources, QStringList resourceFileStoragePaths);
    void onInsertHtmlDelegateError(ErrorString error);
    void onInsertHtmlUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData);

    void onSourceCodeFormatUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData);

    // Slots for undo command signals
    void onUndoCommandError(ErrorString error);

    void onSpellCheckerDictionaryEnabledOrDisabled(bool checked);

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    void getHtmlForPrinting();
#endif

private:
    void init();

    void handleHyperlinkClicked(const QUrl & url);
    void handleInAppLinkClicked(const QString & urlString);
    bool parseInAppLink(const QString & urlString, QString & userId,
                        QString & shardId, QString & noteGuid, ErrorString & errorDescription) const;

    bool checkNoteSize(const QString & newNoteContent);

    void pushNoteContentEditUndoCommand();
    void pushTableActionUndoCommand(const QString & name, NoteEditorPage::Callback callback);

    template <typename T>
    QString composeHtmlTable(const T width, const T singleColumnWidth, const int rows,
                             const int columns, const bool relative);

    void onManagedPageActionFinished(const QVariant & result, const QVector<QPair<QString, QString> > & extraData);
    void updateJavaScriptBindings();

    void changeFontSize(const bool increase);
    void changeIndentation(const bool increase);

    void findText(const QString & textToFind, const bool matchCase, const bool searchBackward = false,
                  NoteEditorPage::Callback = 0) const;

    void clearEditorContent();
    void noteToEditorContent();
    void updateColResizableTableBindings();
    void inkNoteToEditorContent();

    bool htmlToNoteContent(ErrorString & errorDescription);

    void saveNoteResourcesToLocalFiles();
    bool saveResourceToLocalFile(const Resource & resource);
    void updateHashForResourceTag(const QByteArray & oldResourceHash, const QByteArray & newResourceHash);
    void provideSrcForResourceImgTags();

    void manualSaveResourceToFile(const Resource & resource);
    void openResource(const QString & resourceAbsoluteFilePath);

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    void provideSrcAndOnClickScriptForImgEnCryptTags();

    void setupGenericResourceImages();

    // Returns true if the resource image gets built and is being saved to a file asynchronously
    bool findOrBuildGenericResourceImage(const Resource & resource);

    void setupWebSocketServer();
    void setupJavaScriptObjects();
    void setupTextCursorPositionTracking();
#endif

    void setupGenericTextContextMenu(const QStringList & extraData, const QString & selectedHtml, bool insideDecryptedTextFragment);
    void setupImageResourceContextMenu(const QByteArray & resourceHash);
    void setupNonImageResourceContextMenu(const QByteArray & resourceHash);
    void setupEncryptedTextContextMenu(const QString & cipher, const QString & keyLength,
                                       const QString & encryptedText, const QString & hint,
                                       const QString & id);

    void setupActionShortcut(const int key, const QString & context, QAction & action);

    void setupFileIO();
    void setupSpellChecker();
    void setupScripts();
    void setupGeneralSignalSlotConnections();
    void setupNoteEditorPage();
    void setupNoteEditorPageConnections(NoteEditorPage * page);
    void setupTextCursorPositionJavaScriptHandlerConnections();
    void setupSkipRulesForHtmlToEnmlConversion();

    QString initialPageHtml() const;

    void determineStatesForCurrentTextCursorPosition();
    void determineContextMenuEventTarget();

    void setPageEditable(const bool editable);

    bool checkContextMenuSequenceNumber(const quint64 sequenceNumber) const;

    void onPageHtmlReceived(const QString & html, const QVector<QPair<QString,QString> > & extraData = QVector<QPair<QString,QString> >());
    void onSelectedTextEncryptionDone(const QVariant & dummy, const QVector<QPair<QString,QString> > & extraData);

    void onTableActionDone(const QVariant & dummy, const QVector<QPair<QString,QString> > & extraData);

    int resourceIndexByHash(const QList<Resource> & resources, const QByteArray & resourceHash) const;

    void writeNotePageFile(const QString & html);

    bool parseEncryptedTextContextMenuExtraData(const QStringList & extraData, QString & encryptedText,
                                                QString & decryptedText, QString & cipher, QString & keyLength, QString & hint,
                                                QString & id, ErrorString & errorDescription) const;
    void setupPasteGenericTextMenuActions();
    void setupParagraphSubMenuForGenericTextMenu(const QString & selectedHtml);
    void setupStyleSubMenuForGenericTextMenu();
    void setupSpellCheckerDictionariesSubMenuForGenericTextMenu();

    void rebuildRecognitionIndicesCache();

    void enableSpellCheck();
    void disableSpellCheck();

    void onSpellCheckSetOrCleared(const QVariant & dummy, const QVector<QPair<QString,QString> > & extraData);

    bool isNoteReadOnly() const;

    void setupAddHyperlinkDelegate(const quint64 hyperlinkId, const QString & presetHyperlink = QString(),
                                   const QString & replacementLinkText = QString());

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    void onPageHtmlReceivedForPrinting(const QString & html,
                                       const QVector<QPair<QString,QString> > & extraData = QVector<QPair<QString,QString> >());
#endif

private:
    // Overrides for some Qt's virtual methods
    virtual void timerEvent(QTimerEvent * pEvent) Q_DECL_OVERRIDE;
    virtual void dragMoveEvent(QDragMoveEvent * pEvent) Q_DECL_OVERRIDE;
    virtual void dropEvent(QDropEvent * pEvent) Q_DECL_OVERRIDE;

    void pasteImageData(const QMimeData & mimeData);

    void escapeStringForJavaScript(QString & str) const;

private:
    template <class T>
    class Q_DECL_HIDDEN NoteEditorCallbackFunctor
    {
    public:
        NoteEditorCallbackFunctor(NoteEditorPrivate * pNoteEditor,
                                  void (NoteEditorPrivate::* method)(const T & result,
                                                                     const QVector<QPair<QString,QString> > & extraData),
                                  const QVector<QPair<QString,QString> > & extraData = QVector<QPair<QString,QString> >()) :
            m_pNoteEditor(pNoteEditor), m_method(method), m_extraData(extraData)
        {}

        NoteEditorCallbackFunctor(const NoteEditorCallbackFunctor<T> & other) : m_pNoteEditor(other.m_pNoteEditor), m_method(other.m_method), m_extraData(other.m_extraData) {}
        NoteEditorCallbackFunctor & operator=(const NoteEditorCallbackFunctor<T> & other)
        { if (this != &other) { m_pNoteEditor = other.m_pNoteEditor; m_method = other.m_method; m_extraData = other.m_extraData; } return *this; }

        void operator()(const T & result) { if (Q_LIKELY(!m_pNoteEditor.isNull())) { (m_pNoteEditor->*m_method)(result, m_extraData); } }

    private:
        QPointer<NoteEditorPrivate> m_pNoteEditor;
        void (NoteEditorPrivate::* m_method)(const T & result, const QVector<QPair<QString,QString> > & extraData);
        QVector<QPair<QString, QString> > m_extraData;
    };

    friend class NoteEditorCallbackFunctor<QString>;
    friend class NoteEditorCallbackFunctor<QVariant>;

    class Q_DECL_HIDDEN ReplaceCallback
    {
    public:
        ReplaceCallback(NoteEditorPrivate * pNoteEditor) : m_pNoteEditor(pNoteEditor) {}

        void operator()(const QVariant & data) { if (Q_UNLIKELY(m_pNoteEditor.isNull())) { return; } m_pNoteEditor->onReplaceJavaScriptDone(data); }

    private:
        QPointer<NoteEditorPrivate>     m_pNoteEditor;
    };

    struct Q_DECL_HIDDEN Alignment
    {
        enum type {
            Left = 0,
            Center,
            Right,
            Full
        };
    };

    struct Q_DECL_HIDDEN TextFormattingState
    {
        TextFormattingState() :
            m_bold(false),
            m_italic(false),
            m_underline(false),
            m_strikethrough(false),
            m_alignment(Alignment::Left),
            m_insideOrderedList(false),
            m_insideUnorderedList(false),
            m_insideTable(false),
            m_onImageResource(false),
            m_onNonImageResource(false),
            m_resourceHash(),
            m_onEnCryptTag(false),
            m_encryptedText(),
            m_cipher(),
            m_length()
        {}

        bool m_bold;
        bool m_italic;
        bool m_underline;
        bool m_strikethrough;

        Alignment::type m_alignment;

        bool m_insideOrderedList;
        bool m_insideUnorderedList;
        bool m_insideTable;

        bool m_onImageResource;
        bool m_onNonImageResource;
        QString m_resourceHash;

        bool m_onEnCryptTag;
        QString m_encryptedText;
        QString m_cipher;
        QString m_length;
    };

    // Holds some data required for certain context menu actions, like the encrypted text data for its decryption,
    // the hash of the resource under cursor for which the action is toggled etc.
    class Q_DECL_HIDDEN CurrentContextMenuExtraData
    {
    public:
        CurrentContextMenuExtraData() :
            m_contentType(),
            m_encryptedText(),
            m_decryptedText(),
            m_keyLength(),
            m_cipher(),
            m_hint(),
            m_insideDecryptedText(false),
            m_id(),
            m_resourceHash()
        {}

        QString     m_contentType;

        // Encrypted text extra data
        QString     m_encryptedText;
        QString     m_decryptedText;
        QString     m_keyLength;
        QString     m_cipher;
        QString     m_hint;
        bool        m_insideDecryptedText;
        QString     m_id;

        // Resource extra data
        QByteArray  m_resourceHash;
    };

private:
    QString     m_noteEditorPageFolderPath;
    QString     m_noteEditorImageResourcesStoragePath;
    QString     m_genericResourceImageFileStoragePath;

    QFont       m_font;

    // JavaScript scripts
    QString     m_jQueryJs;
    QString     m_jQueryUiJs;
    QString     m_resizableTableColumnsJs;
    QString     m_resizableImageManagerJs;
    QString     m_debounceJs;
    QString     m_rangyCoreJs;
    QString     m_rangySelectionSaveRestoreJs;
    QString     m_onTableResizeJs;
    QString     m_nodeUndoRedoManagerJs;
    QString     m_selectionManagerJs;
    QString     m_textEditingUndoRedoManagerJs;
    QString     m_getSelectionHtmlJs;
    QString     m_snapSelectionToWordJs;
    QString     m_replaceSelectionWithHtmlJs;
    QString     m_replaceHyperlinkContentJs;
    QString     m_updateResourceHashJs;
    QString     m_updateImageResourceSrcJs;
    QString     m_provideSrcForResourceImgTagsJs;
    QString     m_setupEnToDoTagsJs;
    QString     m_flipEnToDoCheckboxStateJs;
    QString     m_onResourceInfoReceivedJs;
    QString     m_findInnermostElementJs;
    QString     m_determineStatesForCurrentTextCursorPositionJs;
    QString     m_determineContextMenuEventTargetJs;
    QString     m_changeFontSizeForSelectionJs;
    QString     m_pageMutationObserverJs;
    QString     m_tableManagerJs;
    QString     m_resourceManagerJs;
    QString     m_htmlInsertionManagerJs;
    QString     m_sourceCodeFormatterJs;
    QString     m_hyperlinkManagerJs;
    QString     m_encryptDecryptManagerJs;
    QString     m_hilitorJs;
    QString     m_imageAreasHilitorJs;
    QString     m_findReplaceManagerJs;
    QString     m_spellCheckerJs;
    QString     m_managedPageActionJs;
    QString     m_setInitialCaretPositionJs;
    QString     m_toDoCheckboxAutomaticInsertionJs;
    QString     m_disablePasteJs;
    QString     m_findAndReplaceDOMTextJs;

#ifndef QUENTIER_USE_QT_WEB_ENGINE
    QString     m_qWebKitSetupJs;
#else
    QString     m_provideSrcForGenericResourceImagesJs;
    QString     m_onGenericResourceImageReceivedJs;
    QString     m_provideSrcAndOnClickScriptForEnCryptImgTagsJs;
    QString     m_qWebChannelJs;
    QString     m_qWebChannelSetupJs;
    QString     m_notifyTextCursorPositionChangedJs;
    QString     m_setupTextCursorPositionTrackingJs;
    QString     m_genericResourceOnClickHandlerJs;
    QString     m_setupGenericResourceOnClickHandlerJs;
    QString     m_clickInterceptorJs;

    QWebSocketServer * m_pWebSocketServer;
    WebSocketClientWrapper * m_pWebSocketClientWrapper;
    QWebChannel * m_pWebChannel;
    EnCryptElementOnClickHandler * m_pEnCryptElementClickHandler;
    GenericResourceOpenAndSaveButtonsOnClickHandler * m_pGenericResourceOpenAndSaveButtonsOnClickHandler;
    HyperlinkClickJavaScriptHandler * m_pHyperlinkClickJavaScriptHandler;
    WebSocketWaiter * m_pWebSocketWaiter;
    bool        m_webSocketReady;

    quint16     m_webSocketServerPort;
#endif

    SpellCheckerDynamicHelper *             m_pSpellCheckerDynamicHandler;
    TableResizeJavaScriptHandler *          m_pTableResizeJavaScriptHandler;
    ResizableImageJavaScriptHandler *       m_pResizableImageJavaScriptHandler;
    GenericResourceImageManager *           m_pGenericResourceImageManager;
    ToDoCheckboxOnClickHandler *            m_pToDoCheckboxClickHandler;
    ToDoCheckboxAutomaticInsertionHandler * m_pToDoCheckboxAutomaticInsertionHandler;
    PageMutationHandler *                   m_pPageMutationHandler;
    ActionsWatcher *                        m_pActionsWatcher;

    QUndoStack * m_pUndoStack;

    QScopedPointer<Account>     m_pAccount;

    QString     m_htmlForPrinting;

    QString     m_blankPageHtml;

    quint64     m_contextMenuSequenceNumber;
    QPoint      m_lastContextMenuEventGlobalPos;
    QPoint      m_lastContextMenuEventPagePos;
    ContextMenuEventJavaScriptHandler * m_pContextMenuEventJavaScriptHandler;

    TextCursorPositionJavaScriptHandler * m_pTextCursorPositionJavaScriptHandler;

    TextFormattingState     m_currentTextFormattingState;

    QUuid       m_writeNoteHtmlToFileRequestId;

    bool        m_isPageEditable;
    bool        m_pendingConversionToNote;
    bool        m_pendingNotePageLoad;

    /**
     * The two following variables deserve a special explanation. Since Qt 5.9 QWebEnginePage::load method started to behave
     * really weirdly: it seems when it's called for the first time, the method blocks the event loop until the page is actually loaded.
     * I.e. when the page got loaded, the execution of code after the call to QWebEnginePage::load (or QWebEnginePage::setUrl
     * since it calls QWebEnginePage::load internally) continues.
     *
     * Why to give a damn, you ask? Well, things become more interesting if you attempt to call QWebEnginePage::load
     * (or QWebEnginePage::setUrl) while there's still an event loop blocked inside QWebEnginePage::load. In particular,
     * what seems to happen is that the second call to QWebEnginePage::load does not block; the page seems to be loaded
     * successfully but then the original blocked call to QWebEnginePage::load returns. The net effect is the appearance
     * of the first loaded URL within the page, not the second one.
     *
     * This behaviour has only been observed with Qt 5.9, not with any prior version. It is (of course) not documented
     * or mentioned anywhere, you have to learn this on your own, the hard way. Thank you, Qt devs, you are the best... not.
     *
     * Working around this issue using the special boolean flag indicating whether the method is currently blocked
     * in at least one event loop. If yes, won't attempt to call QWebEnginePage::load (or QWebEnginePage::setUrl)
     * until the blocked method returns, instead will just save the next URL to load and will load it later
     */
    bool        m_pendingNotePageLoadMethodExit;
    QUrl        m_pendingNextPageUrl;

    bool        m_pendingIndexHtmlWritingToFile;
    bool        m_pendingJavaScriptExecution;

    bool        m_skipPushingUndoCommandOnNextContentChange;

    QScopedPointer<Note>        m_pNote;
    QScopedPointer<Notebook>    m_pNotebook;

    bool        m_modified;

    // These two bools implement a cheap scheme of watching
    // for changes in note editor since some particular moment in time.
    // For example, the conversion of note from HTML into ENML happens
    // in the background mode, when the editor is idle for at least N seconds.
    // How can such idle state be determined? Create a timer for N seconds,
    // as it begins, set m_watchingForContentChange to true and
    // m_contentChangedSinceWatchingStart to false. On every next content change
    // m_contentChangedSinceWatchingStart would be set to true. When the timer ends,
    // it can check the state of m_contentChangedSinceWatchingStart.
    // If it's true, it means the editing is still in progress and it's not nice
    // to block the GUI thread by HTML to ENML conversion. So drop this
    // variable into false again and wait for another N seconds. And only
    // if there were no further edits during N seconds, convert note editor's page
    // to ENML
    bool        m_watchingForContentChange;
    bool        m_contentChangedSinceWatchingStart;

    int         m_secondsToWaitBeforeConversionStart;

    int         m_pageToNoteContentPostponeTimerId;

    QSharedPointer<EncryptionManager>       m_encryptionManager;
    QSharedPointer<DecryptedTextManager>    m_decryptedTextManager;

    ENMLConverter                           m_enmlConverter;

#ifndef QUENTIER_USE_QT_WEB_ENGINE
    NoteEditorPluginFactory *               m_pPluginFactory;
#endif

    QMenu *             m_pGenericTextContextMenu;
    QMenu *             m_pImageResourceContextMenu;
    QMenu *             m_pNonImageResourceContextMenu;
    QMenu *             m_pEncryptedTextContextMenu;

    SpellChecker *      m_pSpellChecker;
    bool                m_spellCheckerEnabled;
    QStringList         m_currentNoteMisSpelledWords;
    StringUtils         m_stringUtils;

    const QString       m_pagePrefix;

    QString     m_lastSelectedHtml;
    QString     m_lastSelectedHtmlForEncryption;
    QString     m_lastSelectedHtmlForHyperlink;

    QString     m_lastMisSpelledWord;

    mutable QString   m_lastSearchHighlightedText;
    mutable bool      m_lastSearchHighlightedTextCaseSensitivity;

    QString     m_enmlCachedMemory;   // Cached memory for HTML to ENML conversions
    QString     m_htmlCachedMemory;   // Cached memory for ENML from Note -> HTML conversions
    QString     m_errorCachedMemory;  // Cached memory for various errors

    QVector<ENMLConverter::SkipHtmlElementRule>     m_skipRulesForHtmlToEnmlConversion;

    ResourceFileStorageManager *    m_pResourceFileStorageManager;
    FileIOProcessorAsync *          m_pFileIOProcessorAsync;

    ResourceInfo                    m_resourceInfo;
    ResourceInfoJavaScriptHandler * m_pResourceInfoJavaScriptHandler;

    QString                         m_resourceLocalFileStorageFolder;

    QHash<QUuid, QString>           m_genericResourceLocalUidBySaveToStorageRequestIds;
    QSet<QUuid>                     m_imageResourceSaveToStorageRequestIds;

    QHash<QString, QString>         m_resourceFileStoragePathsByResourceLocalUid;

    QSet<QString>                   m_localUidsOfResourcesWantedToBeSaved;
    QSet<QString>                   m_localUidsOfResourcesWantedToBeOpened;

    QSet<QUuid>                     m_manualSaveResourceToFileRequestIds;

    QHash<QString, QStringList>     m_fileSuffixesForMimeType;
    QHash<QString, QString>         m_fileFilterStringForMimeType;

    QHash<QByteArray, QString>      m_genericResourceImageFilePathsByResourceHash;

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    GenericResourceImageJavaScriptHandler *     m_pGenericResoureImageJavaScriptHandler;
#endif

    QSet<QUuid>                                 m_saveGenericResourceImageToFileRequestIds;

    QHash<QByteArray, ResourceRecognitionIndices>   m_recognitionIndicesByResourceHash;

    CurrentContextMenuExtraData                 m_currentContextMenuExtraData;

    QHash<QUuid, QPair<QString, QString> >      m_resourceLocalUidAndFileStoragePathByReadResourceRequestIds;

    quint64     m_lastFreeEnToDoIdNumber;
    quint64     m_lastFreeHyperlinkIdNumber;
    quint64     m_lastFreeEnCryptIdNumber;
    quint64     m_lastFreeEnDecryptedIdNumber;

    NoteEditor * const q_ptr;
    Q_DECLARE_PUBLIC(NoteEditor)
};

} // namespace quentier

void initNoteEditorResources();

#endif // LIB_QUENTIER_NOTE_EDITOR_NOTE_EDITOR_P_H

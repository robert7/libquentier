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

#ifndef LIB_QUENTIER_SYNCHRONIZATION_SYNCHRONIZATION_MANAGER_H
#define LIB_QUENTIER_SYNCHRONIZATION_SYNCHRONIZATION_MANAGER_H

#include <quentier/synchronization/IAuthenticationManager.h>
#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>
#include <quentier/types/LinkedNotebook.h>
#include <quentier/utility/Linkage.h>
#include <quentier/utility/Macros.h>
#include <QObject>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerAsync)
QT_FORWARD_DECLARE_CLASS(SynchronizationManagerPrivate)

/**
 * @brief The SynchronizationManager class encapsulates methods and signals & slots required to perform the full or partial
 * synchronization of data with remote Evernote servers. The class also deals with authentication with Evernote service through
 * OAuth.
 */
class QUENTIER_EXPORT SynchronizationManager: public QObject
{
    Q_OBJECT
public:
    /**
     * @param consumerKey - consumer key for your application obtained from the Evernote service
     * @param consumerSecret - consumer secret for your application obtained from the Evernote service
     * @param host - the host to use for the connection with the Evernote service - typically www.evernote.com
     *               but could be sandbox.evernote.com or some other one
     * @param localStorageManagerAsync - local storage manager
     * @param authenticationManager - authentication manager (particular implementation of IAuthenticationManager abstract class)
     */
    SynchronizationManager(const QString & consumerKey, const QString & consumerSecret,
                           const QString & host, LocalStorageManagerAsync & localStorageManagerAsync,
                           IAuthenticationManager & authenticationManager);

    virtual ~SynchronizationManager();

    /**
     * @return true if the synchronization is being performed at the moment, false otherwise
     */
    bool active() const;

    /**
     * @return true or false depending on the option to download the thumbnails for notes containing resources
     * during sync; by default no thumbnails are downloaded
     */
    bool downloadNoteThumbnailsOption() const;

public Q_SLOTS:
    /**
     * Use this slot to set the current account for the synchronization manager. If the slot is called during the synchronization running,
     * it would stop, any internal caches belonging to previously selected account (if any) would be purged (but persistent settings
     * like the authentication token saved in the system keychain would remain). Setting the current account won't automatically start
     * the synchronization for it, use @link synchronize @endling slot for this.
     *
     * The attempt to set the current account of "Local" type would just clean up the synchronization manager as if it was just created
     *
     * After the method finishes its job, setAccountDone signal is emitted
     */
    void setAccount(Account account);

    /**
     * Use this slot to authenticate the new user to do the synchronization with the Evernote service via the client app.
     * The invoking of slot would respond asynchronously with @link authenticationFinished @endlink signal but won't start
     * the synchronization.
     *
     * Note that this slot would always proceed to the actual OAuth.
     */
    void authenticate();

    /**
     * Use this slot to launch the synchronization of data
     */
    void synchronize();

    /**
     * Use this slot to stop the running synchronization; if no synchronization is running by the moment of this slot call,
     * it has no effect
     */
    void stop();

    /**
     * Use this slot to remove any previously cached authentication tokens (and shard ids) for a given user ID. After the call
     * of this method the next attempt to synchronize the data for this user ID would cause the launch of OAuth to get the new
     * authentication token
     */
    void revokeAuthentication(const qevercloud::UserID userId);

    /**
     * Use this slot to switch the option whether the synchronization of notes would download the note thumbnails or not.
     * By default the downloading of thumbnails for notes containing resources is disabled.
     *
     * NOTE: even if thumbnails downloading is enabled, the thumbnails would be downloaded during sync only for notes
     * containing resources
     *
     * After the method finishes its job, setDownloadNoteThumbnailsDone signal is emitted
     */
    void setDownloadNoteThumbnails(bool flag);

    /**
     * Use this slot to switch the option whether the synchronization of notes would download the plain images corresponding
     * to ink notes or not. By default the downloading of ink note images is disabled.
     *
     * After the method finishes its job, setDownloadInkNoteImagesDone signal is emitted
     */
    void setDownloadInkNoteImages(bool flag);

    /**
     * Use this slot to specify the path to folder at which the downloaded ink note images should be stored. Each
     * ink note image would be stored in a separate PNG file which name would be the same as the guid of the corresponding
     * resource and the file extension would be PNG
     *
     * The default storage path would be the folder "inkNoteImages" within the folder returned by
     * applicationPersistentStoragePath function found in quentier/StandardPaths.h header
     *
     * WARNING: if the passed in path cannot be used (either it doesn't exist and cannot be created or exists but is not writable),
     * the default path is silently restored. So make sure you're setting a valid path
     *
     * After the method finishes its job, setInkNoteImagesStoragePathDone signal is emitted
     */
    void setInkNoteImagesStoragePath(QString path);

Q_SIGNALS:
    /**
     * This signal is emitted when the synchronization is started (authentication is not considered a part of
     * synchronization so this signal is only emitted when the authentication is completed)
     */
    void started();

    /**
     * This signal is emitted in response to invoking the stop slot, whether it was invoked manually or from within
     * the SynchronizationManager itself (due to sync failure, for example)
     */
    void stopped();

    /**
     * This signal is emitted when the synchronization fails; at this moment there is no error code explaining the reason
     * of the failure programmatically so the only explanation available is the textual one for the end user
     */
    void failed(ErrorString errorDescription);

    /**
     * This signal is emitted when the synchronization is finished
     * @param account - represents the latest version of @link Account @endlink structure filled during the synchronization procedure
     */
    void finished(Account account);

    /**
     * This signal is emitted in response to the attempt to revoke the authentication for a given user ID
     * @param success - true if the authentication was revoked successfully, false otherwise
     * @param errorDescription - the textual explanation of the failure to revoke the authentication
     * @param userId - the ID of the user for which the revoke of the authentication was requested
     */
    void authenticationRevoked(bool success, ErrorString errorDescription,
                               qevercloud::UserID userId);

    /**
     * This signal is emitted in response to the attempt to authenticate the new user of the client app to synchronize
     * with the Evernote service
     * @param success - true if the authentication was successful, false otherwise
     * @param errorDescription - the textual explanation of the failure to authenticate the new user
     * @param account - the account of the authenticated user
     */
    void authenticationFinished(bool success, ErrorString errorDescription,
                                Account account);

    /**
     * This signal is emitted when the "remote to local" synchronization step is stopped
     */
    void remoteToLocalSyncStopped();

    /**
     * This signal is emitted when the "send local changes" synchronization step is stopped
     */
    void sendLocalChangesStopped();

    /**
     * This signal is emitted if during the "send local changes" synchronization step it was found out that new changes
     * from the Evernote service are available yet no conflict between remote and local changes was found yet.
     *
     * Such situation can rarely happen in case of changes introduced concurrently with the
     * running synchronization - perhaps via another client. The algorithm will handle it,
     * the signal is just for the sake of diagnostic
     */
    void willRepeatRemoteToLocalSyncAfterSendingChanges();

    /**
     * This signal is emitted if during the "send local changes" synchronization step it was found out that new changes
     * from the Evernote service are available AND some of them conflict with the local changes being sent.
     *
     * Such situation can rarely happen in case of changes introduced concurrently with the
     * running synchronization - perhaps via another client. The algorithm will handle it by repeating the "remote to local"
     * incremental synchronization step, the signal is just for the sake of diagnostic
     */
    void detectedConflictDuringLocalChangesSending();

    /**
     * This signal is emitted when the Evernote API rate limit is breached during the synchronization; the algorithm
     * will handle it by auto-pausing itself until the time necessary to wait passes and then automatically continuing
     * the synchronization.
     * @param secondsToWait - the amount of time (in seconds) necessary to wait before the synchronization will continue
     */
    void rateLimitExceeded(qint32 secondsToWait);

    /**
     * This signal is emitted when the "remote to local" synchronization step is finished; once that step is done,
     * the algorithn switches to sending the local changes back to the Evernote service
     */
    void remoteToLocalSyncDone();

    /**
     * This signal is emitted during user own account's sync chunks downloading and denotes the progress of that step. The percentage
     * of completeness can be computed roughly as
     * (highestDownloadedUsn - lastPreviousUsn) / (highestServerUsn - lastPreviousUsn) * 100%
     *
     * @param highestDownloadedUsn denotes the highest update sequence number within data items from sync chunks
     * downloaded so far
     * @param highestServerUsn denotes the current highest update sequence number within the account
     * @param lastPreviousUsn denotes the last update sequence number from previous sync; if current sync is the first one,
     * this value is zero
     */
    void syncChunksDownloadProgress(qint32 highestDownloadedUsn, qint32 highestServerUsn, qint32 lastPreviousUsn);

    /**
     * This signal is emitted when the sync chunks for the stuff from user's own account are downloaded
     * during "remote to local" synchronization step
     */
    void syncChunksDownloaded();

    /**
     * This signal is emitted during linked notebooks sync chunks downloading and denotes the progress of that step,
     * individually for each linked notebook. The percentage of completeness can be computed roughly as
     * (highestDownloadedUsn - lastPreviousUsn) / (highestServerUsn - lastPreviousUsn) * 100%.
     * The sync chunks for each linked notebook are downloaded sequentially so the signals for one linked notebook
     * should not intermix with signals for other linked notebooks, however, it is within hands of Qt's slot schedulers
     *
     * @param highestDownloadedUsn denotes the highest update sequence number within data items from linked notebook
     * sync chunks downloaded so far
     * @param highestServerUsn denotes the current highest update sequence number within the linked notebook
     * @param lastPreviousUsn denotes the last update sequence number from previous sync of the given linked notebook;
     * if current sync is the first one, this value is zero
     * @param linkedNotebook denotes the linked notebook which sync chunks download progress is reported
     */
    void linkedNotebookSyncChunksDownloadProgress(qint32 highestDownloadedUsn, qint32 highestServerUsn, qint32 lastPreviousUsn,
                                                  LinkedNotebook linkedNotebook);

    /**
     * This signal is emitted when the sync chunks for the stuff from linked notebooks are downloaded
     * during "remote to local" synchronization step
     */
    void linkedNotebooksSyncChunksDownloaded();

    /**
     * This signal is emitted on each successful download of full note data from user's own account
     * @param notesDownloaded is the number of notes downloaded by the moment
     * @param totalNotesToDownload is the total number of notes that need to be downloaded
     */
    void notesDownloadProgress(quint32 notesDownloaded, quint32 totalNotesToDownload);

    /**
     * This signal is emitted on each successful download of full note data from linked notebooks
     * @param notesDownloaded is the number of notes downloaded by the moment
     * @param totalNotesToDownload is the total number of notes that need to be downloaded
     */
    void linkedNotebooksNotesDownloadProgress(quint32 notesDownloaded, quint32 totalNotesToDownload);

    /**
     * This signal is emitted on each successful doenload of full resource data
     * from user's own account during the incremental sync (as individual resources are
     * downloaded along with their notes during full sync)
     * @param resourcesDownloaded is the number of resources downloaded by the moment
     * @param totalResourcesToDownload is the total number of resources that
     * need to be downloaded
     */
    void resourcesDownloadProgress(quint32 resourcesDownloaded, quint32 totalResourcesToDownload);

    /**
     * This signal is emitted on each successful download of full resource data
     * from linked notebooks during the incremental sync (as individual resources are
     * downloaded along with their notes during full sync)
     * @param resourcesDownloaded is the number of resources downloaded by the moment
     * @param totalResourcesToDownload is the total number of resources that
     * need to be downloaded
     */
    void linkedNotebooksResourcesDownloadProgress(quint32 resourcesDownloaded, quint32 totalResourcesToDownload);

    /**
     * This signal is emitted during "send local changes" synchronization step when all the relevant
     * data elements from user's own account were prepared for sending to the Evernote service
     */
    void preparedDirtyObjectsForSending();

    /**
     * This signal is emitted during "send local changes" synchronization step when all the relevant
     * data elements from linked notebooks were prepared for sending to the Evernote service
     */
    void preparedLinkedNotebooksDirtyObjectsForSending();

    /**
     * This signal is emitted in response to invoking the setAccount slot after all the activities involved
     * in switching the account inside SynchronizationManager are finished
     */
    void setAccountDone(Account account);

    /**
     * This signal is emitted in response to invoking the setDownloadNoteThumbnails slot after the setting is accepted
     */
    void setDownloadNoteThumbnailsDone(bool flag);

    /**
     * This signal is emitted in response to invoking the setDownloadInkNoteImages slot after the setting is accepted
     */
    void setDownloadInkNoteImagesDone(bool flag);

    /**
     * This signal is emitted in response to invoking the setInkNoteImagesStoragePath slot after the setting is accepted
     */
    void setInkNoteImagesStoragePathDone(QString path);

private:
    SynchronizationManager() Q_DECL_EQ_DELETE;
    Q_DISABLE_COPY(SynchronizationManager)

    SynchronizationManagerPrivate * d_ptr;
    Q_DECLARE_PRIVATE(SynchronizationManager)
};

} // namespace quentier

#endif // LIB_QUENTIER_SYNCHRONIZATION_SYNCHRONIZATION_MANAGER_H

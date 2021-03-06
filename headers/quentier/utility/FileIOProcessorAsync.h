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

#ifndef LIB_QUENTIER_UTILITY_FILE_IO_PROCESSOR_ASYNC_H
#define LIB_QUENTIER_UTILITY_FILE_IO_PROCESSOR_ASYNC_H

#include <quentier/utility/Linkage.h>
#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>
#include <QObject>
#include <QString>
#include <QUuid>
#include <QByteArray>
#include <QIODevice>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(FileIOProcessorAsyncPrivate)

/**
 * @brief The FileIOProcessorAsync class is a wrapper under simple file IO operations,
 * it is meant to be used for simple asynchronous IO
 */
class QUENTIER_EXPORT FileIOProcessorAsync: public QObject
{
    Q_OBJECT
public:
    explicit FileIOProcessorAsync(QObject * parent = Q_NULLPTR);

    /**
     * @brief setIdleTimePeriod - set time period defining the idle state of FileIOProcessorAsync:
     * once the time measured since the last IO operation is over the specified number of seconds,
     * the class emits readyForIO signal to any interested listeners of this event. If this method
     * is not called ever, the default idle time period would be 30 seconds.
     * @param seconds - number of seconds for idle time period
     */
    void setIdleTimePeriod(qint32 seconds);

Q_SIGNALS:
    /**
     * @brief readyForIO - signal sent when the queue for file IO is empty for some time
     * (30 seconds by default, can also be configured via setIdleTimePeriod method)
     * after the last IO event to indicate to signal listeners that they can perform some IO
     * via the FileIOProcessorAsync thread
     */
    void readyForIO();

    /**
     * @brief writeFileRequestProcessed - signal sent when the file write request with given id is finished
     * @param success - true if write operation was successful, false otherwise
     * @param errorDescription - textual description of the error
     * @param requestId - unique identifier of the file write request
     */
    void writeFileRequestProcessed(bool success, ErrorString errorDescription, QUuid requestId);

    /**
     * @brief readFileRequestProcessed - signal send when the file read request with given id is finished
     * @param success - true if read operation was successful, false otherwise
     * @param errorDescription - textual description of the error
     * @param data - data read from file
     * @param requestId - unique identifier of the file read request
     */
    void readFileRequestProcessed(bool success, ErrorString errorDescription, QByteArray data,
                                  QUuid requestId);

public Q_SLOTS:
    /**
     * @brief onWriteFileRequest - slot processing the file write request with given request id
     * @param absoluteFilePath - absolute file path to be written
     * @param data - data to be written to the file
     * @param requestId - unique identifier of the file write request
     * @param append - if true, the data would be appended to file,
     * otherwise the entire file would be erased before with the data is written
     */
    void onWriteFileRequest(QString absoluteFilePath, QByteArray data,
                            QUuid requestId, bool append);

    /**
     * @brief onReadFileRequest - slot processing the file read request with given request id
     * @param absoluteFilePath - absolute file path to be read
     * @param requestId - unique identifier of the file read request
     */
    void onReadFileRequest(QString absoluteFilePath,
                           QUuid requestId);

private:
    FileIOProcessorAsyncPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(FileIOProcessorAsync)
};

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_FILE_IO_PROCESSOR_ASYNC_H

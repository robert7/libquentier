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

#include <quentier/utility/FileIOProcessorAsync.h>
#include <quentier/utility/Macros.h>
#include "FileIOProcessorAsync_p.h"

namespace quentier {

FileIOProcessorAsync::FileIOProcessorAsync(QObject * parent) :
    QObject(parent),
    d_ptr(new FileIOProcessorAsyncPrivate(this))
{
    QObject::connect(d_ptr, QNSIGNAL(FileIOProcessorAsyncPrivate,readyForIO),
                     this, QNSIGNAL(FileIOProcessorAsync,readyForIO));
    QObject::connect(d_ptr, QNSIGNAL(FileIOProcessorAsyncPrivate,writeFileRequestProcessed,bool,ErrorString,QUuid),
                     this, QNSIGNAL(FileIOProcessorAsync,writeFileRequestProcessed,bool,ErrorString,QUuid));
    QObject::connect(d_ptr, QNSIGNAL(FileIOProcessorAsyncPrivate,readFileRequestProcessed,bool,ErrorString,QByteArray,QUuid),
                     this, QNSIGNAL(FileIOProcessorAsync,readFileRequestProcessed,bool,ErrorString,QByteArray,QUuid));
}

void FileIOProcessorAsync::setIdleTimePeriod(qint32 seconds)
{
    Q_D(FileIOProcessorAsync);
    d->setIdleTimePeriod(seconds);
}

void FileIOProcessorAsync::onWriteFileRequest(QString absoluteFilePath, QByteArray data, QUuid requestId, bool append)
{
    Q_D(FileIOProcessorAsync);
    d->onWriteFileRequest(absoluteFilePath, data, requestId, append);
}

void FileIOProcessorAsync::onReadFileRequest(QString absoluteFilePath, QUuid requestId)
{
    Q_D(FileIOProcessorAsync);
    d->onReadFileRequest(absoluteFilePath, requestId);
}

} // namespace quentier

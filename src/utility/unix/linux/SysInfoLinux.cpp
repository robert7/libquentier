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

#include <quentier/utility/SysInfo.h>
#include "../../SysInfo_p.h"
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <QString>
#include <QMutexLocker>

namespace quentier {

qint64 SysInfo::totalMemory()
{
    Q_D(SysInfo);
    QMutexLocker mutexLocker(&d->m_mutex);

    struct sysinfo si;
    int rc = sysinfo(&si);
    if (rc) {
        return -1;
    }

    return static_cast<qint64>(si.totalram);
}

qint64 SysInfo::freeMemory()
{
    Q_D(SysInfo);
    QMutexLocker mutexLocker(&d->m_mutex);

    struct sysinfo si;
    int rc = sysinfo(&si);
    if (rc) {
        return -1;
    }

    return static_cast<qint64>(si.freeram);
}

QString SysInfo::platformName()
{
    utsname info;
    int res = uname(&info);
    if (Q_UNLIKELY(res != 0)) {
        return QStringLiteral("Unknown Unix/Linux");
    }

    QString result = QString::fromUtf8(info.sysname);
    result += QStringLiteral("/");
    result += QString::fromUtf8(info.release);
    result += QStringLiteral(" ");
    result += QString::fromUtf8(info.version);
    return result;
}

} // namespace quentier

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

#ifndef LIB_QUENTIER_TYPES_I_NOTE_STORE_DATA_ELEMENT_H
#define LIB_QUENTIER_TYPES_I_NOTE_STORE_DATA_ELEMENT_H

#include "ILocalStorageDataElement.h"
#include <quentier/utility/Printable.h>
#include <quentier/types/ErrorString.h>
#include <QtGlobal>
#include <QUuid>

namespace quentier {

class QUENTIER_EXPORT INoteStoreDataElement: public ILocalStorageDataElement,
                                             public Printable
{
public:
    virtual void clear() = 0;

    virtual bool hasGuid() const = 0;
    virtual const QString & guid() const = 0;
    virtual void setGuid(const QString & guid) = 0;

    virtual bool hasUpdateSequenceNumber() const = 0;
    virtual qint32 updateSequenceNumber() const = 0;
    virtual void setUpdateSequenceNumber(const qint32 usn) = 0;

    virtual bool checkParameters(ErrorString & errorDescription) const = 0;

    virtual bool isDirty() const = 0;
    virtual void setDirty(const bool dirty) = 0;

    virtual bool isLocal() const = 0;
    virtual void setLocal(const bool local) = 0;

    virtual ~INoteStoreDataElement() {}
};

#define DECLARE_IS_DIRTY \
    virtual bool isDirty() const Q_DECL_OVERRIDE;

#define DECLARE_SET_DIRTY \
    virtual void setDirty(const bool isDirty) Q_DECL_OVERRIDE;

#define QN_DECLARE_DIRTY \
    DECLARE_IS_DIRTY \
    DECLARE_SET_DIRTY

#define DEFINE_IS_DIRTY(type) \
    bool type::isDirty() const { \
        return d->m_isDirty; \
    }

#define DEFINE_SET_DIRTY(type) \
    void type::setDirty(const bool dirty) { \
        d->m_isDirty = dirty; \
    }

#define QN_DEFINE_DIRTY(type) \
    DEFINE_IS_DIRTY(type) \
    DEFINE_SET_DIRTY(type)

#define DECLARE_IS_LOCAL \
    virtual bool isLocal() const Q_DECL_OVERRIDE;

#define DECLARE_SET_LOCAL \
    virtual void setLocal(const bool isLocal) Q_DECL_OVERRIDE;

#define QN_DECLARE_LOCAL \
    DECLARE_IS_LOCAL \
    DECLARE_SET_LOCAL

#define DEFINE_IS_LOCAL(type) \
    bool type::isLocal() const { \
        return d->m_isLocal; \
    }

#define DEFINE_SET_LOCAL(type) \
    void type::setLocal(const bool local) { \
        d->m_isLocal = local; \
    }

#define QN_DEFINE_LOCAL(type) \
    DEFINE_IS_LOCAL(type) \
    DEFINE_SET_LOCAL(type)

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_I_NOTE_STORE_DATA_ELEMENT_H

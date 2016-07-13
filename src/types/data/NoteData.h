#ifndef LIB_QUENTIER_TYPES_DATA_NOTE_DATA_H
#define LIB_QUENTIER_TYPES_DATA_NOTE_DATA_H

#include "FavoritableDataElementData.h"
#include <quentier/utility/QNLocalizedString.h>
#include <QEverCloud.h>
#include <QImage>

namespace quentier {

class NoteData: public FavoritableDataElementData
{
public:
    NoteData();
    NoteData(const NoteData & other);
    NoteData(NoteData && other);
    NoteData(const qevercloud::Note & other);
    virtual ~NoteData();

    void clear();
    bool checkParameters(QNLocalizedString & errorDescription) const;

    QString plainText(QNLocalizedString * pErrorMessage) const;
    QStringList listOfWords(QNLocalizedString * pErrorMessage) const;
    std::pair<QString, QStringList> plainTextAndListOfWords(QNLocalizedString * pErrorMessage) const;

    bool containsToDoImpl(const bool checked) const;
    bool containsEncryption() const;

    void setContent(const QString & content);

    qevercloud::Note m_qecNote;

    struct ResourceAdditionalInfo
    {
        QString  localUid;
        bool     isDirty;

        bool operator==(const ResourceAdditionalInfo & other) const;
    };

    QList<ResourceAdditionalInfo>   m_resourcesAdditionalInfo;
    qevercloud::Optional<QString>   m_notebookLocalUid;
    QStringList                     m_tagLocalUids;
    QImage                          m_thumbnail;

private:
    NoteData & operator=(const NoteData & other) Q_DECL_DELETE;
    NoteData & operator=(NoteData && other) Q_DECL_DELETE;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_DATA_NOTE_DATA_H

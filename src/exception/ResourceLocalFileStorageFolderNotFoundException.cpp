#include <quentier/exception/ResourceLocalFileStorageFolderNotFoundException.h>

namespace quentier {

ResourceLocalFileStorageFolderNotFoundException::ResourceLocalFileStorageFolderNotFoundException(const QNLocalizedString & message) :
    IQuentierException(message)
{}

const QString ResourceLocalFileStorageFolderNotFoundException::exceptionDisplayName() const
{
    return "ResourceLocalFileStorageFolderNotFoundException";
}

} // namespace quentier

// temporary "copy" from kdepimlibs/akonadi/conflicthandling

#ifndef CONFLICTHANDLER_H
#define CONFLICTHANDLER_H

#include <QObject>

class KJob;

namespace Akonadi {
    class DifferencesAlgorithmInterface;
    class Item;
}

typedef ulong WId;

class ConflictHandler : public QObject
{
    Q_OBJECT

public:
    /**
     * Describes the type of conflict that should be resolved by
     * the conflict handler.
     */
    enum ConflictType
    {
      LocalLocalConflict,   ///< Changes of two Akonadi client applications conflict.
      LocalRemoteConflict,  ///< Changes of an Akonadi client application and a resource conflict.
      BackendConflict       ///< Changes of a resource and the backend data conflict.
    };

    /**
     * Describes the strategy that should be used for resolving the conflict.
     */
    enum ResolveStrategy
    {
      UseLocalItem, ///< The local item overwrites the other item inside the Akonadi storage.
      UseOtherItem, ///< The local item is dropped and the other item from the Akonadi storage is used.
      UseBothItems  ///< Both items are kept in the Akonadi storage.
    };

    /**
     * Creates a new conflict handler.
     *
     * @param type The type of the conflict that should be resolved.
     * @param parent The parent object.
     */
    explicit ConflictHandler( ConflictType type, QObject *parent = 0 );

    ~ConflictHandler();

    /**
     * Sets the items that causes the conflict.
     *
     * @param changedItem The item that has been changed, it needs the complete payload set.
     * @param conflictingItem The item from the Akonadi storage that is conflicting.
     *                        This needs only the id set, the payload will be refetched automatically.
     */
    void setConflictingItems( const Akonadi::Item &changedItem, const Akonadi::Item &conflictingItem );

    void setDifferencesInterface( Akonadi::DifferencesAlgorithmInterface *interface );

    void setParentWindowId( WId windowId );

  public Q_SLOTS:
    /**
     * Starts the conflict handling.
     */
    void start();

  Q_SIGNALS:
    /**
     * This signal is emitted whenever the conflict has been resolved
     * automatically or by the user.
     */
    void conflictResolved();

    /**
     * This signal is emitted whenever an error occured during the conflict
     * handling.
     *
     * @param message A user visible string that describes the error.
     */
    void error( const QString &message );

    void commitChange( const Akonadi::Item &item );

    void updateOnBackend( const Akonadi::Item &item );

private:
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT( d, void duplicateLocalItemResult( KJob* ) )
};

#endif

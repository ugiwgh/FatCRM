#ifndef ACCOUNTSPAGE_H
#define ACCOUNTSPAGE_H

#include "page.h"

#include <akonadi/item.h>


class AccountsPage : public Page
{
    Q_OBJECT
public:
    explicit AccountsPage( QWidget *parent = 0 );

    ~AccountsPage();

protected:
    /*reimp*/ void addItem( QMap<QString, QString> data );
    /*reimp*/ void modifyItem( Akonadi::Item &item, QMap<QString, QString> data );
    /*reimp*/ void setupModel();

};
#endif /* ACCOUNTSPAGE_H */


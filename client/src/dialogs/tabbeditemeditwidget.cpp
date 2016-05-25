/*
  This file is part of FatCRM, a desktop application for SugarCRM written by KDAB.

  Copyright (C) 2016 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Authors: Michel Boyer de la Giroday <michel.giroday@kdab.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "tabbeditemeditwidget.h"
#include "ui_tabbeditemeditwidget.h"

#include "associateddatawidget.h"
#include "itemstreemodel.h"
#include "modelrepository.h"
#include "simpleitemeditwidget.h"
#include "sugaraccount.h"
#include "sugaropportunity.h"

#include <KContacts/Addressee>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QStringListModel>
#include <QVBoxLayout>

// kdepimlibs4 compat
#define KABC KContacts

using namespace Akonadi;


TabbedItemEditWidget::TabbedItemEditWidget(SimpleItemEditWidget *ItemEditWidget, DetailsType details, QWidget *parent) :
    ItemEditWidgetBase(parent),
    mType(details),
    mUi(new Ui::TabbedItemEditWidget)
{
    setWindowFlags(Qt::Window);
    mUi->setupUi(this);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel, Qt::Horizontal, this);
    mSaveButton = buttonBox->button(QDialogButtonBox::Save);
    mSaveButton->setEnabled(false);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    layout()->addWidget(buttonBox);

    ItemEditWidget->hideButtonBox();
    mItemEditWidget = ItemEditWidget;
    connect(mItemEditWidget, SIGNAL(dataModified()), this, SLOT(dataChanged()));
    connect(mItemEditWidget, SIGNAL(itemSaved()), this, SLOT(close()));

    if (mType == Account) {
        mUi->tabWidget->insertTab(0, ItemEditWidget, i18n("Account details"));
        mUi->tabWidget->setTabText(1, i18n("Opportunities and Contacts"));
        SugarAccount account = ItemEditWidget->item().payload<SugarAccount>();
        loadAssociatedData(account.id(), Opportunity);
    } else if (mType == Opportunity) {
        mUi->tabWidget->insertTab(0, ItemEditWidget, i18n("Opportunity details"));
        mUi->tabWidget->setTabText(1, i18n("Contacts"));
        SugarOpportunity opportunity = ItemEditWidget->item().payload<SugarOpportunity>();
        loadAssociatedData(opportunity.accountId(), Contact);
    }

    mUi->tabWidget->setCurrentIndex(0);
    setWindowTitle(mItemEditWidget->title());
    initialize();
}

TabbedItemEditWidget::~TabbedItemEditWidget()
{
    delete mUi;
}

Akonadi::Item TabbedItemEditWidget::item() const
{
    return mItemEditWidget->item();
}

bool TabbedItemEditWidget::isModified() const
{
    return mItemEditWidget->isModified();
}

QString TabbedItemEditWidget::title() const
{
    return mItemEditWidget->title();
}
QString TabbedItemEditWidget::detailsName() const
{
    return mItemEditWidget->detailsName();
}

void TabbedItemEditWidget::initialize()
{
    if (mType == Opportunity) {
        mUi->associatedDataTab->hideOpportunityGui();
    }
    connect (mUi->associatedDataTab, SIGNAL(openItem(QString)), this, SLOT(openWidget(QString)));
}

void TabbedItemEditWidget::loadAssociatedData(const QString &accountId, DetailsType type)
{
    ItemsTreeModel *model = ModelRepository::instance()->model(type);
    const int count = model->rowCount();
    QStringListModel *listModel = new QStringListModel(this);
    QStringList list;
    for (int i = 0; i < count; ++i) {
        const QModelIndex index = model->index(i, 0);
        const Item item = model->data(index, EntityTreeModel::ItemRole).value<Item>();
        if (type == Opportunity) {
            // still need to handle contacts for each opportunity here (Probably a bad choice of model and view)
            const SugarOpportunity opportunity = item.payload<SugarOpportunity>();
            if (opportunity.accountId()  ==  accountId) {
                list << opportunity.name();
                QPair<Akonadi::Item, DetailsType> data = qMakePair(item, Opportunity);
                mItemsMap.insert(opportunity.name(), data);
                loadAssociatedData(opportunity.accountId(), Contact);
            }
        } else {
            const KABC::Addressee addressee = item.payload<KABC::Addressee>();
            if (addressee.custom("FATCRM", "X-AccountId")  == accountId) {
                list << addressee.assembledName();
                mItemsMap.insert(addressee.assembledName(), qMakePair(item, Contact));
            }
        }
    }

    listModel->setStringList(list);

    bool haveOpportunities = false;
    bool haveContacts = false;
    if (type == Opportunity) {
        haveOpportunities = (listModel->rowCount() != 0);
        mUi->associatedDataTab->setOpportunitiesModel(listModel);
    } else {
        haveContacts = (listModel->rowCount() != 0);
        mUi->associatedDataTab->setContactsModel(listModel);
    }
    // disable associated data tab if no data
    if (!haveOpportunities && !haveContacts)
        mUi->tabWidget->setTabEnabled(1, false);
}

void TabbedItemEditWidget::openWidget(const QString &itemKey)
{
    emit openWidgetForItem(mItemsMap.value(itemKey).first, mItemsMap.value(itemKey).second);
}

void TabbedItemEditWidget::dataChanged()
{
    mSaveButton->setEnabled(true);
}

void TabbedItemEditWidget::accept()
{
    mItemEditWidget->accept();
    //We need to wait until the item is saved before closing - see connection to itemSaved signal above
}

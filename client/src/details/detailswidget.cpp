#include "detailswidget.h"

#include "accountdetails.h"
#include "campaigndetails.h"
#include "contactdetails.h"
#include "leaddetails.h"
#include "opportunitydetails.h"
#include "referenceddatamodel.h"
#include "sugarclient.h"

#include "kdcrmdata/kdcrmutils.h"

#include <akonadi/item.h>

using namespace Akonadi;

DetailsWidget::DetailsWidget(DetailsType type, QWidget *parent)
    : QWidget(parent),
      mType(type),
      mModified(false),
      mCreateNew(false)
{
    mUi.setupUi(this);
    initialize();
}

DetailsWidget::~DetailsWidget()
{

}

/*
 * Create a detail widget of mType and layout it
 * (called by constructor)
 */
void DetailsWidget::initialize()
{
    if (mUi.detailsContainer->layout()) {
        delete mUi.detailsContainer->layout();
    }

    mDetails = createDetailsForType(mType);

    setConnections();

    connect(mUi.saveButton, SIGNAL(clicked()),
            this, SLOT(saveData()));
    connect(mUi.discardButton, SIGNAL(clicked()),
            this, SLOT(slotDiscardData()));

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addWidget(mDetails);
    mUi.detailsContainer->setLayout(hlayout);

}

/*
 * Enable the save button when changes happen
 *
 */
void DetailsWidget::setConnections()
{
    connect(mDetails, SIGNAL(modified()),
            this, SLOT(slotModified()));
    connect(mUi.description, SIGNAL(textChanged()),
            this,  SLOT(slotModified()));
    mUi.saveButton->setEnabled(false);
    mUi.discardButton->setEnabled(false);
    mModified = false;
}

/*
 * Disconnect changes signal. e.g. when filling the details
 * widgets with existing data, selection change etc ....
 *
 */
void DetailsWidget::reset()
{
    disconnect(mDetails, SIGNAL(modified()),
               this, SLOT(slotModified()));
    disconnect(mUi.description, SIGNAL(textChanged()),
               this,  SLOT(slotModified()));
}

/*
 * Reset all the widgets and properties, to let the user create a new entry.
 */
void DetailsWidget::clearFields()
{
    mDetails->clear();

    mUi.dateModified->clear();

    // initialize other fields
    mUi.description->setPlainText(QString());

    // hide creation/modification date/user
    mUi.createdModifiedContainer->hide();

    // we are creating a new entry
    mCreateNew = true;

    setConnections();
}

/*
 * Retrieves the @p item data, before calling setData.
 */
void DetailsWidget::setItem(const Item &item)
{
    mItem = item;

    // new item selected reset flag and saving
    mCreateNew = false;
    reset();
    setData(mDetails->data(item));
    setConnections();
    mUi.createdModifiedContainer->show();
}

/*
 * Fill in the widgets with their data and properties.
 * It covers all the item types.
 */
void DetailsWidget::setData(const QMap<QString, QString> &data)
{
    mDetails->setData(data, mUi.createdModifiedContainer);
    // Transform the time returned by the server to system time
    // before it is displayed.
    const QString localTime = KDCRMUtils::formatTimestamp(data.value("dateModified"));
    mUi.dateModified->setText(localTime);

    mUi.description->setPlainText((mType != Campaign) ?
                                  data.value("description") :
                                  data.value("content"));
}

/*
 * Return a map containing the data (and properties) from all the widgets
 *
 */
QMap<QString, QString> DetailsWidget::data() const
{
    QMap<QString, QString> currentData = mDetails->getData();

    currentData["description"] = mUi.description->toPlainText();
    currentData["content"] = mUi.description->toPlainText();

    const QString accountId = currentAccountId();
    currentData["parentId"] = accountId;
    currentData["accountId"] = accountId;
    currentData["campaignId"] = currentCampaignId();
    const QString assignedToId = currentAssignedToId();
    currentData["assignedUserId"] = assignedToId;
    currentData["assignedToId"] = assignedToId;
    currentData["reportsToId"] = currentReportsToId();
    return currentData;
}

void DetailsWidget::slotModified()
{
    mModified = true;
    mUi.saveButton->setEnabled(true);
    mUi.discardButton->setEnabled(true);
}

/*
 * Fill in the data map with the current data and emit a signal
 *
 */
void DetailsWidget::saveData()
{
    if (!mData.empty()) {
        mData.clear();
    }

    QMap<QString, QString> detailsMap = data();
    QMap<QString, QString>::const_iterator i = detailsMap.constBegin();
    while (i != detailsMap.constEnd()) {
        mData[i.key()] = mData[i.value()];
        ++i;
    }

    if (mCreateNew) {
        emit createItem();
    } else {
        emit modifyItem();
    }
}

void DetailsWidget::slotDiscardData()
{
    setItem(mItem);
}

/*
 * Return the selected Id of "Account Name" or "Parent Name"
 */
QString DetailsWidget::currentAccountId() const
{
    if (mType != Campaign) {
        const QList<QComboBox *> comboBoxes =  mUi.informationGB->findChildren<QComboBox *>();
        Q_FOREACH (QComboBox *cb, comboBoxes) {
            const QString key = cb->objectName();
            if (key == QLatin1String("parentName") || key == QLatin1String("accountName")) {
                return cb->itemData(cb->currentIndex(), ReferencedDataModel::IdRole).toString();
            }
        }
    }
    return QString();
}

/*
 * Return the selected Campaign Id.
 */
QString DetailsWidget::currentCampaignId() const
{
    if (mType != Campaign) {
        const QList<QComboBox *> comboBoxes =  mUi.informationGB->findChildren<QComboBox *>();
        Q_FOREACH (QComboBox *cb, comboBoxes) {
            const QString key = cb->objectName();
            if (key == QLatin1String("campaignName") || key == QLatin1String("campaign")) {
                return cb->itemData(cb->currentIndex(), ReferencedDataModel::IdRole).toString();
            }
        }
    }
    return QString();
}

/*
 * Return the selected "Assigned To" Id
 */
QString DetailsWidget::currentAssignedToId() const
{
    const QList<QComboBox *> comboBoxes =  mUi.informationGB->findChildren<QComboBox *>();
    Q_FOREACH (QComboBox *cb, comboBoxes) {
        const QString key = cb->objectName();
        if (key == QLatin1String("assignedUserName") || key == QLatin1String("assignedTo")) {
            return cb->itemData(cb->currentIndex(), ReferencedDataModel::IdRole).toString();
        }
    }
    return QString();
}

/*
 * Return the selected "Reports To" Id
 */
QString DetailsWidget::currentReportsToId() const
{
    if (mType == Contact) {
        const QList<QComboBox *> comboBoxes =  mUi.informationGB->findChildren<QComboBox *>();
        Q_FOREACH (QComboBox *cb, comboBoxes) {
            const QString key = cb->objectName();
            if (key == QLatin1String("reportsTo")) {
                return cb->itemData(cb->currentIndex(), ReferencedDataModel::IdRole).toString();
            }
        }
    }
    return QString();
}

Details *DetailsWidget::createDetailsForType(DetailsType type)
{
    switch (type) {
    case Account: return new AccountDetails;
    case Opportunity: return new OpportunityDetails;
    case Contact: return new ContactDetails;
    case Lead: return new LeadDetails;
    case Campaign: return new CampaignDetails;
    }

    return 0;
}

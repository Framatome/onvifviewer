﻿#include "onvifdevicemanagermodel.h"

#include "onvifdevice.h"
#include "onvifdevicemanager.h"
#include <QMetaProperty>
#include <QDebug>

OnvifDeviceManagerModel::OnvifDeviceManagerModel(const OnvifDeviceManager *deviceManager, QObject *parent) :
    QAbstractListModel(parent),
    m_deviceManager(deviceManager)
{
    connect(m_deviceManager, &OnvifDeviceManager::deviceListChanged, this, &OnvifDeviceManagerModel::deviceListChanged);
    deviceListChanged();
}

int OnvifDeviceManagerModel::rowCount(const QModelIndex &) const
{
    return m_deviceManager->deviceList().count();
}

QVariant OnvifDeviceManagerModel::data(const QModelIndex &index, int role) const
{
    Q_ASSERT(index.row() < m_deviceManager->deviceList().size());
    OnvifDevice * device = m_deviceManager->deviceList().value(index.row());
    if(!device)
    {
        qDebug() << "OnvifDeviceManagerModel" << "Invalid index" << index;
        return QVariant();
    }

    if(role == Qt::DisplayRole)
    {
        return QVariant::fromValue(device);
    }

    const QMetaObject* metaObject = device->metaObject();
    if(role >= Qt::UserRole && role < Qt::UserRole + metaObject->propertyCount())
    {
        const QMetaProperty& prop = metaObject->property(role - Qt::UserRole);
        return device->property(prop.name());
    } else {
        qDebug() << "OnvifDeviceManagerModel" << "Unknown role" << index << role;
    }
    return QVariant();
}

QVariant OnvifDeviceManagerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    qDebug() << "headerData" << section << orientation << role;
    return QVariant();
}

QHash<int, QByteArray> OnvifDeviceManagerModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    const QMetaObject& metaObject = OnvifDevice::staticMetaObject;
    for(int i = 0; i < metaObject.propertyCount(); i++)
    {
        roles[Qt::UserRole + i] = QByteArray(metaObject.property(i).name());
    }
    return roles;
}

void OnvifDeviceManagerModel::deviceListChanged()
{
    int indexOfMethod = this->metaObject()->indexOfMethod("deviceChanged()");
    Q_ASSERT(indexOfMethod != -1);
    const QMetaMethod& targetSlot = this->metaObject()->method(indexOfMethod);

    beginResetModel();
    for(auto device : m_deviceManager->deviceList())
    {
        const QMetaObject* metaObject = device->metaObject();
        for(int i = 0; i < metaObject->propertyCount(); i++)
        {
            const QMetaProperty& prop = metaObject->property(i);
            if(prop.hasNotifySignal())
                connect(device, prop.notifySignal(), this, targetSlot);
        }
    }
    endResetModel();
}

void OnvifDeviceManagerModel::deviceChanged()
{
    OnvifDevice * device = qobject_cast<OnvifDevice*>(sender());
    Q_ASSERT(device);

    int i = m_deviceManager->deviceList().indexOf(device);
    emit dataChanged(index(i),index(i));
}

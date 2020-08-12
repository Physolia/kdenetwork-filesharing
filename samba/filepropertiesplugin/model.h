/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2011 Rodrigo Belem <rclbelem@gmail.com>
*/

#ifndef model_h
#define model_h

#include <QStringList>
#include <QAbstractTableModel>
#include <ksambasharedata.h>

class KSambaShareData;

class UserPermissionModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column {
        ColumnUsername,
        ColumnAccess
    };
    Q_ENUM(Column)

    explicit UserPermissionModel(KSambaShareData &shareData, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex & index) const override;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;

    QString getAcl() const;

private:
    QStringList m_userList;
    KSambaShareData m_shareData;
    QVariantMap m_usersAcl;

    void setupData();
    static QStringList getUsersList();
};

#endif

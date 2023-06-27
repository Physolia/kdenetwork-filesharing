/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2020 Harald Sitter <sitter@kde.org>
*/

import QtQuick 2.12
import QtQuick.Controls 2.5 as QQC2
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.4 as Kirigami
import org.kde.filesharing.samba 1.0 as Samba

// When built without packagekit we cannot do auto-installation.
ColumnLayout {
    QQC2.Label {
        Layout.alignment: Qt.AlignHCenter
        Layout.fillWidth: true
        text: xi18nc("@info", "The <application>Samba</application> file sharing service must be installed before folders can be shared.")
        explanation: i18n("Because this distro does not include PackageKit, we cannot show you a nice \"Install it\" button, and you will have to use your package manager to install the <command>samba</command> server package manually.")
        wrapMode: Text.Wrap
    }
    Item {
        Layout.alignment: Qt.AlignHCenter
        Layout.fillHeight: true // space everything up
    }
}

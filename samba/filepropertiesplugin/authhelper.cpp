/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2020-2021 Harald Sitter <sitter@kde.org>
*/

#include "authhelper.h"
#include <kauth/helpersupport.h>
#include <QProcess>
#include <QRegularExpression>
#include <KLocalizedString>

static bool isValidUserName(const QString &name)
{
    // https://systemd.io/USER_NAMES/
    static QRegularExpression expr(QStringLiteral("^[a-z_][a-z0-9_-]*$"));
    return expr.match(name).hasMatch();
}

ActionReply AuthHelper::isuserknown(const QVariantMap &args)
{
    const auto username = args.value(QStringLiteral("username")).toString();
    if (!isValidUserName(username)) {
        auto reply = ActionReply::HelperErrorReply();
        reply.setErrorDescription(xi18nc("@info", "User name <resource>%1</resource> is not valid as the name of a Samba user; cannot check for its existence.", username));
        return reply;
    }

    QProcess p;
    const auto program = QStringLiteral("pdbedit");
    const auto arguments = QStringList({QStringLiteral("--debuglevel=0"), QStringLiteral("--user"), username });
    p.setProgram(program);
    p.setArguments(arguments);
    p.start();
    // Should be fairly quick: short timeout.
    const int pdbeditTimeout = 4000; // milliseconds
    p.waitForFinished(pdbeditTimeout);

    if (p.exitStatus() != QProcess::NormalExit) {
        // QByteArray can't do direct conversion to QString
        const QString errorText = QString::fromUtf8(p.readAllStandardOutput());
        auto reply = ActionReply::HelperErrorReply();
        reply.setErrorDescription(xi18nc("@info '%1 %2' together make up a terminal command; %3 is the command's output",
                                         "Command <command>%1 %2</command> failed:<nl/><nl/>%3", program, arguments.join(QLatin1Char(' ')), errorText));
        return reply;
    }

    ActionReply reply;
    reply.addData(QStringLiteral("exists"), p.exitCode() == 0);
    return reply;
}

ActionReply AuthHelper::createuser(const QVariantMap &args)
{
    const auto username = args.value(QStringLiteral("username")).toString();
    const auto password = args.value(QStringLiteral("password")).toString();
    if (!isValidUserName(username)) {
        auto reply = ActionReply::HelperErrorReply();
        reply.setErrorDescription(xi18nc("@info", "<resource>%1</resource> is not a valid Samba user name; cannot create it.", username));
        return reply;
    }
    if (password.isEmpty()) {
        auto reply = ActionReply::HelperErrorReply();
        reply.setErrorDescription(i18nc("@info", "For security reasons, creating Samba users with empty passwords is not allowed."));
        return reply;
    }

    QProcess p;
    p.setProgram(QStringLiteral("smbpasswd"));
    p.setArguments({
        QStringLiteral("-L"), /* local mode */
        QStringLiteral("-s"), /* read from stdin */
        QStringLiteral("-D"), QStringLiteral("0"), /* force-disable debug */
        QStringLiteral("-a"), /* add user */
        username });
    p.start();
    // despite being in silent mode we still need to write the password twice!
    p.write((password + QStringLiteral("\n")).toUtf8());
    p.write((password + QStringLiteral("\n")).toUtf8());
    p.waitForBytesWritten();
    p.closeWriteChannel();
    p.waitForFinished();

    if (p.exitStatus() != QProcess::NormalExit) {
        auto reply = ActionReply::HelperErrorReply();
        reply.setErrorDescription(QString::fromUtf8(p.readAllStandardError()));
        return reply;
    }

    ActionReply reply;
    reply.addData(QStringLiteral("created"), p.exitCode() == 0);
    // stderr will generally contain info on what went wrong so forward it
    // so the UI may display it
    reply.addData(QStringLiteral("stderr"), p.readAllStandardError());
    return reply;
}

ActionReply AuthHelper::addtogroup(const QVariantMap &args)
{
    const auto user = args.value(QStringLiteral("user")).toString();
    const auto group = args.value(QStringLiteral("group")).toString();
    if (!isValidUserName(user)) {
        auto reply = ActionReply::HelperErrorReply();
        reply.setErrorDescription(xi18nc("@info", "<resource>%1</resource> is not a valid user name; cannot add it to the group <resource>%2</resource>.", user, group));
        return reply;
    }
    if (!isValidUserName(group)) {
        auto reply = ActionReply::HelperErrorReply();
        reply.setErrorDescription(xi18nc("@info", "<resource>%1</resource> is not a valid group name; cannot make user <resource>%2</resource> a member of it.", group, user));
        return reply;
    }
    // Harden against some input abuse.
    // TODO: add ability to resolve remote UID via KAuth and verify the request (or even reduce the arguments down to
    //    only the group and resolve the UID)
    // Keep this condition in sync with the one in groupmanager.cpp
    if (!group.contains(QLatin1String("samba")) || group.contains(QLatin1String("admin")) ||
        group.contains(QLatin1String("root"))) {
        auto reply = ActionReply::HelperErrorReply();
        reply.setErrorDescription(xi18nc("@info", "For security reasons, cannot make user <resource>%1</resource> a member of group <resource>%2</resource>. \
                                                   The group name is insecure; valid group names include the text <resource>samba</resource> and do not \
                                                   include the text <resource>admin</resource> or <resource>root</resource>."));
        return reply;
    }

    QProcess p;
#if defined(Q_OS_FREEBSD)
    p.setProgram(QStringLiteral("pw"));
    p.setArguments({
        QStringLiteral("group"),
        QStringLiteral("mod"),
        QStringLiteral("{%1}").arg(group),
        QStringLiteral("-m"),
        QStringLiteral("{%1}").arg(user) });
#elif defined(Q_OS_LINUX) || defined(Q_OS_HURD)
    p.setProgram(QStringLiteral("/usr/sbin/usermod"));
    p.setArguments({
        QStringLiteral("--append"),
        QStringLiteral("--groups"),
        group,
        user });
#else
#   error "Platform lacks group management support. Please add support."
#endif

    p.start();
    p.waitForFinished(1000);

    if (p.exitCode() != 0 || p.exitStatus() != QProcess::NormalExit) {
        auto reply = ActionReply::HelperErrorReply();
        reply.setErrorDescription(QString::fromUtf8(p.readAll()));
        return reply;
    }

    return ActionReply::SuccessReply();
}

KAUTH_HELPER_MAIN("org.kde.filesharing.samba", AuthHelper)

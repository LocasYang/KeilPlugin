#include "uvprojxhook.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QXmlStreamReader>

#include <functional>

#include "keilproject.h"

namespace {

const QRegularExpression &afterMakeRe()
{
    static const QRegularExpression re(QStringLiteral("<AfterMake>.*?</AfterMake>"),
                                       QRegularExpression::DotMatchesEverythingOption);
    return re;
}

const QRegularExpression &wrapRe()
{
    // Matches both "<exe> --wrap "ORIG" "x.uvprojx"" written by this app and
    // the legacy "python "tool.py" --wrap "ORIG" "x.uvprojx"" commands. The
    // trailing argument is either the project file (legacy) or "#L" (current),
    // optionally followed by "--project "<uvprojx>"".
    static const QRegularExpression re(
        QStringLiteral("--wrap\\s+\"(.*?)\"\\s+\"[^\"]*\""
                       "(?:\\s+--project\\s+\"[^\"]*\")?\\s*$"),
        QRegularExpression::DotMatchesEverythingOption);
    return re;
}

// Base64-wrapped form used when the original command itself contains double
// quotes: "<exe> --wrap-b64 <B64> "#L" [--project "<uvprojx>"]".
const QRegularExpression &wrapB64Re()
{
    static const QRegularExpression re(
        QStringLiteral("--wrap-b64\\s+([A-Za-z0-9+/=]+)\\s+\"[^\"]*\""
                       "(?:\\s+--project\\s+\"[^\"]*\")?\\s*$"),
        QRegularExpression::DotMatchesEverythingOption);
    return re;
}

QString base64Encode(const QString &text)
{
    return QString::fromLatin1(text.toUtf8().toBase64());
}

QString base64Decode(const QString &text)
{
    return QString::fromUtf8(QByteArray::fromBase64(text.toLatin1()));
}

QString slotRegex(int n, const char *format)
{
    return QString::fromLatin1(format).arg(n);
}

QString getSlot(const QString &block, int n)
{
    const QRegularExpression re(slotRegex(n, "<UserProg%1Name>(.*?)</UserProg%1Name>"),
                                QRegularExpression::DotMatchesEverythingOption);
    const auto match = re.match(block);
    return match.hasMatch() ? match.captured(1) : QString();
}

QString xmlEsc(QString value)
{
    value.replace(QLatin1Char('&'), QStringLiteral("&amp;"));
    value.replace(QLatin1Char('<'), QStringLiteral("&lt;"));
    value.replace(QLatin1Char('>'), QStringLiteral("&gt;"));
    return value;
}

QString xmlUnesc(QString value)
{
    value.replace(QStringLiteral("&lt;"), QLatin1String("<"));
    value.replace(QStringLiteral("&gt;"), QLatin1String(">"));
    value.replace(QStringLiteral("&quot;"), QLatin1String("\""));
    value.replace(QStringLiteral("&amp;"), QLatin1String("&"));
    return value;
}

QString regexReplace(const QString &subject, const QRegularExpression &re,
                     const std::function<QString(const QRegularExpressionMatch &)> &fn)
{
    QString out;
    qsizetype last = 0;
    auto it = re.globalMatch(subject);
    while (it.hasNext()) {
        const auto match = it.next();
        out += subject.mid(last, match.capturedStart() - last);
        out += fn(match);
        last = match.capturedEnd();
    }
    out += subject.mid(last);
    return out;
}

QString setSlot(const QString &block, int n, const QString &value, bool run)
{
    const QRegularExpression nameRe(slotRegex(n, "<UserProg%1Name>.*?</UserProg%1Name>"),
                                    QRegularExpression::DotMatchesEverythingOption);
    QString result = regexReplace(block, nameRe, [n, &value](const QRegularExpressionMatch &) {
        return QStringLiteral("<UserProg%1Name>%2</UserProg%1Name>").arg(n).arg(value);
    });

    const QRegularExpression runRe(slotRegex(n, "<RunUserProg%1>\\d</RunUserProg%1>"));
    result = regexReplace(result, runRe, [n, run](const QRegularExpressionMatch &) {
        return QStringLiteral("<RunUserProg%1>%2</RunUserProg%1>").arg(n).arg(run ? 1 : 0);
    });
    return result;
}

bool validateXml(const QString &text, QString *error)
{
    QXmlStreamReader reader(text);
    while (!reader.atEnd()) {
        if (reader.readNext() == QXmlStreamReader::Invalid)
            break;
    }
    if (reader.hasError()) {
        if (error)
            *error = reader.errorString();
        return false;
    }
    return true;
}

} // namespace

namespace UvprojxHook {

QStringList toolMarkers()
{
    static const QStringList markers {
        QStringLiteral("keilpluginstudio.exe"),
        QStringLiteral("--size-report"),
        QStringLiteral("--wrap"),
        QStringLiteral("keil_size_report.py"),
    };
    return markers;
}

bool containsToolMarker(const QString &text)
{
    const QString lowered = text.toLower();
    for (const QString &marker : toolMarkers()) {
        if (lowered.contains(marker))
            return true;
    }
    return false;
}

QString hookCommand(const QString &uvprojxPath, const QString &wrap, bool wrapBase64)
{
    const QString exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    // "#L" is expanded by Keil to the current target's linker output (.axf),
    // so each target reports only itself after its own build. The --project
    // context lets the CLI restore the target's device and IROM/IRAM
    // capacities, which the axf alone cannot provide.
    QString command;
    if (!wrap.isEmpty()) {
        if (wrapBase64)
            command = QStringLiteral("\"%1\" --wrap-b64 %2 \"#L\"").arg(exe, base64Encode(wrap));
        else
            command = QStringLiteral("\"%1\" --wrap \"%2\" \"#L\"").arg(exe, wrap);
    } else {
        command = QStringLiteral("\"%1\" --size-report \"#L\"").arg(exe);
    }
    if (!uvprojxPath.isEmpty())
        command += QStringLiteral(" --project \"%1\"")
                       .arg(QDir::toNativeSeparators(QFileInfo(uvprojxPath).absoluteFilePath()));
    return command;
}

PatchOutcome patchProjectText(const QString &text, const QString &uvprojxPath, bool remove)
{
    PatchOutcome outcome;
    const QString projectBase = QFileInfo(uvprojxPath).fileName();

    outcome.newText = regexReplace(text, afterMakeRe(),
                                   [&](const QRegularExpressionMatch &match) -> QString {
        QString block = match.captured(0);

        if (remove) {
            if (!containsToolMarker(block))
                return block;
            QString updated = block;
            for (int n : { 2, 1 }) {
                const QString value = getSlot(updated, n);
                if (!containsToolMarker(value))
                    continue;
                const auto wrappedB64 = wrapB64Re().match(value);
                if (wrappedB64.hasMatch()) {
                    updated = setSlot(updated, n, xmlEsc(base64Decode(wrappedB64.captured(1))), true);
                    outcome.notes.append(
                        QStringLiteral("%1 · slot%2: 解开 base64 包装, 已还原原命令").arg(projectBase).arg(n));
                    continue;
                }
                const auto wrapped = wrapRe().match(value);
                if (wrapped.hasMatch()) {
                    updated = setSlot(updated, n, xmlEsc(xmlUnesc(wrapped.captured(1))), true);
                    outcome.notes.append(
                        QStringLiteral("%1 · slot%2: 解开包装, 已还原原命令").arg(projectBase).arg(n));
                    continue;
                }
                if (value.contains(QStringLiteral("&amp;&amp;"))) {
                    QStringList kept;
                    const QStringList parts = value.split(QStringLiteral("&amp;&amp;"));
                    for (const QString &part : parts) {
                        if (!containsToolMarker(part))
                            kept.append(part);
                    }
                    if (!kept.isEmpty()) {
                        updated = setSlot(updated, n, kept.join(QStringLiteral("&amp;&amp;")), true);
                        outcome.notes.append(
                            QStringLiteral("%1 · slot%2: 已从命令链中移除报告钩子").arg(projectBase).arg(n));
                        continue;
                    }
                    outcome.notes.append(
                        QStringLiteral("%1 · slot%2: 命令链已空, 清空槽位").arg(projectBase).arg(n));
                } else {
                    outcome.notes.append(
                        QStringLiteral("%1 · slot%2: 已清除").arg(projectBase).arg(n));
                }
                updated = setSlot(updated, n, QString(), false);
            }
            outcome.changed |= (updated != block);
            return updated;
        }

        if (containsToolMarker(block)) {
            // Already installed: upgrade hooks written by older builds (or the
            // legacy Python tool) to the current "#L" per-target format.
            QString upgraded = block;
            bool changedSlot = false;
            for (int n : { 2, 1 }) {
                const QString value = getSlot(upgraded, n);
                if (!containsToolMarker(value))
                    continue;
                const auto wrappedB64 = wrapB64Re().match(value);
                if (wrappedB64.hasMatch()) {
                    // Older base64 hooks lack the "--project" context: decode,
                    // regenerate and keep the wrapped original unchanged.
                    if (!value.contains(QLatin1String("--project"))) {
                        const QString original = base64Decode(wrappedB64.captured(1));
                        upgraded = setSlot(upgraded, n,
                                           xmlEsc(hookCommand(uvprojxPath, original, true)),
                                           true);
                        outcome.notes.append(
                            QStringLiteral("%1 · slot%2: 钩子已升级 (--project 容量上下文)")
                                .arg(projectBase).arg(n));
                        changedSlot = true;
                    }
                    continue;
                }
                const auto wrapped = wrapRe().match(value);
                if (wrapped.hasMatch()) {
                    const QString original = xmlUnesc(wrapped.captured(1));
                    upgraded = setSlot(upgraded, n,
                                       xmlEsc(hookCommand(uvprojxPath, original,
                                                          original.contains(QLatin1Char('"')))),
                                       true);
                    outcome.notes.append(
                        QStringLiteral("%1 · slot%2: 钩子已升级 (#L 按 Target 过滤)").arg(projectBase).arg(n));
                    changedSlot = true;
                    continue;
                }
                if (value.contains(QStringLiteral("--size-report"))
                    && !value.contains(QLatin1String("#L"))) {
                    upgraded = setSlot(upgraded, n, xmlEsc(hookCommand(uvprojxPath)), true);
                    outcome.notes.append(
                        QStringLiteral("%1 · slot%2: 钩子已升级 (#L 按 Target 过滤)").arg(projectBase).arg(n));
                    changedSlot = true;
                }
            }
            if (!changedSlot)
                outcome.notes.append(QStringLiteral("%1 · 已经安装, 跳过").arg(projectBase));
            outcome.changed |= changedSlot;
            return upgraded;
        }
        for (int n : { 2, 1 }) {
            if (!getSlot(block, n).trimmed().isEmpty())
                continue;
            const QString command = hookCommand(uvprojxPath);
            outcome.notes.append(QStringLiteral("%1 · slot%2: %3").arg(projectBase).arg(n).arg(command));
            outcome.changed = true;
            return setSlot(block, n, xmlEsc(command), true);
        }
        // Both slots busy: wrap the original slot-2 command so it still runs
        // before the report is printed. Commands containing double quotes
        // cannot survive the Keil -> exe -> cmd quoting chain verbatim, so
        // they are base64-wrapped instead of being skipped.
        const QString original = xmlUnesc(getSlot(block, 2));
        const bool needsBase64 = original.contains(QLatin1Char('"'));
        const QString command = hookCommand(uvprojxPath, original, needsBase64);
        if (needsBase64) {
            outcome.notes.append(
                QStringLiteral("%1 · slot2: 原命令含引号, 已 base64 包装 [%2] -> 先运行再打印报告")
                    .arg(projectBase, original));
        } else {
            outcome.notes.append(QStringLiteral("%1 · slot2: 包装原命令 [%2] -> 先运行再打印报告")
                                     .arg(projectBase, original));
        }
        outcome.changed = true;
        return setSlot(block, 2, xmlEsc(command), true);
    });

    return outcome;
}

PatchOutcome installInFile(const QString &uvprojxPath)
{
    PatchOutcome outcome;
    const ProjectText::Decoded decoded = ProjectText::decodeFile(uvprojxPath);
    if (!decoded.ok) {
        outcome.error = decoded.error;
        return outcome;
    }

    const PatchOutcome patched = patchProjectText(decoded.text, uvprojxPath, false);
    outcome.notes = patched.notes;
    if (!patched.changed) {
        outcome.newText = decoded.text;
        return outcome;
    }

    QString xmlError;
    if (!validateXml(patched.newText, &xmlError)) {
        outcome.error = QStringLiteral("写回前 XML 校验失败 (%1): %2").arg(uvprojxPath, xmlError);
        outcome.newText = decoded.text;
        return outcome;
    }

    QFile file(uvprojxPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        outcome.error = QStringLiteral("无法写入文件: %1").arg(uvprojxPath);
        return outcome;
    }
    file.write(ProjectText::encodeText(patched.newText, decoded.isUtf8, decoded.hadBom));
    file.close();

    outcome.newText = patched.newText;
    outcome.changed = true;
    return outcome;
}

PatchOutcome uninstallInFile(const QString &uvprojxPath)
{
    PatchOutcome outcome;
    const ProjectText::Decoded decoded = ProjectText::decodeFile(uvprojxPath);
    if (!decoded.ok) {
        outcome.error = decoded.error;
        return outcome;
    }

    const PatchOutcome patched = patchProjectText(decoded.text, uvprojxPath, true);
    outcome.notes = patched.notes;
    if (!patched.changed) {
        outcome.newText = decoded.text;
        return outcome;
    }

    QString xmlError;
    if (!validateXml(patched.newText, &xmlError)) {
        outcome.error = QStringLiteral("写回前 XML 校验失败 (%1): %2").arg(uvprojxPath, xmlError);
        outcome.newText = decoded.text;
        return outcome;
    }

    QFile file(uvprojxPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        outcome.error = QStringLiteral("无法写入文件: %1").arg(uvprojxPath);
        return outcome;
    }
    file.write(ProjectText::encodeText(patched.newText, decoded.isUtf8, decoded.hadBom));
    file.close();

    outcome.newText = patched.newText;
    outcome.changed = true;
    return outcome;
}

} // namespace UvprojxHook

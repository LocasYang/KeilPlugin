#include "novoproject.h"
#include "mapparser.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QXmlStreamReader>

namespace {
QString projectFile(const QString &path)
{
    const QFileInfo info(path);
    return QDir(info.isDir() ? info.absoluteFilePath() : info.absolutePath()).filePath(".cproject");
}
QString xmlEscape(QString s)
{
    return s.replace('&', "&amp;").replace('"', "&quot;").replace('<', "&lt;")
            .replace('>', "&gt;").replace('\r', "&#13;").replace('\n', "&#10;");
}
bool validXml(const QString &text)
{
    QXmlStreamReader xml(text);
    while (!xml.atEnd()) xml.readNext();
    return !xml.hasError();
}
QString expand(QString value, const QString &root, const QString &project, const QString &config)
{
    value.replace("${ProjName}", project).replace("${ProjectName}", project)
         .replace("${ConfigName}", config).replace("${ProjDirPath}", root)
         .replace("${ProjectDirPath}", root)
         .replace(QStringLiteral("${workspace_loc:/%1}").arg(project), root);
    return QDir::fromNativeSeparators(value);
}
}

KeilProjectData NovoProject::parse(const QString &path)
{
    KeilProjectData data;
    data.kind = "cproject";
    data.sourcePath = QFileInfo(path).absoluteFilePath();
    const QString filePath = projectFile(path);
    const auto decoded = ProjectText::decodeFile(filePath);
    if (!decoded.ok) { data.error = decoded.error; return data; }
    if (!validXml(decoded.text)) { data.error = "Invalid .cproject XML"; return data; }
    if (!decoded.text.contains("ilg.gnuarmeclipse.managedbuild.cross")
        && !decoded.text.contains("ilg.gnumcueclipse.managedbuild.cross")
        && !decoded.text.contains("org.eclipse.embedcdt.managedbuild.cross")) {
        data.error = "Only NovoStudio / GNU Arm managed-build .cproject files are supported";
        return data;
    }
    const QString root = QFileInfo(filePath).absolutePath();
    QString name = QDir(root).dirName(), device;
    QFile project(QDir(root).filePath(".project"));
    if (project.open(QIODevice::ReadOnly)) {
        QXmlStreamReader xml(project.readAll());
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement() && xml.name() == QLatin1String("name")) {
                name = xml.readElementText(); break;
            }
        }
    }
    QFile novo(QDir(root).filePath(".novoproject"));
    if (novo.open(QIODevice::ReadOnly))
        device = QJsonDocument::fromJson(novo.readAll()).object().value("partNumber").toString();
    QXmlStreamReader xml(decoded.text);
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement() || xml.name() != QLatin1String("configuration")
            || !xml.attributes().hasAttribute("artifactName")) continue;
        const auto attrs = xml.attributes();
        KeilTarget target;
        target.project = name; target.uvprojx = filePath;
        target.name = attrs.value("name").toString(); target.device = device;
        target.outputName = expand(attrs.value("artifactName").toString(), root, name, target.name);
        QString extension = attrs.value("artifactExtension").toString();
        if (extension.isEmpty()) extension = "elf";
        target.outputDir = QDir(root).filePath(target.name);
        const QString command = attrs.value("postbuildStep").toString();
        target.hookInstalled = command.contains("--novo-hook");
        if (target.hookInstalled) target.reportLanguage = UvprojxHook::reportLanguage(command);
        QString cpu;
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isEndElement() && xml.name() == QLatin1String("configuration")) break;
            if (!xml.isStartElement()) continue;
            const auto a = xml.attributes();
            if (xml.name() == QLatin1String("builder") && a.hasAttribute("buildPath"))
                target.outputDir = expand(a.value("buildPath").toString(), root, name, target.name);
            if (xml.name() == QLatin1String("option")) {
                const QString option = a.value("superClass").toString();
                const QString value = a.value("value").toString();
                if (option.contains("target.family")) cpu = value.section('.', -1);
                if (option.contains("lto") || option.contains("linktimeoptimization")) {
                    target.ltoKnown = true; target.ltoEnabled = value == "true";
                }
            }
        }
        if (target.device.isEmpty()) target.device = cpu;
        if (target.outputName.contains("${") || target.outputDir.contains("${")) {
            data.error = QStringLiteral("Unresolved build path/artifact macro in %1").arg(target.name);
            data.targets.clear(); return data;
        }
        if (QDir::isRelativePath(target.outputDir)) target.outputDir = QDir(root).filePath(target.outputDir);
        target.outputDir = QDir::cleanPath(target.outputDir);
        target.listingDir = target.outputDir;
        target.elfPath = QDir(target.outputDir).filePath(target.outputName + "." + extension);
        target.mapPath = QDir(target.outputDir).filePath(target.outputName + ".map");
        target.mapFound = QFileInfo::exists(target.mapPath);
        if (target.mapFound) {
            const auto summary = MapParser::parse(target.mapPath, target.elfPath);
            if (summary.ok) { target.flashCap = summary.flashCapacity; target.ramCap = summary.ramCapacity; }
        }
        data.targets.append(target);
    }
    if (data.targets.isEmpty()) data.error = "No GNU Arm build configurations found";
    else data.uvprojxFiles.append(filePath);
    return data;
}

UvprojxHook::PatchOutcome NovoProject::patchText(const QString &text, const QString &projectPath,
                                               bool remove, const QString &language)
{
    UvprojxHook::PatchOutcome out;
    out.newText = text;
    if (!validXml(text)) { out.error = "Invalid .cproject XML"; return out; }
    static const QRegularExpression tagRe(R"(<configuration\b[^>]*>)");
    static const QRegularExpression stepRe(R"re(\s+postbuildStep\s*=\s*(?:"[^"]*"|'[^']*'))re");
    static const QRegularExpression ownRe(
        R"re((;\s*)?"[^"]+" --size-report "\$\{BuildArtifactFileName\}" --project "[^"]+" --target "\$\{ConfigName\}" --lang (?:en|zh-CN) --novo-hook=([01]))re");
    auto it = tagRe.globalMatch(text);
    struct Edit { qsizetype start, size; QString value; };
    QVector<Edit> edits;
    int configurations = 0;
    while (it.hasNext()) {
        const auto m = it.next();
        QString tag = m.captured();
        QXmlStreamReader xml(tag.left(tag.size() - 1) + "/>");
        if (!xml.readNextStartElement() || !xml.attributes().hasAttribute("artifactName")) continue;
        ++configurations;
        const auto attrs = xml.attributes();
        const bool hadStep = attrs.hasAttribute("postbuildStep");
        QString command = attrs.value("postbuildStep").toString();
        bool originalHadStep = hadStep;
        const auto own = ownRe.match(command);
        if (command.contains("--novo-hook") && !own.hasMatch()) {
            out.error = "The NovoStudio report command was edited; restore its generated form before updating/removing it";
            return out;
        }
        if (own.hasMatch()) {
            originalHadStep = own.captured(2) == "1";
            command.remove(own.capturedStart(), own.capturedLength());
        } else if (remove) continue;
        if (!remove) {
            const QString exe = QDir::fromNativeSeparators(QCoreApplication::applicationFilePath());
            const QString project = QDir::fromNativeSeparators(QFileInfo(projectPath).absoluteFilePath());
            // CDT separates post-build commands at semicolons. Refuse paths
            // which cannot be represented there rather than altering commands.
            if (exe.contains(';') || project.contains(';') || exe.contains('"') || project.contains('"')) {
                out.error = "Executable/project paths cannot contain semicolons or quotes for CDT post-build steps";
                return out;
            }
            if (!command.isEmpty()) command += "; ";
            command += QStringLiteral("\"%1\" --size-report \"${BuildArtifactFileName}\" --project \"%2\" --target \"${ConfigName}\" --lang %3 --novo-hook=%4")
                           .arg(exe, project, language == "zh-CN" ? "zh-CN" : "en",
                                originalHadStep ? "1" : "0");
        }
        const auto step = stepRe.match(tag);
        const QString replacement = command.isEmpty() && !originalHadStep ? QString()
                : " postbuildStep=\"" + xmlEscape(command) + "\"";
        if (step.hasMatch()) tag.replace(step.capturedStart(), step.capturedLength(), replacement);
        else tag.insert(tag.size() - 1, replacement);
        if (tag != m.captured()) edits.append({m.capturedStart(), m.capturedLength(), tag});
    }
    if (!configurations) { out.error = "No managed-build configuration found"; return out; }
    for (auto it = edits.crbegin(); it != edits.crend(); ++it)
        out.newText.replace(it->start, it->size, it->value);
    if (!validXml(out.newText)) { out.error = "Patched .cproject XML validation failed"; return out; }
    out.changed = out.newText != text;
    return out;
}

UvprojxHook::PatchOutcome NovoProject::patchFile(const QString &path, bool remove, const QString &language)
{
    const QString filePath = projectFile(path);
    const auto data = parse(filePath);
    UvprojxHook::PatchOutcome out;
    if (!data.error.isEmpty()) { out.error = data.error; return out; }
    const auto decoded = ProjectText::decodeFile(filePath);
    if (!decoded.ok) { out.error = decoded.error; return out; }
    out = patchText(decoded.text, filePath, remove, language);
    if (!out.error.isEmpty() || !out.changed) return out;
    const QString backup = filePath + ".keilpluginstudio.bak";
    if (!QFileInfo::exists(backup) && !QFile::copy(filePath, backup)) {
        out.error = "Cannot create .cproject backup"; out.changed = false; return out;
    }
    QSaveFile file(filePath);
    const QByteArray bytes = ProjectText::encodeText(out.newText, decoded.isUtf8, decoded.hadBom);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        out.error = "Cannot save .cproject atomically"; out.changed = false;
    }
    return out;
}

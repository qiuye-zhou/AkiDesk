#include "page_character.h"
#include "ui_page_character.h"

#include "config/AppPaths.h"
#include "config/JsonConfig.h"
#include "utils/PluginManager.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace
{
/* 跨平台 ZIP 解压（Windows 使用 PowerShell，其他平台使用 Python zipfile） */
bool extractZip(const QString &zipPath, const QString &targetDir, QString *err)
{
#ifdef Q_OS_WIN
    QString cmd = QString("Expand-Archive -Path '%1' -DestinationPath '%2' -Force")
                      .arg(zipPath, targetDir);
    int ret = QProcess::execute("powershell", {"-Command", cmd});
    if (ret != 0)
    {
        if (err) *err = "PowerShell 解压失败";
        return false;
    }
    return true;
#else
    QString python = QStandardPaths::findExecutable("python3");
    if (python.isEmpty())
        python = QStandardPaths::findExecutable("python");
    if (python.isEmpty())
    {
        if (err) *err = "未找到 Python 解释器";
        return false;
    }
    int ret = QProcess::execute(python, {"-c",
        "import zipfile,sys; zipfile.ZipFile(sys.argv[1]).extractall(sys.argv[2])",
        zipPath, targetDir});
    if (ret != 0)
    {
        if (err) *err = "Python 解压失败";
        return false;
    }
    return true;
#endif
}
} // namespace

PageCharacter::PageCharacter(QWidget *parent)
    : QWidget(parent), ui(new Ui::PageCharacter), m_pluginManager(new PluginManager)
{
    ui->setupUi(this);
    m_pluginManager->reload();
    refreshCharList();

    /* 从 ini 读取上次选中的角色 */
    QSettings settings(LocalConfigPath, QSettings::IniFormat);
    QString sel = settings.value("character/Selected", "").toString();
    if (!sel.isEmpty())
        ui->comboChar->setCurrentText(sel);
    loadCurrentConfig();

    /* 信号绑定 */
    connect(ui->comboChar, &QComboBox::currentTextChanged, this, &PageCharacter::onCharChanged);
    connect(ui->editPrompt, &QPlainTextEdit::textChanged, this, &PageCharacter::onPromptChanged);
    connect(ui->spinTachieSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &PageCharacter::onTachieSizeChanged);
    connect(ui->comboServer, &QComboBox::currentTextChanged, this, &PageCharacter::onServerChanged);
    connect(ui->comboModel, &QComboBox::currentTextChanged, this, &PageCharacter::onModelChanged);
    connect(ui->checkVits, &QCheckBox::toggled, this, &PageCharacter::onVitsToggled);
    connect(ui->comboVitsModel, &QComboBox::currentTextChanged, this, &PageCharacter::onVitsModelChanged);
    connect(ui->btnImport, &QPushButton::clicked, this, &PageCharacter::onImportCharacter);
    connect(ui->btnDelete, &QPushButton::clicked, this, &PageCharacter::onDeleteCharacter);
    connect(ui->btnResetLoc, &QPushButton::clicked, this, [this]() { emit requestResetTachieLoc(); });

    m_loading = true;
}

PageCharacter::~PageCharacter()
{
    delete m_pluginManager;
    delete ui;
}

/* 加载当前角色的所有配置到 UI 控件 */
void PageCharacter::loadCurrentConfig()
{
    m_loading = false;
    QString name = ui->comboChar->currentText();
    if (name.isEmpty())
    {
        ui->editPrompt->clear();
        ui->spinTachieSize->setValue(100);
        ui->comboServer->setCurrentIndex(0);
        ui->comboModel->clear();
        ui->checkVits->setChecked(false);
        ui->comboVitsModel->clear();
        m_loading = true;
        return;
    }

    /* 角色 Prompt */
    JsonConfig assetCfg(CurrentCharacterAssetConfig());
    ui->editPrompt->setPlainText(assetCfg.value("prompt").toString());

    /* 用户运行配置 */
    JsonConfig charCfg(CurrentCharacterUserConfig());
    ui->spinTachieSize->setValue(charCfg.value("tachieSize", "100").toString().toInt());
    ui->comboServer->setCurrentText(charCfg.value("serverSelect", "DeepSeek").toString());
    refreshModelList();
    ui->comboModel->setCurrentText(charCfg.value("modelSelect").toString());

    bool vits = charCfg.value("vitsEnable").toBool();
    ui->checkVits->setChecked(vits);
    ui->comboVitsModel->setEnabled(vits);

    /* VITS 模型列表 */
    refreshVitsModelList();
    ui->comboVitsModel->setCurrentText(charCfg.value("vitsMasSelect").toString());

    refreshTachieBindings();
    m_loading = true;
}

/* 刷新角色下拉列表 */
void PageCharacter::refreshCharList()
{
    QDir dir(CharacterAssetsPath);
    QStringList names = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    ui->comboChar->clear();
    ui->comboChar->addItems(names);
}

/* 刷新 LLM 模型列表 */
void PageCharacter::refreshModelList()
{
    QString server = ui->comboServer->currentText();
    JsonConfig cfg(GlobalConfigPath);
    QJsonArray arr = cfg.value("llm/" + server + "/ModelList").toArray();
    QStringList list;
    for (const QJsonValue &v : arr)
        list << v.toString();
    ui->comboModel->clear();
    ui->comboModel->addItems(list);
}

/* 刷新 VITS 模型列表 */
void PageCharacter::refreshVitsModelList()
{
    JsonConfig cfg(GlobalConfigPath);
    QJsonArray arr = cfg.value("vits/ModelAndSpeakerList").toArray();
    QStringList list;
    for (const QJsonValue &v : arr)
        list << v.toString();
    ui->comboVitsModel->clear();
    ui->comboVitsModel->addItems(list);
}

/* 刷新立绘动作 → 动画绑定列表 */
void PageCharacter::refreshTachieBindings()
{
    JsonConfig assetCfg(CurrentCharacterAssetConfig());
    QJsonObject map = assetCfg.value("tachieAnimations", QJsonObject()).toObject();
    QStringList entries;
    for (auto it = map.begin(); it != map.end(); ++it)
        entries << it.key() + " → " + it.value().toString();
    ui->listBindings->clear();
    ui->listBindings->addItems(entries);
}

void PageCharacter::onCharChanged(const QString &name)
{
    if (!m_loading) return;
    QSettings settings(LocalConfigPath, QSettings::IniFormat);
    settings.setValue("character/Selected", name);
    loadCurrentConfig();
    emit requestReloadCharSelect("default");
    emit requestReloadAi();
}

void PageCharacter::onPromptChanged()
{
    if (!m_loading) return;
    JsonConfig cfg(CurrentCharacterAssetConfig());
    cfg.setValue("prompt", ui->editPrompt->toPlainText());
}

void PageCharacter::onTachieSizeChanged(int value)
{
    if (!m_loading) return;
    JsonConfig cfg(CurrentCharacterUserConfig());
    cfg.setValue("tachieSize", QString::number(value));
    emit requestSetTachieSize(value);
}

void PageCharacter::onServerChanged(const QString &server)
{
    if (!m_loading) return;
    JsonConfig cfg(CurrentCharacterUserConfig());
    cfg.setValue("serverSelect", server);
    refreshModelList();
    emit requestReloadAi();
}

void PageCharacter::onModelChanged(const QString &model)
{
    if (!m_loading) return;
    JsonConfig cfg(CurrentCharacterUserConfig());
    cfg.setValue("modelSelect", model);
    emit requestReloadAi();
}

void PageCharacter::onVitsToggled(bool enabled)
{
    if (!m_loading) return;
    JsonConfig cfg(CurrentCharacterUserConfig());
    cfg.setValue("vitsEnable", enabled);
    ui->comboVitsModel->setEnabled(enabled);
}

void PageCharacter::onVitsModelChanged(const QString &model)
{
    if (!m_loading) return;
    JsonConfig cfg(CurrentCharacterUserConfig());
    cfg.setValue("vitsMasSelect", model);
}

/* 导入角色：选择 ZIP 包解压到角色资产目录 */
void PageCharacter::onImportCharacter()
{
    QString zipPath = QFileDialog::getOpenFileName(
        this, "选择角色压缩包",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "ZIP 文件 (*.zip)");
    if (zipPath.isEmpty()) return;

    QString charName = QFileInfo(zipPath).completeBaseName();
    QString targetDir = QDir(CharacterAssetsPath).filePath(charName);

    if (QDir(targetDir).exists())
    {
        auto btn = QMessageBox::question(this, "角色已存在",
            QString("角色 %1 已存在，是否覆盖？").arg(charName));
        if (btn != QMessageBox::Yes) return;
    }

    QString err;
    if (!extractZip(zipPath, targetDir, &err))
    {
        QMessageBox::warning(this, "导入失败", err);
        return;
    }

    refreshCharList();
    ui->comboChar->setCurrentText(charName);
    QMessageBox::information(this, "导入成功", QString("角色 %1 已导入").arg(charName));
}

/* 删除选中角色 */
void PageCharacter::onDeleteCharacter()
{
    QString name = ui->comboChar->currentText();
    if (name.isEmpty()) return;

    auto btn = QMessageBox::question(this, "确认删除",
        QString("确定要删除角色 %1 吗？此操作不可恢复。").arg(name));
    if (btn != QMessageBox::Yes) return;

    QDir dir(QDir(CharacterAssetsPath).filePath(name));
    if (!dir.exists() || !dir.removeRecursively())
    {
        QMessageBox::warning(this, "删除失败", "无法删除角色目录");
        return;
    }
    refreshCharList();
    loadCurrentConfig();
}

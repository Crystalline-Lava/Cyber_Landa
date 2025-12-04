#include "LogBrowser.h"
#include "ui_LogBrowser.h"

#include <QHeaderView>
#include <QDateTime>

LogBrowser::LogBrowser(rove::data::LogManager& manager, QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::LogBrowser>()), m_manager(manager) {
    ui->setupUi(this);
    m_table = ui->logTable;
    ui->filterCombo->addItems({QStringLiteral("全部"), QStringLiteral("自动"), QStringLiteral("手动"),
                               QStringLiteral("里程碑"), QStringLiteral("事件")});
    connect(ui->filterCombo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &LogBrowser::onFilterChanged);
    reload();
}

LogBrowser::~LogBrowser() = default;

void LogBrowser::reload() {
    populateTable(m_manager.filterLogs(std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt));
}

void LogBrowser::onFilterChanged(int index) {
    const auto logs = m_manager.filterLogs(std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt);
    std::vector<rove::data::LogEntry> filtered;
    for (const auto& log : logs) {
        if (index == 0) {
            filtered.push_back(log);
        } else if (index == 1 && log.type() == rove::data::LogEntry::LogType::Auto) {
            filtered.push_back(log);
        } else if (index == 2 && log.type() == rove::data::LogEntry::LogType::Manual) {
            filtered.push_back(log);
        } else if (index == 3 && log.type() == rove::data::LogEntry::LogType::Milestone) {
            filtered.push_back(log);
        } else if (index == 4 && log.type() == rove::data::LogEntry::LogType::Event) {
            filtered.push_back(log);
        }
    }
    populateTable(filtered);
}

/**
 * @brief 将日志数据填充到表格，列自适应宽度。
 */
void LogBrowser::populateTable(const std::vector<rove::data::LogEntry>& entries) {
    m_table->clearContents();
    m_table->setRowCount(static_cast<int>(entries.size()));
    int row = 0;
    for (const auto& entry : entries) {
        QString typeText;
        switch (entry.type()) {
        case rove::data::LogEntry::LogType::Auto:
            typeText = QStringLiteral("自动");
            break;
        case rove::data::LogEntry::LogType::Manual:
            typeText = QStringLiteral("手动");
            break;
        case rove::data::LogEntry::LogType::Milestone:
            typeText = QStringLiteral("里程碑");
            break;
        case rove::data::LogEntry::LogType::Event:
            typeText = QStringLiteral("事件");
            break;
        default:
            typeText = QStringLiteral("其他");
        }
        m_table->setItem(row, 0, new QTableWidgetItem(entry.timestamp().toString(Qt::ISODate)));
        m_table->setItem(row, 1, new QTableWidgetItem(typeText));
        m_table->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(entry.content())));
        ++row;
    }
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}



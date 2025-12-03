#include "LogBrowser.h"
#include "ui_LogBrowser.h"

#include <QHeaderView>
#include <QDateTime>

LogBrowser::LogBrowser(rove::logging::LogManager& manager, QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::LogBrowser>()), m_manager(manager) {
    ui->setupUi(this);
    m_table = ui->logTable;
    ui->filterCombo->addItems({QStringLiteral("全部"), QStringLiteral("任务"), QStringLiteral("成就"),
                               QStringLiteral("商店")});
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
    std::vector<rove::logging::LogEntry> filtered;
    for (const auto& log : logs) {
        if (index == 0) {
            filtered.push_back(log);
        } else if (index == 1 && log.type() == rove::logging::LogEntry::LogType::Task) {
            filtered.push_back(log);
        } else if (index == 2 && log.type() == rove::logging::LogEntry::LogType::Achievement) {
            filtered.push_back(log);
        } else if (index == 3 && log.type() == rove::logging::LogEntry::LogType::Shop) {
            filtered.push_back(log);
        }
    }
    populateTable(filtered);
}

/**
 * @brief 将日志数据填充到表格，列自适应宽度。
 */
void LogBrowser::populateTable(const std::vector<rove::logging::LogEntry>& entries) {
    m_table->clearContents();
    m_table->setRowCount(static_cast<int>(entries.size()));
    int row = 0;
    for (const auto& entry : entries) {
        QString typeText;
        switch (entry.type()) {
        case rove::logging::LogEntry::LogType::Task:
            typeText = QStringLiteral("任务");
            break;
        case rove::logging::LogEntry::LogType::Achievement:
            typeText = QStringLiteral("成就");
            break;
        case rove::logging::LogEntry::LogType::Shop:
            typeText = QStringLiteral("商店");
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

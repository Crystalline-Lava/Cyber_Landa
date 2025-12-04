#ifndef LOGBROWSER_H
#define LOGBROWSER_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <optional>
#include <memory>
#include "../core/LogManager.h"

namespace Ui {
class LogBrowser;
}

/**
 * @class LogBrowser
 * @brief 日志浏览器，时间轴样式查看并支持过滤。
 * 中文说明：使用表格呈现日志，提供类型过滤与关键词搜索入口。
 */
class LogBrowser : public QWidget {
    Q_OBJECT

public:
    LogBrowser(rove::data::LogManager& manager, QWidget* parent = nullptr);
    ~LogBrowser() override;

public slots:
    /**
     * @brief 重载日志列表。
     */
    void reload();

private slots:
    /**
     * @brief 过滤条件变更时重建表格。
     */
    void onFilterChanged(int index);

private:
    /**
     * @brief 用管理器数据填充表格。
     */
    void populateTable(const std::vector<rove::data::LogEntry>& entries);

    std::unique_ptr<Ui::LogBrowser> ui;
    rove::data::LogManager& m_manager;
    QTableWidget* m_table{nullptr};
};

#endif  // LOGBROWSER_H



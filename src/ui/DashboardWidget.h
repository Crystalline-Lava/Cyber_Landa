#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QListWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QPolarChart>
#include <QtCharts/QValueAxis>
#include <memory>
#include "../core/User.h"

namespace Ui {
class DashboardWidget;
}

/**
 * @class DashboardWidget
 * @brief 展示等级、金币、近期动态和属性雷达图的仪表盘组件。
 * 中文说明：该组件作为主页概览，负责快速向学生反馈成长状态，并提供一个最近活动列表。
 */
class DashboardWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief 构造函数构建 UI 并准备雷达图。
     * @param parent 父级窗口指针。
     */
    explicit DashboardWidget(QWidget* parent = nullptr);
    ~DashboardWidget() override;

    /**
     * @brief 根据用户数据刷新展示数值与图表。
     * @param user 当前用户实体。
     */
    void renderUser(const rove::data::User& user);

    /**
     * @brief 更新最近活动列表。
     * @param activities 活动文本集合。
     */
    void setRecentActivities(const QStringList& activities);

private:
    /**
     * @brief 构建雷达图坐标轴与数据集。
     */
    void buildRadarChart();

    std::unique_ptr<Ui::DashboardWidget> ui;
    QtCharts::QPolarChart* m_polarChart{nullptr};
    QtCharts::QChartView* m_chartView{nullptr};
};

#endif  // DASHBOARDWIDGET_H

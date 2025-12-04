#ifndef GROWTHDASHBOARD_H
#define GROWTHDASHBOARD_H

#include <QWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <memory>
#include "../core/GrowthSnapshot.h"
#include "../core/GrowthVisualizer.h"
#include "../core/User.h"

namespace Ui {
class GrowthDashboard;
}

/**
 * @class GrowthDashboard
 * @brief 成长面板，包含互动雷达图与成长时间线。
 * 中文说明：该组件使用 QtCharts 展示属性雷达及成长折线，结合 GrowthVisualizer 生成数据。
 */
class GrowthDashboard : public QWidget {
    Q_OBJECT

public:
    GrowthDashboard(rove::GrowthVisualizer& visualizer, QWidget* parent = nullptr);
    ~GrowthDashboard() override;

public slots:
    /**
     * @brief 渲染指定用户的成长趋势。
     * @param user 用户实体。
     * @param snapshots 历史快照列表。
     */
    void render(const rove::data::User& user, const std::vector<rove::data::GrowthSnapshot>& snapshots);

private:
    /**
     * @brief 构建折线图表并调整坐标轴。
     */
    void buildTimeline(const std::vector<rove::data::GrowthSnapshot>& snapshots);

    /**
     * @brief 更新雷达图数据。
     */
    void updateRadar(const rove::data::User::AttributeSet& attrs);

    std::unique_ptr<Ui::GrowthDashboard> ui;
    rove::GrowthVisualizer& m_visualizer;
    QChartView* m_lineView{nullptr};
    QChartView* m_radarView{nullptr};
};

#endif  // GROWTHDASHBOARD_H




#ifndef TUTORIALMANAGER_H
#define TUTORIALMANAGER_H

#include <QObject>
#include <QMap>
#include <QStringList>
#include <algorithm>

/**
 * @class TutorialManager
 * @brief 新手引导控制器，管理步骤、奖励与跳过逻辑。
 * 中文说明：该类作为 MVC 中的控制器层，与 UI 交互发送提示、跟踪进度并发射完成信号。
 */
class TutorialManager : public QObject {
    Q_OBJECT

public:
    explicit TutorialManager(QObject* parent = nullptr);

    /**
     * @brief 获取当前步骤提示文本。
     */
    QString currentHint() const;

    /**
     * @brief 标记指定任务完成并推进教程。
     * @param key 步骤键。
     */
    void markStepDone(const QString& key);

    /**
     * @brief 查询是否全部完成。
     */
    bool isFinished() const;

    /**
     * @brief 重置教程进度。
     */
    void reset();

signals:
    /**
     * @brief 教程状态变更，便于 UI 提示。
     * @param hint 最新提示文本。
     */
    void tutorialHintChanged(const QString& hint);

    /**
     * @brief 全部完成事件。
     */
    void tutorialFinished();

    /**
     * @brief 奖励发放事件。
     * @param coins 奖励金币。
     */
    void rewardIssued(int coins);

public slots:
    /**
     * @brief 跳过教程，直接标记完成。
     */
    void skip();

private:
    void advance();

    QMap<QString, bool> m_steps;
    QStringList m_order;
    int m_currentIndex{0};
};

#endif  // TUTORIALMANAGER_H


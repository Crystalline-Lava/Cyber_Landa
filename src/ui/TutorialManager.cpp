#include "TutorialManager.h"

TutorialManager::TutorialManager(QObject* parent) : QObject(parent) {
    m_steps = {{QStringLiteral("createTask"), false},
               {QStringLiteral("firstAchievement"), false},
               {QStringLiteral("firstPurchase"), false}};
    m_order = {QStringLiteral("createTask"),
               QStringLiteral("firstAchievement"),
               QStringLiteral("firstPurchase")};
}

QString TutorialManager::currentHint() const {
    if (isFinished()) {
        return QStringLiteral("教程已完成，享受校园冒险吧！");
    }
    const QString key = m_order.value(m_currentIndex);
    if (key == QStringLiteral("createTask")) {
        return QStringLiteral("请创建你的第一个任务，体验成长点与金币奖励");
    }
    if (key == QStringLiteral("firstAchievement")) {
        return QStringLiteral("解锁首个成就，认识成就系统奖励");
    }
    return QStringLiteral("在商店完成首次购买，感受道具影响");
}

void TutorialManager::markStepDone(const QString& key) {
    if (!m_steps.contains(key) || m_steps.value(key)) {
        return;
    }
    m_steps[key] = true;
    emit rewardIssued(50);
    advance();
}

bool TutorialManager::isFinished() const { return std::all_of(m_steps.begin(), m_steps.end(), [](bool done) { return done; }); }

void TutorialManager::reset() {
    for (auto it = m_steps.begin(); it != m_steps.end(); ++it) {
        it.value() = false;
    }
    m_currentIndex = 0;
    emit tutorialHintChanged(currentHint());
}

void TutorialManager::skip() {
    for (auto it = m_steps.begin(); it != m_steps.end(); ++it) {
        it.value() = true;
    }
    m_currentIndex = m_order.size();
    emit tutorialFinished();
}

void TutorialManager::advance() {
    while (m_currentIndex < m_order.size() && m_steps.value(m_order[m_currentIndex])) {
        ++m_currentIndex;
    }
    if (isFinished()) {
        emit tutorialFinished();
    } else {
        emit tutorialHintChanged(currentHint());
    }
}

#include "GrowthSnapshot.h"

namespace rove::data {

GrowthSnapshot::GrowthSnapshot()
    : m_id(-1),
      m_timestamp(QDateTime::currentDateTime()),
      m_level(1),
      m_growthPoints(0),
      m_attributes(),
      m_achievementCount(0),
      m_completedTasks(0),
      m_failedTasks(0),
      m_manualLogCount(0) {}

GrowthSnapshot::GrowthSnapshot(int id,
                               const QDateTime& timestamp,
                               int level,
                               int growthPoints,
                               const User::AttributeSet& attributes,
                               int achievementCount,
                               int completedTasks,
                               int failedTasks,
                               int manualLogCount)
    : m_id(id),
      m_timestamp(timestamp),
      m_level(level),
      m_growthPoints(growthPoints),
      m_attributes(attributes),
      m_achievementCount(achievementCount),
      m_completedTasks(completedTasks),
      m_failedTasks(failedTasks),
      m_manualLogCount(manualLogCount) {}

int GrowthSnapshot::id() const noexcept { return m_id; }

void GrowthSnapshot::setId(int id) noexcept { m_id = id; }

const QDateTime& GrowthSnapshot::timestamp() const noexcept { return m_timestamp; }

void GrowthSnapshot::setTimestamp(const QDateTime& timestamp) noexcept { m_timestamp = timestamp; }

int GrowthSnapshot::level() const noexcept { return m_level; }

void GrowthSnapshot::setLevel(int level) noexcept { m_level = level; }

int GrowthSnapshot::growthPoints() const noexcept { return m_growthPoints; }

void GrowthSnapshot::setGrowthPoints(int points) noexcept { m_growthPoints = points; }

const User::AttributeSet& GrowthSnapshot::attributes() const noexcept { return m_attributes; }

void GrowthSnapshot::setAttributes(const User::AttributeSet& attributes) noexcept { m_attributes = attributes; }

int GrowthSnapshot::achievementCount() const noexcept { return m_achievementCount; }

void GrowthSnapshot::setAchievementCount(int count) noexcept { m_achievementCount = count; }

int GrowthSnapshot::completedTasks() const noexcept { return m_completedTasks; }

void GrowthSnapshot::setCompletedTasks(int count) noexcept { m_completedTasks = count; }

int GrowthSnapshot::failedTasks() const noexcept { return m_failedTasks; }

void GrowthSnapshot::setFailedTasks(int count) noexcept { m_failedTasks = count; }

int GrowthSnapshot::manualLogCount() const noexcept { return m_manualLogCount; }

void GrowthSnapshot::setManualLogCount(int count) noexcept { m_manualLogCount = count; }

}  // namespace rove::data

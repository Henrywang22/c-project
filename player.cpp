#include "Player.h"
#include "Weapon.h"
#include "WaveSystem.h"
#include "WeatherSystem.h"

Player::Player()
    : m_worldPos(200, 300),
    m_currentSpeed(GameConfig::SHIP_BASE_SPEED),
    m_baseSpeed(GameConfig::SHIP_BASE_SPEED),
    m_durability(100), m_stamina(GameConfig::MAX_STAMINA),
    m_isStunned(false), m_isDead(false),
    m_stunDuration(0), m_speedReduction(0),
    m_reboundActive(false),
    m_keyW(false), m_keyA(false), m_keyS(false), m_keyD(false), m_keyShift(false)
{
    coins = 0;
    fishCaught = 0;
    fishTotalValue = 0;
    distance = 0;
    gameSeconds = 0;
    visionReduced = false;
    maxDurability = 100;
    maxStamina = GameConfig::MAX_STAMINA;
}

Player& Player::instance() {
    static Player p;
    return p;
}

void Player::keyPress(QKeyEvent* e) {
    switch (e->key()) {
    case Qt::Key_W: m_keyW = true; break;
    case Qt::Key_A: m_keyA = true; break;
    case Qt::Key_S: m_keyS = true; break;
    case Qt::Key_D: m_keyD = true; break;
    case Qt::Key_Shift: m_keyShift = true; break;
    default: break;
    }
}

void Player::keyRelease(QKeyEvent* e) {
    switch (e->key()) {
    case Qt::Key_W: m_keyW = false; break;
    case Qt::Key_A: m_keyA = false; break;
    case Qt::Key_S: m_keyS = false; break;
    case Qt::Key_D: m_keyD = false; break;
    case Qt::Key_Shift: m_keyShift = false; break;
    default: break;
    }
}

void Player::update(qreal deltaTime) {
    if (m_isDead) return;

    if (m_isStunned && m_stunTimer.elapsed() >= m_stunDuration)
        m_isStunned = false;

    if (WeatherSystem::instance().shouldTriggerLightning())
        takeDurabilityDamage(GameConfig::STORM_LIGHTNING_DAMAGE);

    updateMovement(deltaTime);
    checkBorder();
    emit stateChanged();
}

void Player::updateMovement(qreal deltaTime) {
    if (m_isStunned) return;

    qreal targetBaseSpeed = m_baseSpeed;
    if (m_keyShift && m_stamina > 0) {
        targetBaseSpeed = GameConfig::SHIP_BOOST_SPEED;
        m_stamina = qMax(0, m_stamina - GameConfig::BOOST_STAMINA_COST_PER_FRAME);
    }
    else if (!m_keyShift && m_stamina < maxStamina) {
        m_stamina = qMin(maxStamina, m_stamina + 1);
    }

    targetBaseSpeed *= WaveSystem::instance().currentSpeedMultiplier();
    targetBaseSpeed *= (1.0f - m_speedReduction);
    m_currentSpeed += (targetBaseSpeed - m_currentSpeed) * deltaTime * 5.0f;

    QPointF moveDir(0, 0);
    if (m_keyW) moveDir.ry() -= 1;
    if (m_keyS) moveDir.ry() += 1;
    if (m_keyA) moveDir.rx() -= 1;
    if (m_keyD) moveDir.rx() += 1;

    if (!moveDir.isNull()) {
        QVector2D dirVec(moveDir);
        dirVec.normalize();
        m_worldPos += dirVec.toPointF() * m_currentSpeed * deltaTime;
    }

    if (m_reboundActive) {
        m_worldPos += m_reboundDir * deltaTime * 200.0f;
        m_reboundActive = false;
    }

    resetSpeedReduction();
}

void Player::checkBorder() {
    if (m_worldPos.x() < 0) { m_isDead = true; emit playerDied(); }
    if (m_worldPos.y() < GameConfig::TOP_BORDER)    m_worldPos.setY(GameConfig::TOP_BORDER);
    if (m_worldPos.y() > GameConfig::BOTTOM_BORDER) m_worldPos.setY(GameConfig::BOTTOM_BORDER);
    if (m_worldPos.x() > GameConfig::RIGHT_BORDER)  m_worldPos.setX(GameConfig::RIGHT_BORDER);
}

void Player::takeDurabilityDamage(int damage) {
    m_durability = qMax(0, m_durability - damage);
    if (m_durability <= 0) { m_isDead = true; emit playerDied(); }
}

void Player::applyStun(int durationMs) {
    m_isStunned = true;
    m_stunDuration = durationMs;
    m_stunTimer.restart();
}

void Player::applyRebound(const QPointF& direction) {
    m_reboundDir = direction;
    m_reboundActive = true;
}

void Player::applySpeedReduction(qreal reduction) {
    m_speedReduction = qMax(m_speedReduction, reduction);
}

void Player::resetSpeedReduction() {
    m_speedReduction = 0;
}

// Item.cpp需要的接口实现
void Player::restoreStamina(int amount) {
    m_stamina = qMin(maxStamina, m_stamina + amount);
}

void Player::restoreDurability(int amount) {
    m_durability = qMin(maxDurability, m_durability + amount);
}

void Player::upgradeBaseSpeed(float amount) {
    m_baseSpeed += amount;
}

void Player::upgradeMaxDurability(int amount) {
    maxDurability += amount;
    m_durability += amount; // 升级时同时补满新增的耐久
}

void Player::upgradeMaxStamina(int amount) {
    maxStamina += amount;
    m_stamina += amount;
}

Weapon* Player::getCurrentWeapon() {
    return m_currentWeapon;
}

void Player::equipWeapon(Weapon* weapon) {
    m_currentWeapon = weapon;
}

void Player::saveState() {}
void Player::loadState() {}
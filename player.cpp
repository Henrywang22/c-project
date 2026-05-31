#include "Player.h"
#include "Weapon.h"
#include "WaveSystem.h"
#include "WeatherSystem.h"
#include <cmath>

using namespace Config;

// 局部常量配置
constexpr int DASH_STAMINA_COST = 20;
constexpr int POISON_TICK_DAMAGE = 20;

Player::Player()
{
    reset();
}

Player& Player::instance() {
    static Player p;
    return p;
}

void Player::reset() {
    m_worldPos = QPointF(200, 300);
    m_currentSpeed = GameConfig::SHIP_BASE_SPEED;
    m_baseSpeed = GameConfig::SHIP_BASE_SPEED;
    maxDurability = 100;
    maxStamina = GameConfig::MAX_STAMINA;
    m_durability = maxDurability;
    m_stamina = maxStamina;
    m_staminaPenalty = 0;

    m_isStunned = false;
    m_isDead = false;
    m_reboundActive = false;
    m_speedReduction = 0;
    m_keyW = m_keyA = m_keyS = m_keyD = m_keyShift = m_keySpace = false;

    // Dash 初始化
    m_isDashing = false;
    m_dashCooldownMs = 1500;
    m_dashDurationMs = 200;
    m_dashDirection = QPointF(1, 0);
    m_dashCooldown.start();

    // Shock 初始化
    m_isShockActive = false;
    m_shockCharges = 2; // 每局暂定2次救场
    m_shockCooldownMs = 5000;
    m_shockDurationMs = 800;
    m_shockCooldown.start();

    // Debuff 初始化
    m_isInputReversed = false;
    m_noRangedAttack = false;
    m_isPoisoned = false;

    coins = 0;
    fishCaught = 0;
    fishTotalValue = 0;
    distance = 0;
    gameSeconds = 0;
    visionReduced = false;
}

void Player::keyPress(QKeyEvent* e) {
    if (e->isAutoRepeat()) return;
    switch (e->key()) {
    case Qt::Key_W: m_keyW = true; break;
    case Qt::Key_A: m_keyA = true; break;
    case Qt::Key_S: m_keyS = true; break;
    case Qt::Key_D: m_keyD = true; break;
    case Qt::Key_Shift: m_keyShift = true; break;
    case Qt::Key_Space: m_keySpace = true; break;
    default: break;
    }
}

void Player::keyRelease(QKeyEvent* e) {
    if (e->isAutoRepeat()) return;
    switch (e->key()) {
    case Qt::Key_W: m_keyW = false; break;
    case Qt::Key_A: m_keyA = false; break;
    case Qt::Key_S: m_keyS = false; break;
    case Qt::Key_D: m_keyD = false; break;
    case Qt::Key_Shift: m_keyShift = false; break;
    case Qt::Key_Space: m_keySpace = false; break;
    default: break;
    }
}

void Player::update(qreal deltaTime) {
    if (m_isDead) return;

    updateDebuffs(); // 处理各种状态的倒计时

    if (m_isStunned && m_stunTimer.elapsed() >= m_stunDuration)
        m_isStunned = false;

    if (WeatherSystem::instance().shouldTriggerLightning())
        takeDurabilityDamage(GameConfig::STORM_LIGHTNING_DAMAGE);

    updateMovement(deltaTime);
    checkBorder();
    emit stateChanged();
}

void Player::updateDebuffs() {
    // Dash 倒计时
    if (m_isDashing && m_dashTimer.elapsed() >= m_dashDurationMs) {
        m_isDashing = false;
    }

    // Shock 倒计时
    if (m_isShockActive && m_shockTimer.elapsed() >= m_shockDurationMs) {
        m_isShockActive = false;
    }

    // 键位反转 倒计时
    if (m_isInputReversed && m_inputReverseTimer.elapsed() >= m_inputReverseDurationMs) {
        m_isInputReversed = false;
    }

    // 禁远程 倒计时
    if (m_noRangedAttack && m_noRangedTimer.elapsed() >= m_noRangedDurationMs) {
        m_noRangedAttack = false;
    }

    // 中毒 倒计时与伤害结算
    if (m_isPoisoned) {
        if (m_poisonTimer.elapsed() >= m_poisonDurationMs) {
            m_isPoisoned = false;
            // 中毒结束后体力上限减少10%
            applyTemporaryMaxStaminaPenalty(maxStamina * 0.1f);
        }
        else if (m_poisonTickTimer.elapsed() >= 1000) {
            takeDurabilityDamage(POISON_TICK_DAMAGE);
            m_poisonTickTimer.restart();
        }
    }
}

void Player::updateMovement(qreal deltaTime) {
    // 如果处于Dash中，无视眩晕、不受减速影响，极高速强行位移
    if (m_isDashing) {
        qreal dashSpeed = m_baseSpeed * 10.0;
        m_worldPos += m_dashDirection * dashSpeed * deltaTime;
        return;
    }

    if (m_isStunned) return;

    qreal targetBaseSpeed = m_baseSpeed;
    int currentEffectiveMaxStamina = qMax(1, maxStamina - m_staminaPenalty);

    if (m_keyShift && m_stamina > 0) {
        targetBaseSpeed = GameConfig::SHIP_BOOST_SPEED;
        m_stamina = qMax(0, m_stamina - GameConfig::BOOST_STAMINA_COST_PER_FRAME);
    }
    else if (!m_keyShift && m_stamina < currentEffectiveMaxStamina) {
        m_stamina = qMin(currentEffectiveMaxStamina, m_stamina + 1);
    }

    // 环境与中毒衰减
    targetBaseSpeed *= WaveSystem::instance().currentSpeedMultiplier();
    targetBaseSpeed *= (1.0f - m_speedReduction);
    if (m_isPoisoned) targetBaseSpeed *= 0.5f; // 中毒减速 50%

    m_currentSpeed += (targetBaseSpeed - m_currentSpeed) * deltaTime * 5.0f;

    QPointF moveDir(0, 0);
    if (m_keyW) moveDir.ry() -= 1;
    if (m_keyS) moveDir.ry() += 1;
    if (m_keyA) moveDir.rx() -= 1;
    if (m_keyD) moveDir.rx() += 1;

    // 塞壬技能：键位反转
    if (m_isInputReversed) {
        moveDir = -moveDir;
    }

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

// ==========================================
// 战斗、受伤与机制判定
// ==========================================

bool Player::canTakeDamage() const {
    // 处于冲刺中无敌，死亡后不再受击
    return !m_isDashing && !m_isDead;
}

void Player::takeDurabilityDamage(int damage) {
    if (!canTakeDamage()) return;

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

// ==========================================
// Dash 与 Shock 技能系统
// ==========================================

bool Player::canDash() const {
    if (m_isDashing || m_isStunned || m_isDead) return false;
    if (m_dashCooldown.elapsed() < m_dashCooldownMs) return false;
    if (m_stamina < DASH_STAMINA_COST) return false;
    return true;
}

bool Player::isDashing() const {
    return m_isDashing;
}

void Player::triggerDash() {
    if (!canDash()) return;
    if (!consumeStamina(DASH_STAMINA_COST)) return;

    // 获取当前移动方向，如果没动默认向右
    QPointF dir(0, 0);
    if (m_keyW) dir.ry() -= 1;
    if (m_keyS) dir.ry() += 1;
    if (m_keyA) dir.rx() -= 1;
    if (m_keyD) dir.rx() += 1;

    if (m_isInputReversed) dir = -dir;

    if (dir.isNull()) dir.rx() = 1.0;

    QVector2D norm(dir);
    norm.normalize();
    m_dashDirection = norm.toPointF();

    m_isDashing = true;
    m_dashTimer.restart();
    m_dashCooldown.restart();
}

bool Player::canShock() const {
    if (m_isShockActive || m_isDead) return false;
    if (m_shockCharges <= 0) return false;
    if (m_shockCooldown.elapsed() < m_shockCooldownMs) return false;
    return true;
}

void Player::triggerShock() {
    if (!canShock()) return;

    m_shockCharges--;
    m_isShockActive = true;
    m_shockTimer.restart();
    m_shockCooldown.restart();
}

bool Player::isShockActive() const {
    return m_isShockActive;
}

QRectF Player::shockArea() const {
    // 以玩家为中心，边长300的爆发判定范围
    return QRectF(m_worldPos.x() - 150, m_worldPos.y() - 150, 300, 300);
}

// ==========================================
// 体力管控体系
// ==========================================

bool Player::consumeStamina(int amount) {
    if (m_stamina >= amount) {
        m_stamina -= amount;
        return true;
    }
    return false;
}

void Player::restoreStaminaToFull() {
    int currentEffectiveMaxStamina = qMax(1, maxStamina - m_staminaPenalty);
    m_stamina = currentEffectiveMaxStamina;
}

void Player::applyTemporaryMaxStaminaPenalty(int amount) {
    m_staminaPenalty += amount;
    int effectiveMax = qMax(1, maxStamina - m_staminaPenalty);
    if (m_stamina > effectiveMax) {
        m_stamina = effectiveMax;
    }
}

void Player::clearMaxStaminaPenalty() {
    m_staminaPenalty = 0;
}

// ==========================================
// Debuff 体系
// ==========================================

void Player::applyInputReverse(int durationMs) {
    m_isInputReversed = true;
    m_inputReverseDurationMs = durationMs;
    m_inputReverseTimer.restart();
}

bool Player::isInputReversed() const { return m_isInputReversed; }

void Player::applyNoRangedAttack(int durationMs) {
    m_noRangedAttack = true;
    m_noRangedDurationMs = durationMs;
    m_noRangedTimer.restart();
}

bool Player::canUseRangedAttack() const { return !m_noRangedAttack; }

void Player::applyPoison(int durationMs) {
    m_isPoisoned = true;
    m_poisonDurationMs = durationMs;
    m_poisonTimer.restart();
    m_poisonTickTimer.restart();
}

bool Player::isPoisoned() const { return m_isPoisoned; }

bool Player::isMoving() const {
    return m_keyW || m_keyA || m_keyS || m_keyD;
}

bool Player::isSpaceHeld() const { return m_keySpace; }

// ==========================================
// 原有升级/道具回调
// ==========================================

void Player::restoreStamina(int amount) {
    int effectiveMax = qMax(1, maxStamina - m_staminaPenalty);
    m_stamina = qMin(effectiveMax, m_stamina + amount);
}

void Player::restoreDurability(int amount) {
    m_durability = qMin(maxDurability, m_durability + amount);
}

void Player::upgradeBaseSpeed(float amount) {
    m_baseSpeed += amount;
}

void Player::upgradeMaxDurability(int amount) {
    maxDurability += amount;
    m_durability += amount;
}

void Player::upgradeMaxStamina(int amount) {
    maxStamina += amount;
    m_stamina += amount;
}

Weapon* Player::getCurrentWeapon() { return m_currentWeapon; }
void Player::equipWeapon(Weapon* weapon) { m_currentWeapon = weapon; }
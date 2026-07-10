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
    m_reboundDurationMs = 180;
    m_speedReduction = 0;
    m_damageFlashMs = 0;
    clearInputState();
    m_facingDirection = 3;

    // Dash 初始化
    m_isDashing = false;
    m_dashCooldownMs = 1500;
    m_dashDurationMs = 180;
    m_dashDirection = QPointF(1, 0);
    m_dashCooldown.invalidate();

    // Shock 初始化
    m_isShockActive = false;
    m_shockReady = true;
    m_shockRechargeMs = 10000;
    m_shockDurationMs = 620;
    m_shockRechargeTimer.invalidate();

    // Debuff 初始化
    m_isInputReversed = false;
    m_noRangedAttack = false;
    m_isPoisoned = false;
    m_isInkBlinded = false;

    coins = 0;
    testModeInfiniteCoins = false;
    fishCaught = 0;
    fishTotalValue = 0;
    distance = 0;
    gameSeconds = 0;
    visionReduced = false;
}

void Player::restoreSavedProgress(
    int savedDistance,
    int savedDurability,
    int savedStamina,
    int savedMaxDurability,
    int savedMaxStamina,
    qreal savedBaseSpeed
) {
    maxDurability = qMax(1, savedMaxDurability);
    maxStamina = qMax(1, savedMaxStamina);

    m_baseSpeed = savedBaseSpeed > 0
        ? savedBaseSpeed
        : GameConfig::SHIP_BASE_SPEED;
    m_currentSpeed = m_baseSpeed;

    m_durability = qBound(1, savedDurability, maxDurability);
    m_stamina = qBound(0, savedStamina, maxStamina);
    m_staminaPenalty = 0;

    int clampedDistance = qBound(0, savedDistance, GameConfig::RIGHT_BORDER);
    m_worldPos = QPointF(clampedDistance, 300);
    distance = clampedDistance;

    m_isDead = false;
    m_isStunned = false;
    m_reboundActive = false;
    m_reboundDurationMs = 180;
    m_speedReduction = 0;
    m_damageFlashMs = 0;
    m_isDashing = false;
    m_dashCooldown.invalidate();
    m_isShockActive = false;
    m_shockReady = true;
    m_shockRechargeTimer.invalidate();
    m_isInputReversed = false;
    m_noRangedAttack = false;
    m_isPoisoned = false;
    m_isInkBlinded = false;
    visionReduced = false;
    m_facingDirection = 3;
    clearInputState();
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

void Player::clearInputState() {
    m_keyW = m_keyA = m_keyS = m_keyD = m_keyShift = m_keySpace = false;
}

void Player::setWorldPos(const QPointF& pos) {
    m_worldPos = pos;
    checkBorder();
}

void Player::update(qreal deltaTime) {
    if (m_isDead) return;

    updateDebuffs(); // 处理各种状态的倒计时

    if (m_isStunned && m_stunTimer.elapsed() >= m_stunDuration)
        m_isStunned = false;
    m_damageFlashMs = qMax(0, m_damageFlashMs - qRound(deltaTime * 1000.0));

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
    if (!m_shockReady && m_shockRechargeTimer.isValid() &&
        m_shockRechargeTimer.elapsed() >= m_shockRechargeMs) {
        m_shockReady = true;
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
            m_poisonTickTimer.invalidate();
        }
        else if (m_poisonTickTimer.elapsed() >= 1000) {
            takeDurabilityDamage(POISON_TICK_DAMAGE);
            m_poisonTickTimer.restart();
        }
    }

    if (m_isInkBlinded) {
        if (m_inkBlindTimer.elapsed() >= m_inkBlindDurationMs) {
            m_isInkBlinded = false;
            visionReduced = false;
        }
        else {
            visionReduced = true;
            applySpeedReduction(0.28);
        }
    }
}

void Player::updateMovement(qreal deltaTime) {
    // 如果处于Dash中，无视眩晕、不受减速影响，极高速强行位移
    if (m_isDashing) {
        const qreal extraFromShipSpeed =
            qBound<qreal>(0.0, (m_baseSpeed - Config::GameConfig::SHIP_BASE_SPEED) * 0.7, 120.0);
        qreal dashSpeed = 680.0 + extraFromShipSpeed;
        m_worldPos += m_dashDirection * dashSpeed * deltaTime;
        return;
    }

    if (m_reboundActive) {
        m_worldPos += m_reboundDir * deltaTime * 200.0f;
        if (m_reboundTimer.elapsed() >= m_reboundDurationMs) {
            m_reboundActive = false;
        }
        return;
    }

    if (m_isStunned) return;

    qreal targetBaseSpeed = m_baseSpeed;
    int currentEffectiveMaxStamina = qMax(1, maxStamina - m_staminaPenalty);
    const bool hasMovementInput = m_keyW || m_keyA || m_keyS || m_keyD;

    if (m_keyShift && hasMovementInput && m_stamina > 0) {
        targetBaseSpeed = qMax(
            GameConfig::SHIP_BOOST_SPEED,
            m_baseSpeed * GameConfig::SHIP_BOOST_MULTIPLIER
        );
        m_stamina = qMax(0, m_stamina - GameConfig::BOOST_STAMINA_COST_PER_FRAME);
    }
    else if (m_stamina < currentEffectiveMaxStamina) {
        m_stamina = qMin(currentEffectiveMaxStamina, m_stamina + 1);
    }

    // 环境与中毒衰减
    QPointF waveMoveDir(0, 0);
    if (m_keyW) waveMoveDir.ry() -= 1;
    if (m_keyS) waveMoveDir.ry() += 1;
    if (m_keyA) waveMoveDir.rx() -= 1;
    if (m_keyD) waveMoveDir.rx() += 1;
    if (m_isInputReversed) {
        waveMoveDir = -waveMoveDir;
    }
    targetBaseSpeed *= WaveSystem::instance().currentSpeedMultiplierForMovement(waveMoveDir);
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
        if (std::abs(moveDir.x()) >= std::abs(moveDir.y())) {
            m_facingDirection = moveDir.x() < 0 ? 2 : 3;
        }
        else {
            m_facingDirection = moveDir.y() < 0 ? 0 : 1;
        }

        QVector2D dirVec(moveDir);
        dirVec.normalize();
        m_worldPos += dirVec.toPointF() * m_currentSpeed * deltaTime;
    }

    if (WaveSystem::instance().isWaveActive()) {
        const qreal driftDir = WaveSystem::instance().currentDirection() == WaveDirection::RIGHT ? 1.0 : -1.0;
        const qreal driftStrength = 42.0 * qAbs(WaveSystem::instance().currentSpeedMultiplier() - 1.0);
        m_worldPos.rx() += driftDir * driftStrength * deltaTime;
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
    m_damageFlashMs = 360;
    if (m_durability <= 0) { m_isDead = true; emit playerDied(); }
}

qreal Player::damageFlashRatio() const
{
    return qBound(0.0, static_cast<qreal>(m_damageFlashMs) / 360.0, 1.0);
}

void Player::applyStun(int durationMs) {
    m_isStunned = true;
    m_stunDuration = durationMs;
    m_stunTimer.restart();
}

void Player::applyRebound(const QPointF& direction) {
    m_reboundDir = direction;
    m_reboundActive = true;
    m_reboundTimer.restart();
}

void Player::applySpeedReduction(qreal reduction) {
    m_speedReduction = qMax(m_speedReduction, reduction);
}

void Player::resetSpeedReduction() {
    m_speedReduction = 0;
}

void Player::applyInkBlind(int durationMs)
{
    m_isInkBlinded = true;
    m_inkBlindDurationMs = qMax(1, durationMs);
    m_inkBlindTimer.restart();
    visionReduced = true;
}

// ==========================================
// Dash 与 Shock 技能系统
// ==========================================

bool Player::canDash() const {
    if (m_isDashing || m_isStunned || m_isDead) return false;
    if (m_dashCooldown.isValid() && m_dashCooldown.elapsed() < m_dashCooldownMs) return false;
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
    if (std::abs(dir.x()) >= std::abs(dir.y())) {
        m_facingDirection = dir.x() < 0 ? 2 : 3;
    }
    else {
        m_facingDirection = dir.y() < 0 ? 0 : 1;
    }

    QVector2D norm(dir);
    norm.normalize();
    m_dashDirection = norm.toPointF();

    m_isDashing = true;
    m_dashTimer.restart();
    m_dashCooldown.restart();
}

bool Player::canShock() const {
    return m_shockReady && !m_isShockActive && !m_isDead && !m_isStunned;
}

void Player::triggerShock() {
    if (!canShock()) return;

    m_shockReady = false;
    m_isShockActive = true;
    m_shockTimer.restart();
    m_shockRechargeTimer.restart();
}

bool Player::isShockActive() const {
    return m_isShockActive;
}

qreal Player::shockChargeRatio() const {
    if (m_shockReady) return 1.0;
    if (!m_shockRechargeTimer.isValid() || m_shockRechargeMs <= 0) return 0.0;
    return qBound<qreal>(0.0,
        static_cast<qreal>(m_shockRechargeTimer.elapsed()) / m_shockRechargeMs,
        1.0);
}

qreal Player::shockEffectProgress() const {
    if (!m_isShockActive || !m_shockTimer.isValid() || m_shockDurationMs <= 0) {
        return 0.0;
    }
    return qBound<qreal>(0.0,
        static_cast<qreal>(m_shockTimer.elapsed()) / m_shockDurationMs,
        1.0);
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
    m_staminaPenalty = qBound(0, m_staminaPenalty + qMax(0, amount), maxStamina - 1);
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

int Player::inputReverseRemainingMs() const
{
    if (!m_isInputReversed || !m_inputReverseTimer.isValid()) {
        return 0;
    }
    return qMax(0, m_inputReverseDurationMs - static_cast<int>(m_inputReverseTimer.elapsed()));
}

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
bool Player::isBoosting() const { return isMoving() && m_keyShift && m_stamina > 0; }
int Player::facingDirection() const { return m_facingDirection; }

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

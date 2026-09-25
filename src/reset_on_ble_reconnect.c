/*
 * Copyright (c) 2026 buwon
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/* hid.c / endpoints.c / keymap.c are only compiled for non-split boards or the
 * split central (see app/CMakeLists.txt) — the peripheral doesn't have them, and
 * only the central talks to the host anyway, so restrict this listener the same way. */
#if IS_ENABLED(CONFIG_ZMK_BLE) && (!IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))

#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* 호스트와의 활성 BLE 프로필이 끊겼다가 재연결되는 순간, 연결이 끊긴 동안
 * release가 호스트로 전달되지 못해 눌린 채로 고착될 수 있는 키/클릭 상태와
 * 자동 전환된 레이어를 강제로 초기화한다.
 *
 * 재연결 콜백이 뜨는 시점에는 아직 BLE 암호화(security)가 재협상되기 전이라
 * hog.c의 notify가 -EPERM으로 조용히 실패하고 재시도 없이 버려질 수 있다
 * (재본딩 시 암호화가 끝날 때까지 걸리는 시간은 호스트마다 들쭉날쭉하다).
 * 그러면 정작 고착을 풀어야 할 클리어 리포트 자체가 유실돼서 간헐적으로
 * 초기화가 씹히는 것처럼 보인다. 암호화가 자리 잡을 시간을 벌기 위해
 * 초기화를 살짝 지연시키고, 첫 시도가 그 사이에도 실패했을 경우를 대비해
 * 한 번 더 재시도한다. */
static void do_reset_state(struct k_work *work) {
    if (!zmk_ble_active_profile_is_connected()) {
        return;
    }

    LOG_INF("Active BLE profile reconnected: resetting HID state and layers");

    zmk_keymap_layer_to(zmk_keymap_layer_default(), false);
    zmk_endpoints_clear_current();

#if IS_ENABLED(CONFIG_ZMK_POINTING)
    zmk_endpoints_send_mouse_report();
#endif
}

static K_WORK_DELAYABLE_DEFINE(reset_state_work, do_reset_state);
static K_WORK_DELAYABLE_DEFINE(reset_state_retry_work, do_reset_state);

static int reset_state_on_ble_reconnect(const zmk_event_t *eh) {
    if (!zmk_ble_active_profile_is_connected()) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    k_work_schedule(&reset_state_work, K_MSEC(150));
    k_work_schedule(&reset_state_retry_work, K_MSEC(600));

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(reset_state_on_ble_reconnect, reset_state_on_ble_reconnect);
ZMK_SUBSCRIPTION(reset_state_on_ble_reconnect, zmk_ble_active_profile_changed);

#endif // IS_ENABLED(CONFIG_ZMK_BLE) && (!IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))

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
 * 자동 전환된 레이어를 강제로 초기화한다. */
static int reset_state_on_ble_reconnect(const zmk_event_t *eh) {
    if (!zmk_ble_active_profile_is_connected()) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    LOG_INF("Active BLE profile reconnected: resetting HID state and layers");

    zmk_keymap_layer_to(zmk_keymap_layer_default(), false);
    zmk_endpoints_clear_current();

#if IS_ENABLED(CONFIG_ZMK_POINTING)
    zmk_endpoints_send_mouse_report();
#endif

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(reset_state_on_ble_reconnect, reset_state_on_ble_reconnect);
ZMK_SUBSCRIPTION(reset_state_on_ble_reconnect, zmk_ble_active_profile_changed);

#endif // IS_ENABLED(CONFIG_ZMK_BLE) && (!IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL))

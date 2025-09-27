/**
 * @file power.c
 * @author Daniel Quadros
 * @brief Power management
 * @version 0.1
 * @date 2025-09-05
 * 
 * @copyright Copyright (c) 2025
 * 
 * Adapted from // https://github.com/peterharperuk/pico-examples/blob/powman/powman/powman_timer/powman_timer.c
 */

 #include <stdio.h>
#include "pico/stdlib.h"
#include "pico/sync.h"
#include "hardware/powman.h"
#include "hardware/structs/usb.h"

#include "FinalCountdown.h"

static powman_power_state off_state;
static powman_power_state on_state;

static void disable_usb() {
    usb_hw->phy_direct = USB_USBPHY_DIRECT_TX_PD_BITS | USB_USBPHY_DIRECT_RX_PD_BITS | USB_USBPHY_DIRECT_DM_PULLDN_EN_BITS | USB_USBPHY_DIRECT_DP_PULLDN_EN_BITS;
    usb_hw->phy_direct_override = USB_USBPHY_DIRECT_RX_DM_BITS | USB_USBPHY_DIRECT_RX_DP_BITS | USB_USBPHY_DIRECT_RX_DD_BITS |
        USB_USBPHY_DIRECT_OVERRIDE_TX_DIFFMODE_OVERRIDE_EN_BITS | USB_USBPHY_DIRECT_OVERRIDE_DM_PULLUP_OVERRIDE_EN_BITS | USB_USBPHY_DIRECT_OVERRIDE_TX_FSSLEW_OVERRIDE_EN_BITS |
        USB_USBPHY_DIRECT_OVERRIDE_TX_PD_OVERRIDE_EN_BITS | USB_USBPHY_DIRECT_OVERRIDE_RX_PD_OVERRIDE_EN_BITS | USB_USBPHY_DIRECT_OVERRIDE_TX_DM_OVERRIDE_EN_BITS |
        USB_USBPHY_DIRECT_OVERRIDE_TX_DP_OVERRIDE_EN_BITS | USB_USBPHY_DIRECT_OVERRIDE_TX_DM_OE_OVERRIDE_EN_BITS | USB_USBPHY_DIRECT_OVERRIDE_TX_DP_OE_OVERRIDE_EN_BITS |
        USB_USBPHY_DIRECT_OVERRIDE_DM_PULLDN_EN_OVERRIDE_EN_BITS | USB_USBPHY_DIRECT_OVERRIDE_DP_PULLDN_EN_OVERRIDE_EN_BITS | USB_USBPHY_DIRECT_OVERRIDE_DP_PULLUP_EN_OVERRIDE_EN_BITS |
        USB_USBPHY_DIRECT_OVERRIDE_DM_PULLUP_HISEL_OVERRIDE_EN_BITS | USB_USBPHY_DIRECT_OVERRIDE_DP_PULLUP_HISEL_OVERRIDE_EN_BITS;
}

// Iniciação do powman
void powman_init(uint64_t abs_time_ms) {
    // start powman and set the time
    powman_timer_start();
    powman_timer_set_ms(abs_time_ms);

    // Allow power down when debugger connected
    powman_set_debug_power_request_ignored(true);

    // Power states
    powman_power_state P1_7 = POWMAN_POWER_STATE_NONE;
    powman_power_state P0_0 = POWMAN_POWER_STATE_NONE;
    P0_0 = powman_power_state_with_domain_on(P0_0, POWMAN_POWER_DOMAIN_SWITCHED_CORE);
    P0_0 = powman_power_state_with_domain_on(P0_0, POWMAN_POWER_DOMAIN_XIP_CACHE);
    P0_0 = powman_power_state_with_domain_on(P0_0, POWMAN_POWER_DOMAIN_SRAM_BANK0);
    P0_0 = powman_power_state_with_domain_on(P0_0, POWMAN_POWER_DOMAIN_SRAM_BANK1);

    off_state = P1_7;
    on_state = P0_0;
}

// Coloca o RP2350 no modo de economia de energia
static int powman_off(void) {
    // Get ready to power off
    stdio_flush();
    disable_usb();

    // Set power states
    bool valid_state = powman_configure_wakeup_state(off_state, on_state);
    if (!valid_state) {
        return PICO_ERROR_INVALID_STATE;
    }

    // reboot to main
    powman_hw->boot[0] = 0;
    powman_hw->boot[1] = 0;
    powman_hw->boot[2] = 0;
    powman_hw->boot[3] = 0;

    // Switch to required power state
    int rc = powman_set_power_state(off_state);
    if (rc != PICO_OK) {
        return rc;
    }

    // Power down
    while (true) __wfi();
}

// Power off until an absolute time
int powman_off_until_time(uint64_t abs_time_ms) {
    // Start powman timer and turn off
    printf("Powering off for %llu ms\n", powman_timer_get_ms() - abs_time_ms);
    powman_enable_alarm_wakeup_at_ms(abs_time_ms);
    return powman_off();
}
// Power off for a number of milliseconds
int powman_off_for_ms(uint64_t duration_ms) {
    printf("Powering off for %llu ms\n", duration_ms);
    uint64_t ms = powman_timer_get_ms();
    return powman_off_until_time(ms + duration_ms);
}


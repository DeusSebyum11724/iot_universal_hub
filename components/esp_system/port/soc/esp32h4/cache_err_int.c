/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 The cache has an interrupt that can be raised as soon as an access to a cached
 region (flash) is done without the cache being enabled. We use that here
 to panic the CPU, which from a debugging perspective is better than grabbing bad
 data from the bus.
*/
#include "esp_rom_sys.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "soc/periph_defs.h"
#include "riscv/interrupt.h"
#include "hal/cache_ll.h"
#include "esp_private/cache_err_int.h"

// TODO: [ESP32H4] IDF-12288 inherited from verification branch, need check

static const char *TAG = "CACHE_ERR";

const char cache_error_msg[] = "Cache access error";

void esp_cache_err_get_panic_info(esp_cache_err_info_t *err_info)
{
    if (err_info == NULL) {
        return;
    }
    const uint32_t access_err_status = cache_ll_l1_get_access_error_intr_status(0, CACHE_LL_L1_ACCESS_EVENT_MASK);

    /* Return the error string if a cache error is active */
    err_info->err_str = access_err_status ? cache_error_msg : NULL;
}

bool esp_cache_err_has_active_err(void)
{
    return cache_ll_l1_get_access_error_intr_status(0, CACHE_LL_L1_ACCESS_EVENT_MASK);
}

void esp_cache_err_int_init(void)
{
    const uint32_t core_id = 0;

    /* Disable cache interrupts if enabled. */
    ESP_INTR_DISABLE(ETS_CACHEERR_INUM);

    /**
     *  Bind all cache errors to ETS_CACHEERR_INUM interrupt. we will deal with
     * them in handler by different types
     *
     * On ESP32H4 boards, the cache is a shared one but buses are still
     * distinct. So, we have an bus0 and a bus1 sharing the same cache.
     * This error can occur if a bus performs a request but the cache
     * is disabled.
     */
    esp_rom_route_intr_matrix(core_id, ETS_CACHE_INTR_SOURCE, ETS_CACHEERR_INUM);

    /* Set the type and priority to cache error interrupts. */
    esprv_int_set_type(ETS_CACHEERR_INUM, INTR_TYPE_LEVEL);
    esprv_int_set_priority(ETS_CACHEERR_INUM, SOC_INTERRUPT_LEVEL_MEDIUM);

    ESP_DRAM_LOGV(TAG, "access error intr clr & ena mask is: 0x%x", CACHE_LL_L1_ACCESS_EVENT_MASK);
    /* On the hardware side, start by clearing all the bits responsible for cache access error */
    cache_ll_l1_clear_access_error_intr(0, CACHE_LL_L1_ACCESS_EVENT_MASK);
    /* Then enable cache access error interrupts. */
    cache_ll_l1_enable_access_error_intr(0, CACHE_LL_L1_ACCESS_EVENT_MASK);

    /* Enable the interrupts for cache error. */
    ESP_INTR_ENABLE(ETS_CACHEERR_INUM);
}

int esp_cache_err_get_cpuid(void)
{
    // TODO
    return 0;
}

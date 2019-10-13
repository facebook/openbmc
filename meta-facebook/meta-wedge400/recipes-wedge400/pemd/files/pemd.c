/*
 * Copyright 2019-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <openbmc/log.h>
#include <openbmc/pal.h>
#include <facebook/wedge400-pem.h>

// #define DEBUG

#define MIN_POLL_INTERVAL 2
#define PEM_CHECK_RETRY_CNT 10

/* refer to pal.c check pem num & sensors num */
#define PEM_NUM 2
#define PEM_THRESHOLD_NUM PEM1_SENSOR_TEMP3
#define PEM_DISCRETE_NUM (PEM1_SENSOR_CNT - PEM1_SENSOR_TEMP3)
#define PEM_SENSOR_NUM PEM1_SENSOR_CNT
#define PEM_FAN_NUM 2

#define PEM_SENSOR_NAME_LEN_MAX 32

/* 
 * PEM_HANDLER_NORMAL : sensor normal
 * PEM_HANDLER_ARCHIVE : archive ltc4282 & max6615 regs to /mnt/data
 * PEM_HANDLER_RESET : reset COMe & TH3
 * PEM_HANDLER_REBOOT : reboot ltc4282
 * PEM_HANDLER_SHUTDOWN : shutdown COMe & TH3
 */
enum pem_handler_t {
    PEM_HANDLER_NORMAL = 0,
    PEM_HANDLER_ARCHIVE = (1 << 0),
    PEM_HANDLER_RESET = (1 << 1),
    PEM_HANDLER_REBOOT = (1 << 2),
    PEM_HANDLER_SHUTDOWN = (1 << 3),
};

/* 
 * Assume normal is 0
 * normal to fault is 1
 * fault to normal is 2
 * fault to fault is 3
 */
enum bool_t {
    BOOL_NORMAL = 0,
    BOOL_TO_FAULT,
    BOOL_TO_NORMAL,
    BOOL_FAULT,
};

enum boot_status_t {
    NORMAL_LOW = 0,
    NORMAL_HIGH,
};

/*
 * threshold type values of sensor
 */
enum sensor_threshold_t {
    SENSOR_UCR_THRESH,
    SENSOR_UNC_THRESH,
    SENSOR_LCR_THRESH,
    SENSOR_LNC_THRESH,
    MAX_THRESH_CNT,
};

/*
 * sensor's status type
 * SENSOR_NORMAL: sensor normal
 * SENSOR_LNC: lower non-critical
 * SENSOR_LCR: lower critical
 * SENSOR_UNC: upper ono-critical
 * SENSOR_UCR: upper critical
 */
enum sensor_status_t {
    SENSOR_NORMAL,
    SENSOR_LNC = (1 << 0),
    SENSOR_LCR = (1 << 1),
    SENSOR_UNC = (1 << 2),
    SENSOR_UCR = (1 << 3),
};

/* 
 * value : sensor current value
 * ucr : sensor up to critical value need to log & do some handler
 * unc : sensor up to the value just need to log
 * lcr : sensor below critical value need to log & do some handler
 * lnc : sensor below the value just need to log
 */
struct threshold_sensor_t {
    char sensor_name[PEM_SENSOR_NAME_LEN_MAX];
    int index;
    float value;
    float threshold[MAX_THRESH_CNT];
    int threshold_cnt[MAX_THRESH_CNT];
    int fail_flag;
};

/* 
 * value : sensor current value
 * normal : discrete sensor normal value
 */
struct bool_sensor_t {
    char sensor_name[PEM_SENSOR_NAME_LEN_MAX];
    int index;
    int value;
    int last_value;
    int normal;
};

struct pem_t {
    int fan_fail_cnt;
    struct threshold_sensor_t sensors[PEM_THRESHOLD_NUM];
    struct bool_sensor_t discrete[PEM_DISCRETE_NUM];
};

/* 
 * check current sensor status
 */
static inline int check_threshold_sensor_status(struct threshold_sensor_t *sensor) {
    if((sensor->threshold[SENSOR_UCR_THRESH] != 0) && 
       (sensor->value >= sensor->threshold[SENSOR_UCR_THRESH])) {
        sensor->threshold_cnt[SENSOR_UCR_THRESH]++;
        if (sensor->threshold_cnt[SENSOR_UCR_THRESH] >= PEM_CHECK_RETRY_CNT) {
            sensor->threshold_cnt[SENSOR_UCR_THRESH] = PEM_CHECK_RETRY_CNT;
            return SENSOR_UCR;
        }
        if((sensor->threshold[SENSOR_UNC_THRESH] != 0) && 
           (sensor->value >= sensor->threshold[SENSOR_UNC_THRESH])) {
            sensor->threshold_cnt[SENSOR_UNC_THRESH]++;
            if (sensor->threshold_cnt[SENSOR_UNC_THRESH] >= PEM_CHECK_RETRY_CNT) {
                sensor->threshold_cnt[SENSOR_UNC_THRESH] = PEM_CHECK_RETRY_CNT;
                return SENSOR_UNC;
            }
        }
        goto exit;
    } else if((sensor->threshold[SENSOR_UNC_THRESH] != 0) && 
              (sensor->value >= sensor->threshold[SENSOR_UNC_THRESH])) {
        sensor->threshold_cnt[SENSOR_UNC_THRESH]++;
        if (sensor->threshold_cnt[SENSOR_UNC_THRESH] >= PEM_CHECK_RETRY_CNT) {
            sensor->threshold_cnt[SENSOR_UNC_THRESH] = PEM_CHECK_RETRY_CNT;
            return SENSOR_UNC;
        }
        goto exit;
    }
    if((sensor->threshold[SENSOR_LCR_THRESH] != 0) && 
       (sensor->value <= sensor->threshold[SENSOR_LCR_THRESH])) {
        sensor->threshold_cnt[SENSOR_LCR_THRESH]++;
        if (sensor->threshold_cnt[SENSOR_LCR_THRESH] >= PEM_CHECK_RETRY_CNT) {
            sensor->threshold_cnt[SENSOR_LCR_THRESH] = PEM_CHECK_RETRY_CNT;
            return SENSOR_LCR;
        }
        if((sensor->threshold[SENSOR_LNC_THRESH] != 0) && 
           (sensor->value <= sensor->threshold[SENSOR_LNC_THRESH])) {
            sensor->threshold_cnt[SENSOR_LNC_THRESH]++;
            if (sensor->threshold_cnt[SENSOR_LNC_THRESH] >= PEM_CHECK_RETRY_CNT) {
                sensor->threshold_cnt[SENSOR_LNC_THRESH] = PEM_CHECK_RETRY_CNT;
                return SENSOR_LNC;
            }
        }
        goto exit;
    } else if((sensor->threshold[SENSOR_LNC_THRESH] != 0) && 
              (sensor->value <= sensor->threshold[SENSOR_LNC_THRESH])) {
        sensor->threshold_cnt[SENSOR_LNC_THRESH]++;
        if (sensor->threshold_cnt[SENSOR_LNC_THRESH] >= PEM_CHECK_RETRY_CNT) {
            sensor->threshold_cnt[SENSOR_LNC_THRESH] = PEM_CHECK_RETRY_CNT;
            return SENSOR_LNC;
        }
        goto exit;
    }

    /* If value out of range interrupted, clear counter */
    sensor->threshold_cnt[SENSOR_UCR_THRESH] = 0;
    sensor->threshold_cnt[SENSOR_UNC_THRESH] = 0;
    sensor->threshold_cnt[SENSOR_LCR_THRESH] = 0;
    sensor->threshold_cnt[SENSOR_LNC_THRESH] = 0;

exit:
    return SENSOR_NORMAL;
}

static inline int check_bool_fail(struct bool_sensor_t *sensor) {
    return sensor->value != sensor->normal;
}

/* 
 * init pem sensors' threshold value or normal value of discrete sensors
 */
static void init_pem_sensors(uint8_t pem_num, struct pem_t *pem) {
    for(int i = 0; i < PEM_THRESHOLD_NUM; i++) {
        pem->sensors[i].index = pem_num * PEM_SENSOR_NUM + PEM1_SENSOR_IN_VOLT + i;
        memset(pem->sensors[i].sensor_name, 0, sizeof(pem->sensors[i].sensor_name)/sizeof(char));
        pal_get_sensor_name(pem_num + FRU_PEM1, pem->sensors[i].index, pem->sensors[i].sensor_name);
        pal_get_sensor_threshold(pem_num + FRU_PEM1, pem->sensors[i].index,
                                 UCR_THRESH, &pem->sensors[i].threshold[SENSOR_UCR_THRESH]);
        pal_get_sensor_threshold(pem_num + FRU_PEM1, pem->sensors[i].index,
                                 UNC_THRESH, &pem->sensors[i].threshold[SENSOR_UNC_THRESH]);
        pal_get_sensor_threshold(pem_num + FRU_PEM1, pem->sensors[i].index,
                                 LCR_THRESH, &pem->sensors[i].threshold[SENSOR_LCR_THRESH]);
        pal_get_sensor_threshold(pem_num + FRU_PEM1, pem->sensors[i].index,
                                 LNC_THRESH, &pem->sensors[i].threshold[SENSOR_LNC_THRESH]);
        for(int type = SENSOR_UCR_THRESH; type <= SENSOR_LNC_THRESH; type++) {
            pem->sensors[i].threshold_cnt[type] = 0;
        }
        pem->sensors[i].fail_flag = SENSOR_NORMAL;
#ifdef DEBUG
        OBMC_INFO("[debug] PEM %d threshold sensor index %d %s init with [ucr: %0.2f, cnt: %d] " \
                   "[unc: %0.2f, cnt: %d] [lcr: %0.2f, cnt: %d] [lnc: %0.2f, cnt: %d]", pem_num + 1, 
                   pem->sensors[i].index, pem->sensors[i].sensor_name,
                   pem->sensors[i].threshold[SENSOR_UCR_THRESH], pem->sensors[i].threshold_cnt[SENSOR_UCR_THRESH],
                   pem->sensors[i].threshold[SENSOR_UNC_THRESH], pem->sensors[i].threshold_cnt[SENSOR_UNC_THRESH],
                   pem->sensors[i].threshold[SENSOR_LCR_THRESH], pem->sensors[i].threshold_cnt[SENSOR_LCR_THRESH],
                   pem->sensors[i].threshold[SENSOR_LNC_THRESH], pem->sensors[i].threshold_cnt[SENSOR_LNC_THRESH]);
#endif
    }
    for(int i = 0; i < PEM_DISCRETE_NUM; i++) {
        int normal;
        pem->discrete[i].index = pem_num * PEM_SENSOR_NUM + PEM1_SENSOR_FAULT_OV + i;
        memset(pem->discrete[i].sensor_name, 0, sizeof(pem->discrete[i].sensor_name)/sizeof(char));
        pal_get_sensor_name(pem_num + FRU_PEM1, pem->discrete[i].index, pem->discrete[i].sensor_name);
        switch(pem->discrete[i].index) {
        case PEM1_SENSOR_FAULT_FET_SHORT:
        case PEM1_SENSOR_FAULT_FET_BAD:
        case PEM2_SENSOR_FAULT_FET_SHORT:
        case PEM2_SENSOR_FAULT_FET_BAD:

        case PEM1_SENSOR_STATUS_FET_SHORT:
        case PEM1_SENSOR_STATUS_FET_BAD:
        case PEM2_SENSOR_STATUS_FET_SHORT:
        case PEM2_SENSOR_STATUS_FET_BAD:
            normal = NORMAL_LOW;
            break;
        default:
            continue;
        }
        pem->discrete[i].normal = normal;
        pem->discrete[i].last_value = normal;
#ifdef DEBUG
        OBMC_INFO("[debug] PEM %d discrete sensor index %d %s init with [normal value: %d]",
                    pem_num + 1, pem->discrete[i].index, pem->discrete[i].sensor_name,
                    pem->discrete[i].normal);
#endif
    }
}

/* 
 * dump pem sensors need to be check
 */
static int dump_pem_sensors(uint8_t pem_num, struct pem_t *pem) {
    for (int i = 0; i < PEM_THRESHOLD_NUM; i++) {
        if (pal_sensor_read_raw(pem_num + FRU_PEM1, pem->sensors[i].index,
                               &pem->sensors[i].value)) {
#ifdef DEBUG
        OBMC_ERROR(-1, "PEM %d threshold sensor_0x%02x %s read failed", pem_num + 1,
                        pem->sensors[i].index, pem->sensors[i].sensor_name);
#endif
            return -1;
        }
#ifdef DEBUG
        OBMC_INFO("[debug] PEM %d threshold sensor_0x%02x %s: %0.2f", pem_num + 1,
                    pem->sensors[i].index, pem->sensors[i].sensor_name, pem->sensors[i].value);
#endif
    }
    for (int i = 0; i < PEM_DISCRETE_NUM; i++) {
        /* always check pem fet related discrete sensors */
        switch(pem->discrete[i].index) {
            case PEM1_SENSOR_FAULT_FET_SHORT:
            case PEM1_SENSOR_FAULT_FET_BAD:
            case PEM1_SENSOR_STATUS_FET_SHORT:
            case PEM1_SENSOR_STATUS_FET_BAD:
            case PEM2_SENSOR_FAULT_FET_SHORT:
            case PEM2_SENSOR_FAULT_FET_BAD:
            case PEM2_SENSOR_STATUS_FET_SHORT:
            case PEM2_SENSOR_STATUS_FET_BAD:
                break;
            default:
                continue;
        }
        if (pal_sensor_discrete_read_raw(pem_num + FRU_PEM1, pem->discrete[i].index,
                                        &pem->discrete[i].value)) {
#ifdef DEBUG
        OBMC_ERROR(-1, "PEM %d discrete sensor_0x%02x %s read failed", pem_num + 1,
                        pem->discrete[i].index, pem->discrete[i].sensor_name);
#endif
            return -1;
        }
#ifdef DEBUG
        OBMC_INFO("[debug] PEM %d discrete sensor_0x%02x %s: %d", pem_num + 1,
                    pem->discrete[i].index, pem->discrete[i].sensor_name, pem->discrete[i].value);
#endif
    }
    return 0;
}

/* 
 * check pem threshold sensors and handle exception
 */
static int check_pem_threshold_sensors(uint8_t pem_num, struct pem_t *pem) {
    int ret = PEM_HANDLER_NORMAL;
    pem->fan_fail_cnt = 0;
    for (int i = 0; i < PEM_THRESHOLD_NUM; i++) {
        switch(check_threshold_sensor_status(&pem->sensors[i])) {
        case SENSOR_UCR:
            /* prevent duplicate log */
            if(pem->sensors[i].fail_flag & SENSOR_UCR) {
                continue;
            }
            pem->sensors[i].fail_flag |= SENSOR_UCR;
            switch(pem->sensors[i].index) {
            case PEM1_SENSOR_IN_VOLT:
            case PEM1_SENSOR_OUT_VOLT:
            case PEM1_SENSOR_POWER:
            case PEM1_SENSOR_CURR:
            case PEM1_SENSOR_FAN1_TACH:
            case PEM1_SENSOR_FAN2_TACH:
            case PEM2_SENSOR_IN_VOLT:
            case PEM2_SENSOR_OUT_VOLT:
            case PEM2_SENSOR_POWER:
            case PEM2_SENSOR_CURR:
            case PEM2_SENSOR_FAN1_TACH:
            case PEM2_SENSOR_FAN2_TACH:
                OBMC_CRIT("PEM %d sensor %s UCR Error", pem_num + 1, pem->sensors[i].sensor_name);
                break;
            case PEM1_SENSOR_TEMP1:
            case PEM2_SENSOR_TEMP1:
                ret |= (PEM_HANDLER_ARCHIVE | PEM_HANDLER_SHUTDOWN);
                OBMC_CRIT("PEM %d sensor %s (OTP) UCR Error", pem_num + 1, pem->sensors[i].sensor_name);
                break;
            case PEM1_SENSOR_TEMP2:
            case PEM1_SENSOR_TEMP3:
            case PEM2_SENSOR_TEMP2:
            case PEM2_SENSOR_TEMP3:
                OBMC_CRIT("PEM %d sensor %s UCR Error", pem_num + 1, pem->sensors[i].sensor_name);
                break;
            }
            break;
        case SENSOR_UNC:
            /* Check if UCR recovery */
            if(pem->sensors[i].fail_flag & SENSOR_UCR) {
                pem->sensors[i].fail_flag &= (~SENSOR_UCR);
                pem->sensors[i].threshold_cnt[SENSOR_UCR_THRESH] = 0;
                OBMC_INFO("PEM %d sensor %s UCR recovery", pem_num + 1, pem->sensors[i].sensor_name);
            }
            /* prevent duplicate log */
            if(pem->sensors[i].fail_flag & SENSOR_UNC) {
                break;
            }
            pem->sensors[i].fail_flag |= SENSOR_UNC;
            switch(pem->sensors[i].index) {
            case PEM1_SENSOR_TEMP1:
            case PEM2_SENSOR_TEMP1:
                ret |= PEM_HANDLER_ARCHIVE;
                OBMC_CRIT("PEM %d sensor %s (OTP) UNC Error", pem_num + 1, pem->sensors[i].sensor_name);
                break;
            }
            break;
        case SENSOR_LCR:
            /* prevent duplicate log */
            if(pem->sensors[i].fail_flag & SENSOR_LCR) {
                break;
            }
            pem->sensors[i].fail_flag |= SENSOR_LCR;
            switch(pem->sensors[i].index) {
            case PEM1_SENSOR_IN_VOLT:
            case PEM1_SENSOR_OUT_VOLT:
            case PEM2_SENSOR_IN_VOLT:
            case PEM2_SENSOR_OUT_VOLT:
                OBMC_CRIT("PEM %d sensor %s LCR Error", pem_num + 1, pem->sensors[i].sensor_name);
                break;
            case PEM1_SENSOR_FAN1_TACH:
            case PEM1_SENSOR_FAN2_TACH:
            case PEM2_SENSOR_FAN1_TACH:
            case PEM2_SENSOR_FAN2_TACH:
                pem->fan_fail_cnt++;
                OBMC_CRIT("PEM %d sensor %s LCR Error", pem_num + 1, pem->sensors[i].sensor_name);
                break;
            }
            break;
        case SENSOR_LNC:
            /* Check if LCR recovery */
            if(pem->sensors[i].fail_flag & SENSOR_LCR) {
                pem->sensors[i].fail_flag &= (~SENSOR_LCR);
                pem->sensors[i].threshold_cnt[SENSOR_LCR_THRESH] = 0;
                OBMC_INFO("PEM %d sensor %s LCR recovery", pem_num + 1, pem->sensors[i].sensor_name);
            }
            break;
        default:
            /* If last is normal, skip to check failure recovery */
            if(pem->sensors[i].fail_flag == SENSOR_NORMAL)
                break;
            if(pem->sensors[i].fail_flag & SENSOR_UCR) {
                pem->sensors[i].fail_flag &= (~SENSOR_UCR);
                OBMC_INFO("PEM %d sensor %s UCR recovery", pem_num + 1, pem->sensors[i].sensor_name);
            }
            if(pem->sensors[i].fail_flag & SENSOR_UNC) {
                pem->sensors[i].fail_flag &= (~SENSOR_UNC);
                OBMC_INFO("PEM %d sensor %s UNC recovery", pem_num + 1, pem->sensors[i].sensor_name);
            }
            if(pem->sensors[i].fail_flag & SENSOR_LCR) {
                pem->sensors[i].fail_flag &= (~SENSOR_LCR);
                OBMC_INFO("PEM %d sensor %s LCR recovery", pem_num + 1, pem->sensors[i].sensor_name);
            }
            if(pem->sensors[i].fail_flag & SENSOR_LNC) {
                pem->sensors[i].fail_flag &= (~SENSOR_LNC);
                OBMC_INFO("PEM %d sensor %s LNC recovery", pem_num + 1, pem->sensors[i].sensor_name);
            }
            break;
        }
#ifdef DEBUG
        OBMC_INFO("[debug] PEM %d check threshold sensor_0x%02x %s value: %0.2f with [ucr: %0.2f, cnt: %d] " \
                   "[unc: %0.2f, cnt: %d] [lcr: %0.2f, cnt: %d] [lnc: %0.2f, cnt: %d]", pem_num + 1, 
                   pem->sensors[i].index, pem->sensors[i].sensor_name, pem->sensors[i].value,
                   pem->sensors[i].threshold[SENSOR_UCR_THRESH], pem->sensors[i].threshold_cnt[SENSOR_UCR_THRESH],
                   pem->sensors[i].threshold[SENSOR_UNC_THRESH], pem->sensors[i].threshold_cnt[SENSOR_UNC_THRESH],
                   pem->sensors[i].threshold[SENSOR_LCR_THRESH], pem->sensors[i].threshold_cnt[SENSOR_LCR_THRESH],
                   pem->sensors[i].threshold[SENSOR_LNC_THRESH], pem->sensors[i].threshold_cnt[SENSOR_LNC_THRESH]);
#endif
    }
    if(pem->fan_fail_cnt >= PEM_FAN_NUM)
        ret |= PEM_HANDLER_SHUTDOWN;
    return ret;
}

/* 
 * check pem discrete sensors and handle exception
 */
static int check_pem_discrete_sensors(uint8_t pem_num, struct pem_t *pem) {
    int ret = PEM_HANDLER_NORMAL;
    for (int i = 0; i < PEM_DISCRETE_NUM; i++) {
        switch(pem->discrete[i].index) {
        case PEM1_SENSOR_FAULT_FET_SHORT:
        case PEM1_SENSOR_FAULT_FET_BAD:
        case PEM2_SENSOR_FAULT_FET_SHORT:
        case PEM2_SENSOR_FAULT_FET_BAD:
        case PEM1_SENSOR_STATUS_FET_BAD:
        case PEM1_SENSOR_STATUS_FET_SHORT:
        case PEM2_SENSOR_STATUS_FET_BAD:
        case PEM2_SENSOR_STATUS_FET_SHORT:
#ifdef DEBUG
        OBMC_INFO("[debug] PEM %d discrete sensor_0x%02x %s value: %d", pem_num + 1,
                    pem->discrete[i].index, pem->discrete[i].sensor_name,
                    pem->discrete[i].value);
#endif
            if(check_bool_fail(&pem->discrete[i])) {
                OBMC_CRIT("PEM %d: %s occurring", pem_num + 1, pem->discrete[i].sensor_name);
            } else {
#ifdef DEBUG
                OBMC_INFO("[debug] PEM %d discrete sensor_0x%02x %s is normal",
                            pem_num + 1, pem->discrete[i].index, pem->discrete[i].sensor_name);
#endif
            }
            break;
        default:
            break;
        }
    }

    return ret;
}

static void pem_handler(uint8_t brd_type, uint8_t pem_num, uint8_t sensors_status) {
    /* Archive ltc4282 & max6615 to /mnt/data */
    if (sensors_status & PEM_HANDLER_ARCHIVE) {
        log_pem_critical_regs(pem_num);
        archive_pem_chips(pem_num);
        OBMC_CRIT("Archive PEM %d to /mnt/data/pem/ success", pem_num + 1);
    }
    /* Shutdown COMe & TH3/GB when OTP happens */
    if(sensors_status & PEM_HANDLER_SHUTDOWN) {
        if(brd_type == BRD_TYPE_WEDGE400)
            OBMC_CRIT("PEM %d do COMe/TH3 shutdown", pem_num + 1);
        if(brd_type == BRD_TYPE_WEDGE400C)
            OBMC_CRIT("PEM %d do COMe/GB shutdown", pem_num + 1);
        if (pal_set_server_power(0, SERVER_POWER_OFF)) {
            OBMC_CRIT("PEM %d do COMe shutdown failed", pem_num + 1);
        }
        if (pal_set_th3_power(TH3_POWER_OFF)) {
            if(brd_type == BRD_TYPE_WEDGE400)
                OBMC_CRIT("PEM %d do TH3 shutdown failed", pem_num + 1);
            if(brd_type == BRD_TYPE_WEDGE400C)
                OBMC_CRIT("PEM %d do GB shutdown failed", pem_num + 1);
        }
    }
}

/*
 * Starts monitoring all the sensors on a pem for all the threshold/discrete values.
 * Each pthread runs this monitoring for a different pem.
 */
static void *
pem_monitor(void *arg) {
    uint8_t pem_num = (uint8_t)(uintptr_t)arg;
    uint8_t pem_status;
    uint8_t brd_type;
    unsigned int sensors_status;
    struct pem_t pem;
    int pem_retry = 0;
    memset(&pem, 0, sizeof(pem));

    if(pal_get_board_type(&brd_type)) {
        OBMC_ERROR(-1, "get board type failed, handle as OTP UCR");
        sensors_status |= (PEM_HANDLER_ARCHIVE | PEM_HANDLER_SHUTDOWN);
        pem_handler(brd_type, pem_num, sensors_status);
    }
    init_pem_sensors(pem_num, &pem);
    while(1) {
        pem_status = 0;
        sensors_status = PEM_HANDLER_NORMAL;
#ifdef DEBUG
        OBMC_INFO("[debug] PEM %d mainloop begin", pem_num + 1);
#endif
        pal_is_fru_ready(pem_num + FRU_PEM1, &pem_status);
        if (pem_status != 1) {
#ifdef DEBUG
        OBMC_INFO("[debug] PEM %d mainloop skip to monitor because pem is not ready",
                    pem_num + 1);
#endif
            pem_retry++;
            if(pem_retry >= PEM_CHECK_RETRY_CNT) {
                OBMC_CRIT("PEM %d monitor exit because PEM is not ready", pem_num + 1);
                return;
            }
            sleep(5);
            continue;
        }
        if (dump_pem_sensors(pem_num, &pem)) {
            OBMC_ERROR(-1, "PEM %d is ready but dump sensors failed, "
                              "skip to check sensors", pem_num + 1);
            sleep(5);
            continue;
        }
        sensors_status |= check_pem_threshold_sensors(pem_num, &pem);
        sensors_status |= check_pem_discrete_sensors(pem_num, &pem);
        pem_handler(brd_type, pem_num, sensors_status);
#ifdef DEBUG
        OBMC_INFO("[debug] PEM %d mainloop end with sensor status: %d", pem_num + 1, sensors_status);
#endif
    }
}

static int
run_pemd(void) {
    int ret, arg;
    pthread_t thread_pem[PEM_NUM];

    for(int i = 0; i < PEM_NUM; i++) {
        if(pthread_create(&thread_pem[i], NULL, pem_monitor, (void*)(uintptr_t)i) < 0) {
            OBMC_WARN("pthread_create for PEM %d failed\n", i + 1);
            continue;
        }
        sleep(1);
    }

    for(int i = 0; i < PEM_NUM; i++) {
        pthread_join(thread_pem[i], NULL);
    }

    return 0;
}

int
main(int argc, char **argv) {
    int rc, pid_file;

    pid_file = open("/var/run/pemd.pid", O_CREAT | O_RDWR, 0666);
    rc = flock(pid_file, LOCK_EX | LOCK_NB);
    if(rc) {
        if(EWOULDBLOCK == errno) {
            printf("Another pemd instance is running...\n");
            exit(0);
        }
    } else {
        /* Initializing logging facility. */
        rc = obmc_log_init(argv[0], LOG_INFO, 0);
        if (rc != 0) {
            fprintf(stderr, "%s: failed to initialize logger: %s\n",
                    argv[0], strerror(errno));
            exit(1);
        }
        rc = obmc_log_set_syslog(0, LOG_DAEMON);
        if (rc != 0) {
            fprintf(stderr, "%s: failed to setup syslog: %s\n",
                    argv[0], strerror(errno));
            exit(1);
        }
        obmc_log_unset_std_stream();

        OBMC_INFO("pemd: daemon started");
        run_pemd();
    }

    exit(0);
}

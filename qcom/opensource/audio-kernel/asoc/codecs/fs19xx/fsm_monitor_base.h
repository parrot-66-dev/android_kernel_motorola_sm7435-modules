/* SPDX-License-Identifier: GPL-2.0+ */
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2023. All rights reserved.
 * 2023-08-10 File created.
 */

#ifndef __FSM_MONITOR_BASE_H__
#define __FSM_MONITOR_BASE_H__

#include "fsm_public.h"
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/power_supply.h>
#include <linux/of.h>

struct fsm_mntr_cfg_tbl {
	int threshold;
	int volume;
};

struct fsm_mntr {
	struct device *dev;
	struct workqueue_struct *thread_wq;
	struct delayed_work delay_work;
	int mntr_mode;
	int mntr_scene;
	int mntr_period;
	int mntr_avg_count;
	int bsg_cfg_count;
	int *bsg_tbl;
	int csg_cfg_count;
	int *csg_tbl;
	int bat_vol;
	int bat_cap;
	int bat_temp;
	int mix_volume;
	int cur_volume;
	bool monitor_on;
};

static struct fsm_mntr *g_fsm_mntr;

static int fsm_monitor_get_volume(struct fsm_mntr *mntr, int *volume)
{
	struct fsm_mntr_cfg_tbl *tbl;
	int new_volume;
	int index;
	int i;

	if (mntr == NULL || volume == NULL)
		return -EINVAL;

	new_volume = mntr->mix_volume;
	if (mntr->mntr_mode & 0x3) { // BSG_VOL|BSG_CAP
		tbl = (struct fsm_mntr_cfg_tbl *)mntr->bsg_tbl;
		for (index = -1, i = 0; i < mntr->bsg_cfg_count; i++) {
			if (mntr->bat_vol > tbl[i].threshold)
				continue;
			if (index < 0 || tbl[i].threshold < tbl[index].threshold)
				index = i;
		}
		if (index >= 0 && new_volume > tbl[index].volume)
			new_volume = tbl[index].volume;
		dev_dbg(mntr->dev, "get bsg volume: %d\n", new_volume);
	}

	if (mntr->mntr_mode & 0x4) { // CSG
		tbl = (struct fsm_mntr_cfg_tbl *)mntr->csg_tbl;
		for (index = -1, i = 0; i < mntr->csg_cfg_count; i++) {
			if (mntr->bat_temp > tbl[i].threshold)
				continue;
			if (index < 0 || tbl[i].threshold < tbl[index].threshold)
				index = i;
		}
		if (index >= 0 && new_volume > tbl[index].volume)
			new_volume = tbl[index].volume;
		dev_dbg(mntr->dev, "get csg volume: %d\n", new_volume);
	}

	*volume = new_volume;
	dev_dbg(mntr->dev, "get volume: %d\n", *volume);

	return 0;
}

static int fsm_monitor_update_volume(struct fsm_mntr *mntr)
{
	static int vol, temp, count;
	int volume;
	int ret;

	if (mntr == NULL)
		return -EINVAL;

	if (!mntr->mntr_mode)
		return 0;

	if (mntr->mntr_mode & 0x1) // BSG_VOL
		vol += mntr->bat_vol;
	else if (mntr->mntr_mode & 0x2) // BSG_CAP
		vol += mntr->bat_cap;
	temp += mntr->bat_temp; // CSG
	count++;

	if (mntr->monitor_on && count < mntr->mntr_avg_count)
		return 0;

	mntr->bat_vol = vol / count;
	mntr->bat_temp = temp / count;

	ret = fsm_monitor_get_volume(mntr, &volume);
	if (!ret && mntr->cur_volume != volume) {
		fsm_set_volume(volume);
		mntr->cur_volume = volume;
		dev_info(mntr->dev, "bat avg vol:%d temp:%d volume: %d\n",
				mntr->bat_vol, mntr->bat_temp, volume);
	}

	vol = 0;
	temp = 0;
	count = 0;

	return 0;
}

static int fsm_get_ambient_info(struct fsm_mntr *mntr)
{
	union power_supply_propval prop;
	struct power_supply *pspy;
	int ret;

	if (mntr == NULL || mntr->dev == NULL)
		return -EINVAL;

	pspy = power_supply_get_by_name("battery");
	if (pspy == NULL) {
		dev_err(mntr->dev, "Failed to get power supply!\n");
		return -EINVAL;
	}

	ret = power_supply_get_property(pspy,
			POWER_SUPPLY_PROP_VOLTAGE_NOW, &prop);
	mntr->bat_vol = DIV_ROUND_CLOSEST(prop.intval, 1000);

	ret |= power_supply_get_property(pspy,
			POWER_SUPPLY_PROP_CAPACITY, &prop);
	mntr->bat_cap = prop.intval;

	ret |= power_supply_get_property(pspy,
			POWER_SUPPLY_PROP_TEMP, &prop);
	mntr->bat_temp = DIV_ROUND_CLOSEST(prop.intval, 10);

	power_supply_put(pspy);
	dev_dbg(mntr->dev, "bat vol:%d cap:%d temp:%d\n",
			mntr->bat_vol,
			mntr->bat_cap,
			mntr->bat_temp);

	return ret;
}

#endif // __FSM_MONITOR_BASE_H__

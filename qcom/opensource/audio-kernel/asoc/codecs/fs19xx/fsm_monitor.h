/* SPDX-License-Identifier: GPL-2.0+ */
/**
 * Copyright (C) Shanghai FourSemi Semiconductor Co.,Ltd 2016-2023. All rights reserved.
 * 2023-08-10 File created.
 */

#ifndef __FSM_MONITOR_H__
#define __FSM_MONITOR_H__

#include "fsm_monitor_base.h"

static int fsm_monitor_parse_bsg_dts(struct fsm_mntr *mntr)
{
	struct property *prop;
	const __be32 *data;
	int size;
	int i;

	if (mntr == NULL)
		return -EINVAL;

	if (!(mntr->mntr_mode & 0x3))
		return 0;

	prop = of_find_property(mntr->dev->of_node, "fsm,mntr-bsg-cfg", &size);
	if (prop == NULL || !prop->value)
		return -ENODATA;

	if (size & 0x7) { // size = 2*4N bytes
		dev_err(mntr->dev, "Invalid size: %d\n", size);
		return -EINVAL;
	}

	mntr->bsg_tbl = devm_kzalloc(mntr->dev, size, GFP_KERNEL);
	if (mntr->bsg_tbl == NULL)
		return -ENOMEM;

	mntr->bsg_cfg_count = size / sizeof(struct fsm_mntr_cfg_tbl);
	data = prop->value;
	for (i = 0; i < size / sizeof(int); i++)
		mntr->bsg_tbl[i] = be32_to_cpup(data++);

	dev_info(mntr->dev, "bsg count: %d\n", mntr->bsg_cfg_count);

	return 0;
}

static int fsm_monitor_parse_csg_dts(struct fsm_mntr *mntr)
{
	struct property *prop;
	const __be32 *data;
	int size;
	int i;

	if (mntr == NULL)
		return -EINVAL;

	if (!(mntr->mntr_mode & 0x4))
		return 0;

	prop = of_find_property(mntr->dev->of_node, "fsm,mntr-csg-cfg", &size);
	if (prop == NULL || !prop->value)
		return -ENODATA;

	if (size & 0x7) { // size = 2*4N bytes
		dev_err(mntr->dev, "Invalid size: %d\n", size);
		return -EINVAL;
	}

	mntr->csg_tbl = devm_kzalloc(mntr->dev, size, GFP_KERNEL);
	if (mntr->bsg_tbl == NULL)
		return -ENOMEM;

	mntr->csg_cfg_count = size / sizeof(struct fsm_mntr_cfg_tbl);
	data = prop->value;
	for (i = 0; i < size / sizeof(int); i++)
		mntr->csg_tbl[i] = be32_to_cpup(data++);

	dev_info(mntr->dev, "csg count: %d\n", mntr->csg_cfg_count);

	return 0;
}

static int fsm_monitor_parse_dts(struct fsm_mntr *mntr)
{
	struct device_node *np;
	int ret;

	np = mntr->dev->of_node;
	ret = of_property_read_bool(np, "fsm,mntr-mode");
	if (!ret) {
		dev_info(mntr->dev, "Not found property: fsm,mntr-mode\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "fsm,mntr-mode", &mntr->mntr_mode);
	if (ret) {
		dev_err(mntr->dev, "Failed to read fsm,mntr-mode\n");
		return ret;
	}

	ret = of_property_read_u32(np, "fsm,mntr-scene", &mntr->mntr_scene);
	if (ret)
		mntr->mntr_scene = 0x0001; // Music as default

	ret = of_property_read_u32(np, "fsm,mntr-period", &mntr->mntr_period);
	if (ret)
		mntr->mntr_period = 1000; // 1000ms

	ret = of_property_read_u32(np,
			"fsm,mntr-avg-count", &mntr->mntr_avg_count);
	if (ret)
		mntr->mntr_avg_count = 5; // 5 times

	ret = fsm_monitor_parse_bsg_dts(mntr);
	if (ret) {
		dev_err(mntr->dev, "Failed to parse bsg dts: %d\n", ret);
		return ret;
	}

	ret = fsm_monitor_parse_csg_dts(mntr);
	if (ret) {
		dev_err(mntr->dev, "Failed to parse csg dts: %d\n", ret);
		return ret;
	}

	return 0;
}

static void fsm_vbat_monitor_work(struct work_struct *work)
{
	struct fsm_mntr *mntr;
	int ret;

	mntr = container_of(work, struct fsm_mntr, delay_work.work);

	ret = fsm_get_ambient_info(mntr);
	if (ret) {
		dev_err(mntr->dev, "Failed to get ambient info: %d\n", ret);
		return;
	}

	ret = fsm_monitor_update_volume(mntr);
	if (ret)
		dev_err(mntr->dev, "Failed to update volume: %d\n", ret);

	queue_delayed_work(mntr->thread_wq,
			&mntr->delay_work,
			msecs_to_jiffies(mntr->mntr_period));
}

static int fsm_monitor_init(struct device *dev)
{
	struct fsm_mntr *mntr;
	int ret;

	if (dev == NULL)
		return -EINVAL;

	mntr = devm_kzalloc(dev, sizeof(struct fsm_mntr), GFP_KERNEL);
	if (mntr == NULL)
		return -ENOMEM;

	mntr->dev = dev;
	ret = fsm_monitor_parse_dts(mntr);
	if (ret) {
		dev_info(dev, "Not use vbat monitor?\n");
		ret = 0;
		goto err_exit;
	}

	mntr->thread_wq = create_singlethread_workqueue("fsm-monitor");
	INIT_DELAYED_WORK(&mntr->delay_work, fsm_vbat_monitor_work);

	g_fsm_mntr = mntr;

	return 0;

err_exit:
	if (mntr->bsg_tbl)
		devm_kfree(dev, mntr->bsg_tbl);
	if (mntr->csg_tbl)
		devm_kfree(dev, mntr->csg_tbl);
	devm_kfree(dev, mntr);
	g_fsm_mntr = NULL;

	return ret;
}

static void fsm_monitor_deinit(struct device *dev)
{
	struct fsm_mntr *mntr = g_fsm_mntr;

	if (mntr == NULL)
		return;

	if (mntr->bsg_tbl)
		devm_kfree(dev, mntr->bsg_tbl);
	if (mntr->csg_tbl)
		devm_kfree(dev, mntr->csg_tbl);
	devm_kfree(dev, mntr);
	g_fsm_mntr = NULL;
}

#endif // __FSM_MONITOR_H__

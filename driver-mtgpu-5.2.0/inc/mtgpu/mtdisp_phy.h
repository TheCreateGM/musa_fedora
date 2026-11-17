/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __MTDISP_PHY_H__
#define __MTDISP_PHY_H__

#include "phy_types.h"

int  mtdisp_phy_init(struct mtdisp_hwlib_phy *phy);
int  mtdisp_phy_exit(struct mtdisp_hwlib_phy *phy);
int  mtdisp_phy_power_on(struct mtdisp_hwlib_phy *phy);
int  mtdisp_phy_power_off(struct mtdisp_hwlib_phy *phy);
void mtdisp_phy_set_link_rate(struct mtdisp_hwlib_phy *phy, u8 bw_code);
void mtdisp_phy_set_lane_count(struct mtdisp_hwlib_phy *phy, u8 count);
void mtdisp_phy_set_vswing_preemph(struct mtdisp_hwlib_phy *phy, u8 vswing, u8 preemph);
void mtdisp_phy_set_ssc(struct mtdisp_hwlib_phy *phy, bool ssc_en);

#endif /* __MTDISP_PHY_H__ */

/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#ifndef TETRA_DECODER_BURSTTYPE_HPP
#define TETRA_DECODER_BURSTTYPE_HPP

enum BurstType {
    ControlUplinkBurst,
    NormalUplinkBurst,
    NormalUplinkBurst_Split,
    NormalDownlinkBurst,
    NormalDownlinkBurst_Split,
    SynchronizationBurst,
};

#endif // TETRA_DECODER_BURSTTYPE_HPP

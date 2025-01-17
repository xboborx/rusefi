/**
 * @file	at32_common.cpp
 * @brief	Low level common Artery AT32 code
 *
 * @date Oct 29, 2023
 * @author Andrey Gusakov, (c) 2023
 */

#include "pch.h"

int at32GetMcuType(uint32_t id, const char **pn, const char **package, uint32_t *flashSize)
{
    const struct {
        uint32_t uid;
        const char *pn;
        uint32_t flashSize;
        const char *package;
    } at32f43x_types[] = {
        { 0x70084540, "AT32F435ZMT7", 4032, "LQFP144" },
        { 0x70083341, "AT32F435ZGT7", 1024, "LQFP144" },
        { 0x70084598, "AT32F435ZDT7",  448, "LQFP144" },
        { 0x70083242, "AT32F435ZCT7",  256, "LQFP144" },
        { 0x70084543, "AT32F435VMT7", 4032, "LQFP100" },
        { 0x70083344, "AT32F435VGT7", 1024, "LQFP100" },
        { 0x70084599, "AT32F435VDT7",  448, "LQFP100" },
        { 0x70083245, "AT32F435VCT7",  256, "LQFP100" },
        { 0x70084546, "AT32F435RMT7", 4032,  "LQFP64" },
        { 0x70083347, "AT32F435RGT7", 1024,  "LQFP64" },
        { 0x7008459A, "AT32F435RDT7",  448,  "LQFP64" },
        { 0x70083248, "AT32F435RCT7",  256,  "LQFP64" },
        { 0x70084549, "AT32F435CMT7", 4032,  "LQFP48" },
        { 0x7008334A, "AT32F435CGT7", 1024,  "LQFP48" },
        { 0x7008459B, "AT32F435CDT7",  448,  "LQFP48" },
        { 0x7008324B, "AT32F435CCT7",  256,  "LQFP48" },
        { 0x7008454C, "AT32F435CMU7", 4032,   "QFN48" },
        { 0x7008334D, "AT32F435CGU7", 1024,   "QFN48" },
        { 0x7008459C, "AT32F435CDU7",  448,   "QFN48" },
        { 0x7008324E, "AT32F435CCU7",  256,   "QFN48" },
        { 0x7008454F, "AT32F437ZMT7", 4032, "LQFP144" },
        { 0x70083350, "AT32F437ZGT7", 1024, "LQFP144" },
        { 0x7008459D, "AT32F437ZDT7",  448, "LQFP144" },
        { 0x70083251, "AT32F437ZCT7",  256, "LQFP144" },
        { 0x70084552, "AT32F437VMT7", 4032, "LQFP100" },
        { 0x70083353, "AT32F437VGT7", 1024, "LQFP100" },
        { 0x7008459E, "AT32F437VDT7",  448, "LQFP100" },
        { 0x70083254, "AT32F437VCT7",  256, "LQFP100" },
        { 0x70084555, "AT32F437RMT7", 4032,  "LQFP64" },
        { 0x70083356, "AT32F437RGT7", 1024,  "LQFP64" },
        { 0x7008459F, "AT32F437RDT7",  448,  "LQFP64" },
        { 0x70083257, "AT32F437RCT7",  256,  "LQFP64" },
    };

    for (int i = 0; i < efi::size(at32f43x_types); i++) {
        if (id == at32f43x_types[i].uid) {
            if (pn)
                *pn = at32f43x_types[i].pn;
            if (package)
                *package = at32f43x_types[i].package;
            if (flashSize)
                *flashSize = at32f43x_types[i].flashSize;
            return 0;
        }
    }

    /* unknown */
    return -1;
}

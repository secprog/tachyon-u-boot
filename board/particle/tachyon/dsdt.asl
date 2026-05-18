/*
 * Intel ACPI Component Architecture
 * AML/ASL+ Disassembler version 20260408 (32-bit version)
 * Copyright (c) 2000 - 2026 Intel Corporation
 * 
 * Disassembling to symbolic ASL+ operators
 *
 * Disassembly of c:/Users/Dani/Downloads/wwww/dsdt.aml
 *
 * Original Table Header:
 *     Signature        "DSDT"
 *     Length           0x00015EEB (89835)
 *     Revision         0x02
 *     Checksum         0x0E
 *     OEM ID           "QCOMM "
 *     OEM Table ID     "TACHYON "
 *     OEM Revision     0x00000003 (3)
 *     Compiler ID      "MSFT"
 *     Compiler Version 0x05000000 (83886080)
 */
DefinitionBlock ("DSDT.aml", "DSDT", 2, "QCOM", "TACHYON", 0x00000001)
{
    External (_SB_.DPP0, IntObj)
    External (_SB_.DPP1, IntObj)
    External (_SB_.MBCL, DeviceObj)
    External (_SB_.MPP0, IntObj)
    External (_SB_.MPP1, IntObj)
    External (_SB_.SCSS._STA, IntObj)
    External (_SB_.SPSS._STA, IntObj)
    External (_SB_.TZ13._PSV, IntObj)
    External (_SB_.TZ13._TC1, IntObj)
    External (_SB_.TZ13._TC2, IntObj)
    External (_SB_.TZ13._TSP, IntObj)
    External (_SB_.TZ13.TPSV, UnknownObj)
    External (_SB_.TZ13.TTC1, UnknownObj)
    External (_SB_.TZ13.TTC2, UnknownObj)
    External (_SB_.TZ13.TTSP, UnknownObj)
    External (_SB_.TZ15.TPSV, IntObj)
    External (_SB_.TZ16.TPSV, IntObj)
    External (_SB_.TZ17._PSV, IntObj)
    External (_SB_.TZ17._TC1, IntObj)
    External (_SB_.TZ17._TC2, IntObj)
    External (_SB_.TZ17._TSP, IntObj)
    External (_SB_.TZ18.TPSV, IntObj)
    External (_SB_.TZ31._CRT, IntObj)
    External (_SB_.TZ31._PSV, IntObj)
    External (_SB_.TZ31.TCRT, UnknownObj)
    External (_SB_.TZ31.TPSV, UnknownObj)
    External (_SB_.TZ33._CRT, IntObj)
    External (_SB_.TZ33._PSV, IntObj)
    External (_SB_.TZ33.TCRT, UnknownObj)
    External (_SB_.TZ33.TPSV, UnknownObj)
    External (_SB_.TZ34, UnknownObj)
    External (_SB_.TZ34._CRT, IntObj)
    External (_SB_.TZ34._PSV, IntObj)
    External (_SB_.TZ34._TC1, IntObj)
    External (_SB_.TZ34._TC2, IntObj)
    External (_SB_.TZ34._TSP, IntObj)
    External (_SB_.TZ34.TCRT, UnknownObj)
    External (_SB_.TZ34.TPSV, UnknownObj)
    External (_SB_.TZ34.TTC1, UnknownObj)
    External (_SB_.TZ34.TTC2, UnknownObj)
    External (_SB_.TZ34.TTSP, UnknownObj)
    External (_SB_.TZ35, UnknownObj)
    External (_SB_.TZ35._CRT, IntObj)
    External (_SB_.TZ35._PSV, IntObj)
    External (_SB_.TZ35._TC1, IntObj)
    External (_SB_.TZ35._TC2, IntObj)
    External (_SB_.TZ35._TSP, IntObj)
    External (_SB_.TZ35.TCRT, UnknownObj)
    External (_SB_.TZ35.TPSV, UnknownObj)
    External (_SB_.TZ35.TTC1, UnknownObj)
    External (_SB_.TZ35.TTC2, UnknownObj)
    External (_SB_.TZ35.TTSP, UnknownObj)
    External (_SB_.TZ36, UnknownObj)
    External (_SB_.TZ36._CRT, IntObj)
    External (_SB_.TZ36._PSV, IntObj)
    External (_SB_.TZ36._TC1, IntObj)
    External (_SB_.TZ36._TC2, IntObj)
    External (_SB_.TZ36._TSP, IntObj)
    External (_SB_.TZ36.TCRT, UnknownObj)
    External (_SB_.TZ36.TPSV, UnknownObj)
    External (_SB_.TZ36.TTC1, UnknownObj)
    External (_SB_.TZ36.TTC2, UnknownObj)
    External (_SB_.TZ36.TTSP, UnknownObj)
    External (_SB_.TZ37, UnknownObj)
    External (_SB_.TZ37._CRT, IntObj)
    External (_SB_.TZ37._PSV, IntObj)
    External (_SB_.TZ37._TC1, IntObj)
    External (_SB_.TZ37._TC2, IntObj)
    External (_SB_.TZ37._TSP, IntObj)
    External (_SB_.TZ37.TCRT, UnknownObj)
    External (_SB_.TZ37.TPSV, UnknownObj)
    External (_SB_.TZ37.TTC1, UnknownObj)
    External (_SB_.TZ37.TTC2, UnknownObj)
    External (_SB_.TZ37.TTSP, UnknownObj)
    External (_SB_.TZ38, UnknownObj)
    External (_SB_.TZ38._CRT, IntObj)
    External (_SB_.TZ38._PSV, IntObj)
    External (_SB_.TZ38._TC1, IntObj)
    External (_SB_.TZ38._TC2, IntObj)
    External (_SB_.TZ38._TSP, IntObj)
    External (_SB_.TZ38.TCRT, UnknownObj)
    External (_SB_.TZ38.TPSV, UnknownObj)
    External (_SB_.TZ38.TTC1, UnknownObj)
    External (_SB_.TZ38.TTC2, UnknownObj)
    External (_SB_.TZ38.TTSP, UnknownObj)
    External (_SB_.TZ53._PSV, IntObj)
    External (_SB_.TZ53._TC1, IntObj)
    External (_SB_.TZ53._TC2, IntObj)
    External (_SB_.TZ53._TSP, IntObj)
    External (_SB_.TZ53.TPSV, UnknownObj)
    External (_SB_.TZ53.TTC1, UnknownObj)
    External (_SB_.TZ53.TTC2, UnknownObj)
    External (_SB_.TZ53.TTSP, UnknownObj)
    External (_SB_.TZ54._PSV, IntObj)
    External (_SB_.TZ54._TC1, IntObj)
    External (_SB_.TZ54._TC2, IntObj)
    External (_SB_.TZ54._TSP, IntObj)
    External (_SB_.TZ54.TPSV, UnknownObj)
    External (_SB_.TZ54.TTC1, UnknownObj)
    External (_SB_.TZ54.TTC2, UnknownObj)
    External (_SB_.TZ54.TTSP, UnknownObj)
    External (_SB_.TZ55._PSV, IntObj)
    External (_SB_.TZ55._TC1, IntObj)
    External (_SB_.TZ55._TC2, IntObj)
    External (_SB_.TZ55._TSP, IntObj)
    External (_SB_.TZ55.TPSV, UnknownObj)
    External (_SB_.TZ55.TTC1, UnknownObj)
    External (_SB_.TZ55.TTC2, UnknownObj)
    External (_SB_.TZ55.TTSP, UnknownObj)
    External (_SB_.TZ56._PSV, IntObj)
    External (_SB_.TZ56._TC1, IntObj)
    External (_SB_.TZ56._TC2, IntObj)
    External (_SB_.TZ56._TSP, IntObj)
    External (_SB_.TZ56.TPSV, UnknownObj)
    External (_SB_.TZ56.TTC1, UnknownObj)
    External (_SB_.TZ56.TTC2, UnknownObj)
    External (_SB_.TZ56.TTSP, UnknownObj)
    External (_SB_.TZ60, UnknownObj)
    External (_SB_.TZ60._PSV, IntObj)
    External (_SB_.TZ60._TC1, IntObj)
    External (_SB_.TZ60._TC2, IntObj)
    External (_SB_.TZ60._TSP, IntObj)
    External (_SB_.TZ60.TPSV, UnknownObj)
    External (_SB_.TZ60.TTC1, UnknownObj)
    External (_SB_.TZ60.TTC2, UnknownObj)
    External (_SB_.TZ60.TTSP, UnknownObj)
    External (_SB_.TZ61, UnknownObj)
    External (_SB_.TZ61._PSV, IntObj)
    External (_SB_.TZ61._TC1, IntObj)
    External (_SB_.TZ61._TC2, IntObj)
    External (_SB_.TZ61._TSP, IntObj)
    External (_SB_.TZ61.TPSV, UnknownObj)
    External (_SB_.TZ61.TTC1, UnknownObj)
    External (_SB_.TZ61.TTC2, UnknownObj)
    External (_SB_.TZ61.TTSP, UnknownObj)
    External (_SB_.TZ62, UnknownObj)
    External (_SB_.TZ62._PSV, IntObj)
    External (_SB_.TZ62._TC1, IntObj)
    External (_SB_.TZ62._TC2, IntObj)
    External (_SB_.TZ62._TSP, IntObj)
    External (_SB_.TZ62.TPSV, UnknownObj)
    External (_SB_.TZ62.TTC1, UnknownObj)
    External (_SB_.TZ62.TTC2, UnknownObj)
    External (_SB_.TZ62.TTSP, UnknownObj)

    Scope (\_SB)
    {
        Name (PSUB, "IOT06490")
        Name (SOID, 0xFFFFFFFF)
        Name (STOR, 0xABCABCAB)
        Name (SIDS, "899800000000000")
        Name (SIDV, 0xFFFFFFFF)
        Name (SVMJ, 0xFFFF)
        Name (SVMI, 0xFFFF)
        Name (SDFE, 0xFFFF)
        Name (SFES, "899800000000000")
        Name (SIDM, 0x0000000FFFFFFFFF)
        Name (SUFS, 0xFFFFFFFF)
        Name (PUS3, 0xFFFFFFFF)
        Name (SUS3, 0xFFFFFFFF)
        Name (SIDT, 0xFFFFFFFF)
        Name (SJTG, 0xFFFFFFFF)
        Name (SOSN, 0xAAAAAAAABBBBBBBB)
        Name (PLST, 0xFFFFFFFF)
        Name (EMUL, 0xFFFFFFFF)
        Name (RMTB, 0xAAAAAAAA)
        Name (RMTX, 0xBBBBBBBB)
        Name (RFMB, 0xCCCCCCCC)
        Name (RFMS, 0xDDDDDDDD)
        Name (RFAB, 0xEEEEEEEE)
        Name (RFAS, 0x77777777)
        Name (TCMA, 0xDEADBEEF)
        Name (TCML, 0xBEEFDEAD)
        Name (SOSI, 0xDEADBEEFFFFFFFFF)
        Name (PRP0, 0xFFFFFFFF)
        Name (PRP1, 0xFFFFFFFF)
        Name (SKUV, 0xFFFFFFFF)
        Name (SDDR, 0xFFFFFFFF)
        Name (UAON, 0xFFFFFFFF)
        Device (UFS0)
        {
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If ((STOR == One))
                {
                    Return (0x0F)
                }
                Else
                {
                    Return (Zero)
                }
            }

            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.PEP0, 
            })
            Name (_HID, "QCOM24A5")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Alias (\_SB.EMUL, EMUL)
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_CCA, One)  // _CCA: Cache Coherency Attribute
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x17)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x40, 0xD8, 0x01,  // .....@..
                    /* 0008 */  0x00, 0xC0, 0x01, 0x00, 0x89, 0x06, 0x00, 0x01,  // ........
                    /* 0010 */  0x01, 0x29, 0x01, 0x00, 0x00, 0x79, 0x00         // .)...y.
                })
                Return (RBUF) /* \_SB_.UFS0._CRS.RBUF */
            }

            Device (DEV0)
            {
                Method (_ADR, 0, NotSerialized)  // _ADR: Address
                {
                    Return (0x08)
                }

                Method (_RMV, 0, NotSerialized)  // _RMV: Removal Status
                {
                    Return (Zero)
                }
            }
        }

        Device (SDC1)
        {
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If ((STOR == 0x02))
                {
                    Return (0x0F)
                }
                Else
                {
                    Return (Zero)
                }
            }

            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.PEP0, 
            })
            Name (_HID, "QCOM24BF")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_CCA, Zero)  // _CCA: Cache Coherency Attribute
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x17)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x40, 0x7C, 0x00,  // .....@|.
                    /* 0008 */  0x00, 0x10, 0x00, 0x00, 0x89, 0x06, 0x00, 0x01,  // ........
                    /* 0010 */  0x01, 0xAC, 0x02, 0x00, 0x00, 0x79, 0x00         // .....y.
                })
                Return (RBUF) /* \_SB_.SDC1._CRS.RBUF */
            }

            Device (EMMC)
            {
                Method (_ADR, 0, NotSerialized)  // _ADR: Address
                {
                    Return (0x08)
                }

                Method (_RMV, 0, NotSerialized)  // _RMV: Removal Status
                {
                    Return (Zero)
                }
            }

            Method (_DIS, 0, NotSerialized)  // _DIS: Disable Device
            {
            }
        }

        Device (SDC2)
        {
            Name (_DEP, Package (0x02)  // _DEP: Dependencies
            {
                \_SB.PEP0, , 
                \_SB.GIO0, 
            })
            Name (_HID, "QCOM2466")  // _HID: Hardware ID
            Name (_UID, One)  // _UID: Unique ID
            Name (_CCA, Zero)  // _CCA: Cache Coherency Attribute
            Alias (\_SB.PSUB, _SUB)
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x5D)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x40, 0x80, 0x08,  // .....@..
                    /* 0008 */  0x00, 0x10, 0x00, 0x00, 0x89, 0x06, 0x00, 0x01,  // ........
                    /* 0010 */  0x01, 0xEF, 0x00, 0x00, 0x00, 0x8C, 0x20, 0x00,  // ...... .
                    /* 0018 */  0x01, 0x00, 0x01, 0x00, 0x1D, 0x00, 0x01, 0x00,  // ........
                    /* 0020 */  0x00, 0x88, 0x13, 0x17, 0x00, 0x00, 0x19, 0x00,  // ........
                    /* 0028 */  0x23, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x5C, 0x5F,  // #.....\_
                    /* 0030 */  0x53, 0x42, 0x2E, 0x47, 0x49, 0x4F, 0x30, 0x00,  // SB.GIO0.
                    /* 0038 */  0x8C, 0x20, 0x00, 0x01, 0x01, 0x01, 0x00, 0x08,  // . ......
                    /* 0040 */  0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00,  // ........
                    /* 0048 */  0x00, 0x19, 0x00, 0x23, 0x00, 0x00, 0x00, 0x5B,  // ...#...[
                    /* 0050 */  0x00, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x47, 0x49,  // .\_SB.GI
                    /* 0058 */  0x4F, 0x30, 0x00, 0x79, 0x00                     // O0.y.
                })
                Return (RBUF) /* \_SB_.SDC2._CRS.RBUF */
            }

            Method (_DIS, 0, NotSerialized)  // _DIS: Disable Device
            {
            }

            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }
        }

        Device (ABD)
        {
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.PEP0, 
            })
            Name (_HID, "QCOM0427")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, Zero)  // _UID: Unique ID
            OperationRegion (ROP1, GenericSerialBus, Zero, 0x0100)
            Name (AVBL, Zero)
            Method (_REG, 2, NotSerialized)  // _REG: Region Availability
            {
                If ((Arg0 == 0x09))
                {
                    AVBL = Arg1
                }
            }
        }

        Name (ESNL, 0x14)
        Name (DBFL, 0x17)
        Device (PMIC)
        {
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.SPMI, 
            })
            Name (_HID, "QCOM0A2B")  // _HID: Hardware ID
            Name (_CID, "PNP0CA3")  // _CID: Compatible ID
            Alias (\_SB.PSUB, _SUB)
            Method (PMCF, 0, NotSerialized)
            {
                Name (CFG0, Package (0x0B)
                {
                    0x0A, 
                    Package (0x02)
                    {
                        Zero, 
                        0x10
                    }, 

                    Package (0x02)
                    {
                        One, 
                        0x10
                    }, 

                    Package (0x02)
                    {
                        0x02, 
                        0x10
                    }, 

                    Package (0x02)
                    {
                        0x03, 
                        0x10
                    }, 

                    Package (0x02)
                    {
                        0x04, 
                        0x10
                    }, 

                    Package (0x02)
                    {
                        0x10, 
                        0x10
                    }, 

                    Package (0x02)
                    {
                        0x10, 
                        0x10
                    }, 

                    Package (0x02)
                    {
                        0x10, 
                        0x10
                    }, 

                    Package (0x02)
                    {
                        0x08, 
                        0x09
                    }, 

                    Package (0x02)
                    {
                        0x10, 
                        0x10
                    }
                })
                Return (CFG0) /* \_SB_.PMIC.PMCF.CFG0 */
            }
        }

        Device (PML0)
        {
            Name (_HID, "QCOM0AD3")  // _HID: Hardware ID
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.I2C2, 
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (Zero)
            }

            Method (_SUB, 0, NotSerialized)  // _SUB: Subsystem ID
            {
                Return ("IDP06490")
            }

            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                If (((((\_SB.SKUV == 0x1F) || (\_SB.SKUV == 0x20)) || (\_SB.SKUV == 
                    0x21)) || (\_SB.SKUV == 0x22)))
                {
                    Name (RBFC, Buffer (0xB8)
                    {
                        /* 0000 */  0x8E, 0x19, 0x00, 0x01, 0x00, 0x01, 0x02, 0x00,  // ........
                        /* 0008 */  0x00, 0x01, 0x06, 0x00, 0xA0, 0x86, 0x01, 0x00,  // ........
                        /* 0010 */  0x08, 0x00, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x49,  // ..\_SB.I
                        /* 0018 */  0x32, 0x43, 0x32, 0x00, 0x8E, 0x19, 0x00, 0x01,  // 2C2.....
                        /* 0020 */  0x00, 0x01, 0x02, 0x00, 0x00, 0x01, 0x06, 0x00,  // ........
                        /* 0028 */  0xA0, 0x86, 0x01, 0x00, 0x09, 0x00, 0x5C, 0x5F,  // ......\_
                        /* 0030 */  0x53, 0x42, 0x2E, 0x49, 0x32, 0x43, 0x32, 0x00,  // SB.I2C2.
                        /* 0038 */  0x8E, 0x19, 0x00, 0x01, 0x00, 0x01, 0x02, 0x00,  // ........
                        /* 0040 */  0x00, 0x01, 0x06, 0x00, 0xA0, 0x86, 0x01, 0x00,  // ........
                        /* 0048 */  0x0C, 0x00, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x49,  // ..\_SB.I
                        /* 0050 */  0x32, 0x43, 0x32, 0x00, 0x8E, 0x19, 0x00, 0x01,  // 2C2.....
                        /* 0058 */  0x00, 0x01, 0x02, 0x00, 0x00, 0x01, 0x06, 0x00,  // ........
                        /* 0060 */  0xA0, 0x86, 0x01, 0x00, 0x0D, 0x00, 0x5C, 0x5F,  // ......\_
                        /* 0068 */  0x53, 0x42, 0x2E, 0x49, 0x32, 0x43, 0x32, 0x00,  // SB.I2C2.
                        /* 0070 */  0x8C, 0x20, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00,  // . ......
                        /* 0078 */  0x00, 0x03, 0xC8, 0x00, 0x00, 0x00, 0x17, 0x00,  // ........
                        /* 0080 */  0x00, 0x19, 0x00, 0x23, 0x00, 0x00, 0x00, 0x12,  // ...#....
                        /* 0088 */  0x01, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x50, 0x4D,  // .\_SB.PM
                        /* 0090 */  0x30, 0x31, 0x00, 0x8C, 0x20, 0x00, 0x01, 0x01,  // 01.. ...
                        /* 0098 */  0x01, 0x00, 0x00, 0x00, 0x03, 0xC8, 0x00, 0x00,  // ........
                        /* 00A0 */  0x00, 0x17, 0x00, 0x00, 0x19, 0x00, 0x23, 0x00,  // ......#.
                        /* 00A8 */  0x00, 0x00, 0x13, 0x01, 0x5C, 0x5F, 0x53, 0x42,  // ....\_SB
                        /* 00B0 */  0x2E, 0x50, 0x4D, 0x30, 0x31, 0x00, 0x79, 0x00   // .PM01.y.
                    })
                    Return (RBFC) /* \_SB_.PML0._CRS.RBFC */
                }
                Else
                {
                    Name (RBUF, Buffer (0x5D)
                    {
                        /* 0000 */  0x8E, 0x19, 0x00, 0x01, 0x00, 0x01, 0x02, 0x00,  // ........
                        /* 0008 */  0x00, 0x01, 0x06, 0x00, 0xA0, 0x86, 0x01, 0x00,  // ........
                        /* 0010 */  0x08, 0x00, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x49,  // ..\_SB.I
                        /* 0018 */  0x32, 0x43, 0x32, 0x00, 0x8E, 0x19, 0x00, 0x01,  // 2C2.....
                        /* 0020 */  0x00, 0x01, 0x02, 0x00, 0x00, 0x01, 0x06, 0x00,  // ........
                        /* 0028 */  0xA0, 0x86, 0x01, 0x00, 0x09, 0x00, 0x5C, 0x5F,  // ......\_
                        /* 0030 */  0x53, 0x42, 0x2E, 0x49, 0x32, 0x43, 0x32, 0x00,  // SB.I2C2.
                        /* 0038 */  0x8C, 0x20, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00,  // . ......
                        /* 0040 */  0x00, 0x03, 0xC8, 0x00, 0x00, 0x00, 0x17, 0x00,  // ........
                        /* 0048 */  0x00, 0x19, 0x00, 0x23, 0x00, 0x00, 0x00, 0x13,  // ...#....
                        /* 0050 */  0x01, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x50, 0x4D,  // .\_SB.PM
                        /* 0058 */  0x30, 0x31, 0x00, 0x79, 0x00                     // 01.y.
                    })
                    Return (RBUF) /* \_SB_.PML0._CRS.RBUF */
                }
            }
        }

        Device (PM01)
        {
            Name (_HID, "QCOM0A2D")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, One)  // _UID: Unique ID
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.PMIC, 
            })
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x0B)
                {
                    /* 0000 */  0x89, 0x06, 0x00, 0x09, 0x01, 0x01, 0x02, 0x00,  // ........
                    /* 0008 */  0x00, 0x79, 0x00                                 // .y.
                })
                Return (RBUF) /* \_SB_.PM01._CRS.RBUF */
            }

            Method (_DSM, 4, NotSerialized)  // _DSM: Device-Specific Method
            {
                While (One)
                {
                    Name (_T_0, Buffer (0x01)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                    {
                         0x00                                             // .
                    })
                    CopyObject (ToBuffer (Arg0), _T_0) /* \_SB_.PM01._DSM._T_0 */
                    If ((_T_0 == ToUUID ("4f248f40-d5e2-499f-834c-27758ea1cd3f") /* GPIO Controller */))
                    {
                        While (One)
                        {
                            Name (_T_1, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_1 = ToInteger (Arg2)
                            If ((_T_1 == Zero))
                            {
                                Return (Buffer (One)
                                {
                                     0x03                                             // .
                                })
                            }
                            ElseIf ((_T_1 == One))
                            {
                                Return (Package (0x02)
                                {
                                    0x07, 
                                    0x06
                                })
                            }
                            Else
                            {
                            }

                            Break
                        }
                    }
                    Else
                    {
                        Return (Buffer (One)
                        {
                             0x00                                             // .
                        })
                    }

                    Break
                }
            }
        }

        Device (PMAP)
        {
            Name (_HID, "QCOM0A2C")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (0x03)  // _DEP: Dependencies
            {
                \_SB.PMIC, , 
                \_SB.ABD, , 
                \_SB.SCM0, 
            })
            Method (GEPT, 0, NotSerialized)
            {
                Name (BUFF, Buffer (0x04){})
                CreateByteField (BUFF, Zero, STAT)
                CreateWordField (BUFF, 0x02, DATA)
                DATA = 0x02
                Return (DATA) /* \_SB_.PMAP.GEPT.DATA */
            }

            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x02)
                {
                     0x79, 0x00                                       // y.
                })
                Return (RBUF) /* \_SB_.PMAP._CRS.RBUF */
            }
        }

        Device (PRTC)
        {
            Name (_HID, "ACPI000E" /* Time and Alarm Device */)  // _HID: Hardware ID
            Name (_DEP, Package (0x01)  // _DEP: Dependencies
            {
        \_SB.PMAP
            })
            Method (_GCP, 0, NotSerialized)  // _GCP: Get Capabilities
            {
                Return (0x04)
            }

            Field (\_SB.ABD.ROP1, BufferAcc, NoLock, Preserve)
            {
                Connection (
                    I2cSerialBusV2 (0x0002, ControllerInitiated, 0x00000000,
                        AddressingMode7Bit, "\\_SB.ABD",
                        0x00, ResourceConsumer, , Exclusive,
                        )
                ), 
                AccessAs (BufferAcc, AttribRawBytes (0x18)), 
                FLD0,   192
            }

            Method (_GRT, 0, NotSerialized)  // _GRT: Get Real Time
            {
                Name (BUFF, Buffer (0x1A){})
                CreateField (BUFF, 0x10, 0x80, TME1)
                CreateField (BUFF, 0x90, 0x20, ACT1)
                CreateField (BUFF, 0xB0, 0x20, ACW1)
                BUFF = FLD0 /* \_SB_.PRTC.FLD0 */
                Return (TME1) /* \_SB_.PRTC._GRT.TME1 */
            }

            Method (_SRT, 1, NotSerialized)  // _SRT: Set Real Time
            {
                Name (BUFF, Buffer (0x32){})
                CreateByteField (BUFF, Zero, STAT)
                CreateField (BUFF, 0x10, 0x80, TME1)
                CreateField (BUFF, 0x90, 0x20, ACT1)
                CreateField (BUFF, 0xB0, 0x20, ACW1)
                ACT1 = Zero
                TME1 = Arg0
                ACW1 = Zero
                BUFF = FLD0 = BUFF /* \_SB_.PRTC._SRT.BUFF */
                If ((STAT != Zero))
                {
                    Return (One)
                }

                Return (Zero)
            }
        }

        Device (PMBM)
        {
            Name (_HID, "QCOM0A2A")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.PMGK, 
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (Zero)
            }

            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x02)
                {
                     0x79, 0x00                                       // y.
                })
                Return (RBUF) /* \_SB_.PMBM._CRS.RBUF */
            }
        }

        Device (BCL1)
        {
            Name (_HID, "QCOM0A77")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.PMIC, 
            })
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x011A)
                {
                    /* 0000 */  0x8C, 0x20, 0x00, 0x01, 0x00, 0x01, 0x00, 0x0B,  // . ......
                    /* 0008 */  0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00,  // ........
                    /* 0010 */  0x00, 0x19, 0x00, 0x23, 0x00, 0x00, 0x00, 0x08,  // ...#....
                    /* 0018 */  0x01, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x50, 0x4D,  // .\_SB.PM
                    /* 0020 */  0x30, 0x31, 0x00, 0x8C, 0x20, 0x00, 0x01, 0x00,  // 01.. ...
                    /* 0028 */  0x01, 0x00, 0x09, 0x00, 0x01, 0x00, 0x00, 0x00,  // ........
                    /* 0030 */  0x00, 0x17, 0x00, 0x00, 0x19, 0x00, 0x23, 0x00,  // ......#.
                    /* 0038 */  0x00, 0x00, 0x09, 0x01, 0x5C, 0x5F, 0x53, 0x42,  // ....\_SB
                    /* 0040 */  0x2E, 0x50, 0x4D, 0x30, 0x31, 0x00, 0x8C, 0x20,  // .PM01.. 
                    /* 0048 */  0x00, 0x01, 0x00, 0x01, 0x00, 0x09, 0x00, 0x01,  // ........
                    /* 0050 */  0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x19,  // ........
                    /* 0058 */  0x00, 0x23, 0x00, 0x00, 0x00, 0x0A, 0x01, 0x5C,  // .#.....\
                    /* 0060 */  0x5F, 0x53, 0x42, 0x2E, 0x50, 0x4D, 0x30, 0x31,  // _SB.PM01
                    /* 0068 */  0x00, 0x8C, 0x20, 0x00, 0x01, 0x00, 0x01, 0x00,  // .. .....
                    /* 0070 */  0x09, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x17,  // ........
                    /* 0078 */  0x00, 0x00, 0x19, 0x00, 0x23, 0x00, 0x00, 0x00,  // ....#...
                    /* 0080 */  0x0B, 0x01, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x50,  // ..\_SB.P
                    /* 0088 */  0x4D, 0x30, 0x31, 0x00, 0x8C, 0x20, 0x00, 0x01,  // M01.. ..
                    /* 0090 */  0x00, 0x01, 0x00, 0x0B, 0x00, 0x01, 0x00, 0x00,  // ........
                    /* 0098 */  0x00, 0x00, 0x17, 0x00, 0x00, 0x19, 0x00, 0x23,  // .......#
                    /* 00A0 */  0x00, 0x00, 0x00, 0x10, 0x02, 0x5C, 0x5F, 0x53,  // .....\_S
                    /* 00A8 */  0x42, 0x2E, 0x50, 0x4D, 0x30, 0x31, 0x00, 0x8C,  // B.PM01..
                    /* 00B0 */  0x20, 0x00, 0x01, 0x00, 0x01, 0x00, 0x09, 0x00,  //  .......
                    /* 00B8 */  0x01, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00,  // ........
                    /* 00C0 */  0x19, 0x00, 0x23, 0x00, 0x00, 0x00, 0x11, 0x02,  // ..#.....
                    /* 00C8 */  0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x50, 0x4D, 0x30,  // \_SB.PM0
                    /* 00D0 */  0x31, 0x00, 0x8C, 0x20, 0x00, 0x01, 0x00, 0x01,  // 1.. ....
                    /* 00D8 */  0x00, 0x09, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 00E0 */  0x17, 0x00, 0x00, 0x19, 0x00, 0x23, 0x00, 0x00,  // .....#..
                    /* 00E8 */  0x00, 0x12, 0x02, 0x5C, 0x5F, 0x53, 0x42, 0x2E,  // ...\_SB.
                    /* 00F0 */  0x50, 0x4D, 0x30, 0x31, 0x00, 0x8C, 0x20, 0x00,  // PM01.. .
                    /* 00F8 */  0x01, 0x00, 0x01, 0x00, 0x09, 0x00, 0x01, 0x00,  // ........
                    /* 0100 */  0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x19, 0x00,  // ........
                    /* 0108 */  0x23, 0x00, 0x00, 0x00, 0x13, 0x02, 0x5C, 0x5F,  // #.....\_
                    /* 0110 */  0x53, 0x42, 0x2E, 0x50, 0x4D, 0x30, 0x31, 0x00,  // SB.PM01.
                    /* 0118 */  0x79, 0x00                                       // y.
                })
                Return (RBUF) /* \_SB_.BCL1._CRS.RBUF */
            }

            Method (BCLQ, 0, NotSerialized)
            {
                Name (CFG0, Package (0x08)
                {
                    "PM3_BCLBIG_LVL0", 
                    "PM3_BCLBIG_LVL1", 
                    "PM3_BCLBIG_LVL2", 
                    "PM3_BCLBIG_BAN", 
                    "PM7_BCLBIG_LVL0", 
                    "PM7_BCLBIG_LVL1", 
                    "PM7_BCLBIG_LVL2", 
                    "PM7_BCLBIG_BAN"
                })
                Return (CFG0) /* \_SB_.BCL1.BCLQ.CFG0 */
            }
        }

        Device (PMGK)
        {
            Name (_HID, "QCOM0A8E")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (0x02)  // _DEP: Dependencies
            {
                \_SB.GLNK, , 
                \_SB.ABD, 
            })
            Name (LKUP, Zero)
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }

            Method (GEPT, 0, NotSerialized)
            {
                Name (BUFF, Buffer (0x04){})
                CreateByteField (BUFF, Zero, STAT)
                CreateWordField (BUFF, 0x02, DATA)
                DATA = 0x03
                Return (DATA) /* \_SB_.PMGK.GEPT.DATA */
            }

            Name (UBUF, Buffer (0x32){})
            CreateField (UBUF, Zero, 0x08, BSTA)
            CreateField (UBUF, 0x08, 0x08, BSIZ)
            CreateField (UBUF, 0x10, 0x10, BVER)
            CreateField (UBUF, 0x30, 0x20, BCCI)
            CreateField (UBUF, 0x50, 0x40, BCTL)
            CreateField (UBUF, 0x90, 0x80, BMGI)
            CreateField (UBUF, 0x0110, 0x80, BMGO)
            Method (USBN, 1, NotSerialized)
            {
                UBUF = UCSI /* \_SB_.PMGK.UCSI */
                \_SB.UCSI.VERS = BVER /* \_SB_.PMGK.BVER */
                \_SB.UCSI.MSGI = BMGI /* \_SB_.PMGK.BMGI */
                \_SB.UCSI.CCI = BCCI /* \_SB_.PMGK.BCCI */
                Notify (\_SB.UCSI, Arg0)
                Return (Zero)
            }

            Name (PBUF, Buffer (0x20){})
            CreateField (PBUF, Zero, 0x08, BPID)
            CreateField (PBUF, 0x08, 0x08, BORI)
            CreateField (PBUF, 0x10, 0x08, BMUX)
            CreateField (PBUF, 0x20, 0x10, BVID)
            CreateField (PBUF, 0x30, 0x10, BSID)
            CreateField (PBUF, 0x40, 0x08, BSSD)
            Method (UPAN, 1, NotSerialized)
            {
                PBUF = Arg0
                \_SB.UCS0.EUPD |= One
                \_SB.UCS0.EINF = One
                \_SB.UCS0.ECC0 = BORI /* \_SB_.PMGK.BORI */
                \_SB.UCS0.EMX0 = BMUX /* \_SB_.PMGK.BMUX */
                \_SB.UCS0.EVI0 = BVID /* \_SB_.PMGK.BVID */
                \_SB.UCS0.ESI0 = BSID /* \_SB_.PMGK.BSID */
                \_SB.UCS0.ESV0 = BSSD /* \_SB_.PMGK.BSSD */
                \_SB.UCS0.USBR ()
                Return (Zero)
            }

            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x02)
                {
                     0x79, 0x00                                       // y.
                })
                Return (RBUF) /* \_SB_.PMGK._CRS.RBUF */
            }

            Method (LKST, 1, NotSerialized)
            {
                LKUP = Arg0
                Notify (\_SB.UCSI, Zero) // Bus Check
                Return (Zero)
            }

            Field (\_SB.ABD.ROP1, BufferAcc, NoLock, Preserve)
            {
                Connection (
                    I2cSerialBusV2 (0x0003, ControllerInitiated, 0x00000000,
                        AddressingMode7Bit, "\\_SB.ABD",
                        0x00, ResourceConsumer, , Exclusive,
                        )
                ), 
                AccessAs (BufferAcc, AttribRawBytes (0x30)), 
                UCSI,   384
            }
        }

        Device (PEP0)
        {
            Name (_HID, "QCOM0A17")  // _HID: Hardware ID
            Name (_CID, "PNP0D80" /* Windows-compatible System Power Management Controller */)  // _CID: Compatible ID
            Method (THTZ, 4, NotSerialized)
            {
                Name (PROD, Zero)
                PROD = ((\_SB.SJTG >> 0x0C) & 0xFFFF)
                While (One)
                {
                    Name (_T_0, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                    _T_0 = ToInteger (Arg0)
                    If ((_T_0 == One))
                    {
                        While (One)
                        {
                            Name (_T_1, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_1 = ToInteger (Arg3)
                            If ((_T_1 == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ1.TPSV = Arg1
                                    Notify (\_SB.TZ1, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ1._PSV ())
                            }
                            ElseIf ((_T_1 == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ1.TTSP = Arg1
                                    Notify (\_SB.TZ1, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ1._TSP ())
                            }
                            ElseIf ((_T_1 == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ1.TTC1 = Arg1
                                    Notify (\_SB.TZ1, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ1._TC1 ())
                            }
                            ElseIf ((_T_1 == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ1.TTC2 = Arg1
                                    Notify (\_SB.TZ1, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ1._TC2 ())
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x03))
                    {
                        While (One)
                        {
                            Name (_T_2, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_2 = ToInteger (Arg3)
                            If ((_T_2 == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ3.TPSV = Arg1
                                    Notify (\_SB.TZ3, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ3._PSV ())
                            }
                            ElseIf ((_T_2 == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ3.TTSP = Arg1
                                    Notify (\_SB.TZ3, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ3._TSP ())
                            }
                            ElseIf ((_T_2 == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ3.TTC1 = Arg1
                                    Notify (\_SB.TZ3, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ3._TC1 ())
                            }
                            ElseIf ((_T_2 == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ3.TTC2 = Arg1
                                    Notify (\_SB.TZ3, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ3._TC2 ())
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x05))
                    {
                        While (One)
                        {
                            Name (_T_3, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_3 = ToInteger (Arg3)
                            If ((_T_3 == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ5.TPSV = Arg1
                                    Notify (\_SB.TZ5, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ5._PSV ())
                            }
                            ElseIf ((_T_3 == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ5.TTSP = Arg1
                                    Notify (\_SB.TZ5, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ5._TSP ())
                            }
                            ElseIf ((_T_3 == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ5.TTC1 = Arg1
                                    Notify (\_SB.TZ5, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ5._TC1 ())
                            }
                            ElseIf ((_T_3 == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ5.TTC2 = Arg1
                                    Notify (\_SB.TZ5, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ5._TC2 ())
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x06))
                    {
                        While (One)
                        {
                            Name (_T_4, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_4 = ToInteger (Arg3)
                            If ((_T_4 == Zero))
                            {
                                If (Arg2)
                                {
                                    If ((((PROD == 0x0197) & (\_SB.SIDT == 0x03)) | ((
                                        PROD == 0x0198) & (\_SB.SIDT == 0x04))))
                                    {
                                        \_SB.TZ6.PSV1 = Arg1
                                    }
                                    Else
                                    {
                                        \_SB.TZ6.PSV2 = Arg1
                                    }

                                    Notify (\_SB.TZ6, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ6._PSV ())
                            }
                            ElseIf ((_T_4 == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ6.TTSP = Arg1
                                    Notify (\_SB.TZ6, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ6._TSP ())
                            }
                            ElseIf ((_T_4 == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ6.TTC1 = Arg1
                                    Notify (\_SB.TZ6, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ6._TC1 ())
                            }
                            ElseIf ((_T_4 == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ6.TTC2 = Arg1
                                    Notify (\_SB.TZ6, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ6._TC2 ())
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x0A))
                    {
                        While (One)
                        {
                            Name (_T_5, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_5 = ToInteger (Arg3)
                            If ((_T_5 == Zero))
                            {
                                If (Arg2)
                                {
                                    If ((((PROD == 0x0197) & (\_SB.SIDT == 0x03)) | ((
                                        PROD == 0x0198) & (\_SB.SIDT == 0x04))))
                                    {
                                        \_SB.TZ10.PSV1 = Arg1
                                    }
                                    Else
                                    {
                                        \_SB.TZ10.PSV2 = Arg1
                                    }

                                    Notify (\_SB.TZ10, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ10._PSV ())
                            }
                            ElseIf ((_T_5 == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ10.TTSP = Arg1
                                    Notify (\_SB.TZ10, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ10._TSP ())
                            }
                            ElseIf ((_T_5 == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ10.TTC1 = Arg1
                                    Notify (\_SB.TZ10, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ10._TC1 ())
                            }
                            ElseIf ((_T_5 == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ10.TTC2 = Arg1
                                    Notify (\_SB.TZ10, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ10._TC2 ())
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x0B))
                    {
                        While (One)
                        {
                            Name (_T_6, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_6 = ToInteger (Arg3)
                            If ((_T_6 == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ11.TPSV = Arg1
                                    Notify (\_SB.TZ11, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ11._PSV ())
                            }
                            ElseIf ((_T_6 == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ11.TTSP = Arg1
                                    Notify (\_SB.TZ11, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ11._TSP ())
                            }
                            ElseIf ((_T_6 == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ11.TTC1 = Arg1
                                    Notify (\_SB.TZ11, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ11._TC1 ())
                            }
                            ElseIf ((_T_6 == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ11.TTC2 = Arg1
                                    Notify (\_SB.TZ11, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ11._TC2 ())
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x0D))
                    {
                        While (One)
                        {
                            Name (_T_7, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_7 = ToInteger (Arg3)
                            If ((_T_7 == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ13.TPSV = Arg1
                                    Notify (\_SB.TZ13, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ13._PSV) /* External reference */
                            }
                            ElseIf ((_T_7 == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ13.TTSP = Arg1
                                    Notify (\_SB.TZ13, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ13._TSP) /* External reference */
                            }
                            ElseIf ((_T_7 == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ13.TTC1 = Arg1
                                    Notify (\_SB.TZ13, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ13._TC1) /* External reference */
                            }
                            ElseIf ((_T_7 == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ13.TTC2 = Arg1
                                    Notify (\_SB.TZ13, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ13._TC2) /* External reference */
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x0F))
                    {
                        While (One)
                        {
                            Name (_T_8, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_8 = ToInteger (Arg3)
                            If ((_T_8 == Zero))
                            {
                                If (Arg2)
                                {
                                    Return (0xFFFF)
                                }

                                Return (\_SB.TZ15._PSV ())
                            }
                            ElseIf ((_T_8 == 0x02))
                            {
                                If (Arg2)
                                {
                                    Return (0xFFFF)
                                }

                                Return (\_SB.TZ15._TSP)
                            }
                            ElseIf ((_T_8 == 0x03))
                            {
                                If (Arg2)
                                {
                                    Return (0xFFFF)
                                }

                                Return (\_SB.TZ15._TC1 ())
                            }
                            ElseIf ((_T_8 == 0x04))
                            {
                                If (Arg2)
                                {
                                    Return (0xFFFF)
                                }

                                Return (\_SB.TZ15._TC2 ())
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x10))
                    {
                        While (One)
                        {
                            Name (_T_9, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_9 = ToInteger (Arg3)
                            If ((_T_9 == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ16.TPSV = Arg1
                                    Notify (\_SB.TZ16, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ16._PSV ())
                            }
                            ElseIf ((_T_9 == One))
                            {
                                If (Arg2)
                                {
                                    Return (0xFFFF)
                                }

                                Return (\_SB.TZ16._CRT ())
                            }
                            ElseIf ((_T_9 == 0x02))
                            {
                                If (Arg2)
                                {
                                    Return (0xFFFF)
                                }

                                Return (\_SB.TZ16._TSP)
                            }
                            ElseIf ((_T_9 == 0x03))
                            {
                                If (Arg2)
                                {
                                    Return (0xFFFF)
                                }

                                Return (\_SB.TZ16._TC1 ())
                            }
                            ElseIf ((_T_9 == 0x04))
                            {
                                If (Arg2)
                                {
                                    Return (0xFFFF)
                                }

                                Return (\_SB.TZ16._TC2 ())
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x11))
                    {
                        While (One)
                        {
                            Name (_T_A, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_A = ToInteger (Arg3)
                            If ((_T_A == Zero))
                            {
                                If (Arg2)
                                {
                                    Return (0xFFFF)
                                }

                                Return (\_SB.TZ17._PSV) /* External reference */
                            }
                            ElseIf ((_T_A == 0x02))
                            {
                                If (Arg2)
                                {
                                    Return (0xFFFF)
                                }

                                Return (\_SB.TZ17._TSP) /* External reference */
                            }
                            ElseIf ((_T_A == 0x03))
                            {
                                If (Arg2)
                                {
                                    Return (0xFFFF)
                                }

                                Return (\_SB.TZ17._TC1) /* External reference */
                            }
                            ElseIf ((_T_A == 0x04))
                            {
                                If (Arg2)
                                {
                                    Return (0xFFFF)
                                }

                                Return (\_SB.TZ17._TC2) /* External reference */
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x12))
                    {
                        While (One)
                        {
                            Name (_T_B, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_B = ToInteger (Arg3)
                            If ((_T_B == Zero))
                            {
                                If (Arg2)
                                {
                                    Return (0xFFFF)
                                }

                                Return (\_SB.TZ18._PSV ())
                            }
                            ElseIf ((_T_B == 0x02))
                            {
                                If (Arg2)
                                {
                                    Return (0xFFFF)
                                }

                                Return (\_SB.TZ18._TSP)
                            }
                            ElseIf ((_T_B == 0x03))
                            {
                                If (Arg2)
                                {
                                    Return (0xFFFF)
                                }

                                Return (\_SB.TZ18._TC1 ())
                            }
                            ElseIf ((_T_B == 0x04))
                            {
                                If (Arg2)
                                {
                                    Return (0xFFFF)
                                }

                                Return (\_SB.TZ18._TC2 ())
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x1F))
                    {
                        While (One)
                        {
                            Name (_T_C, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_C = ToInteger (Arg3)
                            If ((_T_C == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ31.TPSV = Arg1
                                    Notify (\_SB.TZ31, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ31._PSV) /* External reference */
                            }
                            ElseIf ((_T_C == One))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ31.TCRT = Arg1
                                    Notify (\_SB.TZ31, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ31._CRT) /* External reference */
                            }
                            ElseIf ((_T_C == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ31.TTSP = Arg1
                                    Notify (\_SB.TZ31, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ31._TSP ())
                            }
                            ElseIf ((_T_C == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ31.TTC1 = Arg1
                                    Notify (\_SB.TZ31, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ31._TC1 ())
                            }
                            ElseIf ((_T_C == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ31.TTC2 = Arg1
                                    Notify (\_SB.TZ31, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ31._TC2 ())
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x20))
                    {
                        While (One)
                        {
                            Name (_T_D, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_D = ToInteger (Arg3)
                            If ((_T_D == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ32.TPSV = Arg1
                                    Notify (\_SB.TZ32, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ32._PSV ())
                            }
                            ElseIf ((_T_D == One))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ32.TCRT = Arg1
                                    Notify (\_SB.TZ32, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ32._CRT ())
                            }
                            ElseIf ((_T_D == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ32.TTSP = Arg1
                                    Notify (\_SB.TZ32, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ32._TSP ())
                            }
                            ElseIf ((_T_D == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ32.TTC1 = Arg1
                                    Notify (\_SB.TZ32, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ32._TC1 ())
                            }
                            ElseIf ((_T_D == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ32.TTC2 = Arg1
                                    Notify (\_SB.TZ32, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ32._TC2 ())
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x21))
                    {
                        While (One)
                        {
                            Name (_T_E, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_E = ToInteger (Arg3)
                            If ((_T_E == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ33.TPSV = Arg1
                                    Notify (\_SB.TZ33, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ33._PSV) /* External reference */
                            }
                            ElseIf ((_T_E == One))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ33.TCRT = Arg1
                                    Notify (\_SB.TZ33, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ33._CRT) /* External reference */
                            }
                            ElseIf ((_T_E == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ33.TTSP = Arg1
                                    Notify (\_SB.TZ33, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ33._TSP ())
                            }
                            ElseIf ((_T_E == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ33.TTC1 = Arg1
                                    Notify (\_SB.TZ33, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ33._TC1 ())
                            }
                            ElseIf ((_T_E == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ33.TTC2 = Arg1
                                    Notify (\_SB.TZ33, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ33._TC2 ())
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x22))
                    {
                        While (One)
                        {
                            Name (_T_F, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_F = ToInteger (Arg3)
                            If ((_T_F == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ34.TPSV = Arg1
                                    Notify (\_SB.TZ34, 0x81) // Information Change
                                }

                                Return (\_SB.TZ34._PSV) /* External reference */
                            }
                            ElseIf ((_T_F == One))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ34.TCRT = Arg1
                                    Notify (\_SB.TZ34, 0x81) // Information Change
                                }

                                Return (\_SB.TZ34._CRT) /* External reference */
                            }
                            ElseIf ((_T_F == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ34.TTSP = Arg1
                                    Notify (\_SB.TZ34, 0x81) // Information Change
                                }

                                Return (\_SB.TZ34._TSP) /* External reference */
                            }
                            ElseIf ((_T_F == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ34.TTC1 = Arg1
                                    Notify (\_SB.TZ34, 0x81) // Information Change
                                }

                                Return (\_SB.TZ34._TC1) /* External reference */
                            }
                            ElseIf ((_T_F == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ34.TTC2 = Arg1
                                    Notify (\_SB.TZ34, 0x81) // Information Change
                                }

                                Return (\_SB.TZ34._TC2) /* External reference */
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x23))
                    {
                        While (One)
                        {
                            Name (_T_G, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_G = ToInteger (Arg3)
                            If ((_T_G == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ35.TPSV = Arg1
                                    Notify (\_SB.TZ35, 0x81) // Information Change
                                }

                                Return (\_SB.TZ35._PSV) /* External reference */
                            }
                            ElseIf ((_T_G == One))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ35.TCRT = Arg1
                                    Notify (\_SB.TZ35, 0x81) // Information Change
                                }

                                Return (\_SB.TZ35._CRT) /* External reference */
                            }
                            ElseIf ((_T_G == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ35.TTSP = Arg1
                                    Notify (\_SB.TZ35, 0x81) // Information Change
                                }

                                Return (\_SB.TZ35._TSP) /* External reference */
                            }
                            ElseIf ((_T_G == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ35.TTC1 = Arg1
                                    Notify (\_SB.TZ35, 0x81) // Information Change
                                }

                                Return (\_SB.TZ35._TC1) /* External reference */
                            }
                            ElseIf ((_T_G == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ35.TTC2 = Arg1
                                    Notify (\_SB.TZ35, 0x81) // Information Change
                                }

                                Return (\_SB.TZ35._TC2) /* External reference */
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x24))
                    {
                        While (One)
                        {
                            Name (_T_H, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_H = ToInteger (Arg3)
                            If ((_T_H == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ36.TPSV = Arg1
                                    Notify (\_SB.TZ36, 0x81) // Information Change
                                }

                                Return (\_SB.TZ36._PSV) /* External reference */
                            }
                            ElseIf ((_T_H == One))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ36.TCRT = Arg1
                                    Notify (\_SB.TZ36, 0x81) // Information Change
                                }

                                Return (\_SB.TZ36._CRT) /* External reference */
                            }
                            ElseIf ((_T_H == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ36.TTSP = Arg1
                                    Notify (\_SB.TZ36, 0x81) // Information Change
                                }

                                Return (\_SB.TZ36._TSP) /* External reference */
                            }
                            ElseIf ((_T_H == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ36.TTC1 = Arg1
                                    Notify (\_SB.TZ36, 0x81) // Information Change
                                }

                                Return (\_SB.TZ36._TC1) /* External reference */
                            }
                            ElseIf ((_T_H == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ36.TTC2 = Arg1
                                    Notify (\_SB.TZ36, 0x81) // Information Change
                                }

                                Return (\_SB.TZ36._TC2) /* External reference */
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x25))
                    {
                        While (One)
                        {
                            Name (_T_I, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_I = ToInteger (Arg3)
                            If ((_T_I == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ37.TPSV = Arg1
                                    Notify (\_SB.TZ37, 0x81) // Information Change
                                }

                                Return (\_SB.TZ37._PSV) /* External reference */
                            }
                            ElseIf ((_T_I == One))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ37.TCRT = Arg1
                                    Notify (\_SB.TZ37, 0x81) // Information Change
                                }

                                Return (\_SB.TZ37._CRT) /* External reference */
                            }
                            ElseIf ((_T_I == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ37.TTSP = Arg1
                                    Notify (\_SB.TZ37, 0x81) // Information Change
                                }

                                Return (\_SB.TZ37._TSP) /* External reference */
                            }
                            ElseIf ((_T_I == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ37.TTC1 = Arg1
                                    Notify (\_SB.TZ37, 0x81) // Information Change
                                }

                                Return (\_SB.TZ37._TC1) /* External reference */
                            }
                            ElseIf ((_T_I == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ37.TTC2 = Arg1
                                    Notify (\_SB.TZ37, 0x81) // Information Change
                                }

                                Return (\_SB.TZ37._TC2) /* External reference */
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x26))
                    {
                        While (One)
                        {
                            Name (_T_J, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_J = ToInteger (Arg3)
                            If ((_T_J == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ38.TPSV = Arg1
                                    Notify (\_SB.TZ38, 0x81) // Information Change
                                }

                                Return (\_SB.TZ38._PSV) /* External reference */
                            }
                            ElseIf ((_T_J == One))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ38.TCRT = Arg1
                                    Notify (\_SB.TZ38, 0x81) // Information Change
                                }

                                Return (\_SB.TZ38._CRT) /* External reference */
                            }
                            ElseIf ((_T_J == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ38.TTSP = Arg1
                                    Notify (\_SB.TZ38, 0x81) // Information Change
                                }

                                Return (\_SB.TZ38._TSP) /* External reference */
                            }
                            ElseIf ((_T_J == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ38.TTC1 = Arg1
                                    Notify (\_SB.TZ38, 0x81) // Information Change
                                }

                                Return (\_SB.TZ38._TC1) /* External reference */
                            }
                            ElseIf ((_T_J == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ38.TTC2 = Arg1
                                    Notify (\_SB.TZ38, 0x81) // Information Change
                                }

                                Return (\_SB.TZ38._TC2) /* External reference */
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x63))
                    {
                        While (One)
                        {
                            Name (_T_K, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_K = ToInteger (Arg3)
                            If ((_T_K == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ99.TPSV = Arg1
                                    Notify (\_SB.TZ99, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ99._PSV ())
                            }
                            ElseIf ((_T_K == One))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ99.TCRT = Arg1
                                    Notify (\_SB.TZ99, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ99._CRT ())
                            }
                            ElseIf ((_T_K == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ99.TTSP = Arg1
                                    Notify (\_SB.TZ99, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ99._TSP ())
                            }
                            ElseIf ((_T_K == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ99.TTC1 = Arg1
                                    Notify (\_SB.TZ99, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ99._TC1 ())
                            }
                            ElseIf ((_T_K == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ99.TTC2 = Arg1
                                    Notify (\_SB.TZ99, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ99._TC2 ())
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x33))
                    {
                        While (One)
                        {
                            Name (_T_L, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_L = ToInteger (Arg3)
                            If ((_T_L == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ51.TPSV = Arg1
                                    Notify (\_SB.TZ51, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ51._PSV ())
                            }
                            ElseIf ((_T_L == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ51.TTSP = Arg1
                                    Notify (\_SB.TZ51, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ51._TSP ())
                            }
                            ElseIf ((_T_L == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ51.TTC1 = Arg1
                                    Notify (\_SB.TZ51, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ51._TC1 ())
                            }
                            ElseIf ((_T_L == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ51.TTC2 = Arg1
                                    Notify (\_SB.TZ51, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ51._TC2 ())
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x34))
                    {
                        While (One)
                        {
                            Name (_T_M, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_M = ToInteger (Arg3)
                            If ((_T_M == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ52.TPSV = Arg1
                                    Notify (\_SB.TZ52, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ52._PSV ())
                            }
                            ElseIf ((_T_M == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ52.TTSP = Arg1
                                    Notify (\_SB.TZ52, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ52._TSP ())
                            }
                            ElseIf ((_T_M == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ52.TTC1 = Arg1
                                    Notify (\_SB.TZ52, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ52._TC1 ())
                            }
                            ElseIf ((_T_M == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ52.TTC2 = Arg1
                                    Notify (\_SB.TZ52, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ52._TC2 ())
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x35))
                    {
                        While (One)
                        {
                            Name (_T_N, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_N = ToInteger (Arg3)
                            If ((_T_N == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ53.TPSV = Arg1
                                    Notify (\_SB.TZ53, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ53._PSV) /* External reference */
                            }
                            ElseIf ((_T_N == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ53.TTSP = Arg1
                                    Notify (\_SB.TZ53, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ53._TSP) /* External reference */
                            }
                            ElseIf ((_T_N == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ53.TTC1 = Arg1
                                    Notify (\_SB.TZ53, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ53._TC1) /* External reference */
                            }
                            ElseIf ((_T_N == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ53.TTC2 = Arg1
                                    Notify (\_SB.TZ53, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ53._TC2) /* External reference */
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x36))
                    {
                        While (One)
                        {
                            Name (_T_O, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_O = ToInteger (Arg3)
                            If ((_T_O == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ54.TPSV = Arg1
                                    Notify (\_SB.TZ54, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ54._PSV) /* External reference */
                            }
                            ElseIf ((_T_O == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ54.TTSP = Arg1
                                    Notify (\_SB.TZ54, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ54._TSP) /* External reference */
                            }
                            ElseIf ((_T_O == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ54.TTC1 = Arg1
                                    Notify (\_SB.TZ54, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ54._TC1) /* External reference */
                            }
                            ElseIf ((_T_O == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ54.TTC2 = Arg1
                                    Notify (\_SB.TZ54, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ54._TC2) /* External reference */
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x37))
                    {
                        While (One)
                        {
                            Name (_T_P, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_P = ToInteger (Arg3)
                            If ((_T_P == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ55.TPSV = Arg1
                                    Notify (\_SB.TZ55, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ55._PSV) /* External reference */
                            }
                            ElseIf ((_T_P == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ55.TTSP = Arg1
                                    Notify (\_SB.TZ55, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ55._TSP) /* External reference */
                            }
                            ElseIf ((_T_P == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ55.TTC1 = Arg1
                                    Notify (\_SB.TZ55, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ55._TC1) /* External reference */
                            }
                            ElseIf ((_T_P == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ55.TTC2 = Arg1
                                    Notify (\_SB.TZ55, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ55._TC2) /* External reference */
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x38))
                    {
                        While (One)
                        {
                            Name (_T_Q, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_Q = ToInteger (Arg3)
                            If ((_T_Q == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ56.TPSV = Arg1
                                    Notify (\_SB.TZ56, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ56._PSV) /* External reference */
                            }
                            ElseIf ((_T_Q == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ56.TTSP = Arg1
                                    Notify (\_SB.TZ56, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ56._TSP) /* External reference */
                            }
                            ElseIf ((_T_Q == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ56.TTC1 = Arg1
                                    Notify (\_SB.TZ56, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ56._TC1) /* External reference */
                            }
                            ElseIf ((_T_Q == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ56.TTC2 = Arg1
                                    Notify (\_SB.TZ56, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ56._TC2) /* External reference */
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x39))
                    {
                        While (One)
                        {
                            Name (_T_R, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_R = ToInteger (Arg3)
                            If ((_T_R == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ57.TPSV = Arg1
                                    Notify (\_SB.TZ57, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ57._PSV ())
                            }
                            ElseIf ((_T_R == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ57.TTSP = Arg1
                                    Notify (\_SB.TZ57, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ57._TSP ())
                            }
                            ElseIf ((_T_R == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ57.TTC1 = Arg1
                                    Notify (\_SB.TZ57, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ57._TC1 ())
                            }
                            ElseIf ((_T_R == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ57.TTC2 = Arg1
                                    Notify (\_SB.TZ57, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ57._TC2 ())
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x3A))
                    {
                        While (One)
                        {
                            Name (_T_S, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_S = ToInteger (Arg3)
                            If ((_T_S == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ58.TPSV = Arg1
                                    Notify (\_SB.TZ58, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ58._PSV ())
                            }
                            ElseIf ((_T_S == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ58.TTSP = Arg1
                                    Notify (\_SB.TZ58, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ58._TSP ())
                            }
                            ElseIf ((_T_S == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ58.TTC1 = Arg1
                                    Notify (\_SB.TZ58, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ58._TC1 ())
                            }
                            ElseIf ((_T_S == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ58.TTC2 = Arg1
                                    Notify (\_SB.TZ58, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ58._TC2 ())
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x3B))
                    {
                        While (One)
                        {
                            Name (_T_T, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_T = ToInteger (Arg3)
                            If ((_T_T == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ59.TPSV = Arg1
                                    Notify (\_SB.TZ59, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ59._PSV ())
                            }
                            ElseIf ((_T_T == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ59.TTSP = Arg1
                                    Notify (\_SB.TZ59, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ59._TSP ())
                            }
                            ElseIf ((_T_T == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ59.TTC1 = Arg1
                                    Notify (\_SB.TZ59, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ59._TC1 ())
                            }
                            ElseIf ((_T_T == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ59.TTC2 = Arg1
                                    Notify (\_SB.TZ59, 0x81) // Thermal Trip Point Change
                                }

                                Return (\_SB.TZ59._TC2 ())
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x3C))
                    {
                        While (One)
                        {
                            Name (_T_U, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_U = ToInteger (Arg3)
                            If ((_T_U == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ60.TPSV = Arg1
                                    Notify (\_SB.TZ60, 0x81) // Information Change
                                }

                                Return (\_SB.TZ60._PSV) /* External reference */
                            }
                            ElseIf ((_T_U == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ60.TTSP = Arg1
                                    Notify (\_SB.TZ60, 0x81) // Information Change
                                }

                                Return (\_SB.TZ60._TSP) /* External reference */
                            }
                            ElseIf ((_T_U == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ60.TTC1 = Arg1
                                    Notify (\_SB.TZ60, 0x81) // Information Change
                                }

                                Return (\_SB.TZ60._TC1) /* External reference */
                            }
                            ElseIf ((_T_U == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ60.TTC2 = Arg1
                                    Notify (\_SB.TZ60, 0x81) // Information Change
                                }

                                Return (\_SB.TZ60._TC2) /* External reference */
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x3D))
                    {
                        While (One)
                        {
                            Name (_T_V, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_V = ToInteger (Arg3)
                            If ((_T_V == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ61.TPSV = Arg1
                                    Notify (\_SB.TZ61, 0x81) // Information Change
                                }

                                Return (\_SB.TZ61._PSV) /* External reference */
                            }
                            ElseIf ((_T_V == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ61.TTSP = Arg1
                                    Notify (\_SB.TZ61, 0x81) // Information Change
                                }

                                Return (\_SB.TZ61._TSP) /* External reference */
                            }
                            ElseIf ((_T_V == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ61.TTC1 = Arg1
                                    Notify (\_SB.TZ61, 0x81) // Information Change
                                }

                                Return (\_SB.TZ61._TC1) /* External reference */
                            }
                            ElseIf ((_T_V == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ61.TTC2 = Arg1
                                    Notify (\_SB.TZ61, 0x81) // Information Change
                                }

                                Return (\_SB.TZ61._TC2) /* External reference */
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    ElseIf ((_T_0 == 0x3E))
                    {
                        While (One)
                        {
                            Name (_T_W, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_W = ToInteger (Arg3)
                            If ((_T_W == Zero))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ62.TPSV = Arg1
                                    Notify (\_SB.TZ62, 0x81) // Information Change
                                }

                                Return (\_SB.TZ62._PSV) /* External reference */
                            }
                            ElseIf ((_T_W == 0x02))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ62.TTSP = Arg1
                                    Notify (\_SB.TZ62, 0x81) // Information Change
                                }

                                Return (\_SB.TZ62._TSP) /* External reference */
                            }
                            ElseIf ((_T_W == 0x03))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ62.TTC1 = Arg1
                                    Notify (\_SB.TZ62, 0x81) // Information Change
                                }

                                Return (\_SB.TZ62._TC1) /* External reference */
                            }
                            ElseIf ((_T_W == 0x04))
                            {
                                If (Arg2)
                                {
                                    \_SB.TZ62.TTC2 = Arg1
                                    Notify (\_SB.TZ62, 0x81) // Information Change
                                }

                                Return (\_SB.TZ62._TC2) /* External reference */
                            }
                            Else
                            {
                                Return (0xFFFF)
                            }

                            Break
                        }
                    }
                    Else
                    {
                        Return (0xFFFF)
                    }

                    Break
                }
            }

            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.IPCC, 
            })
            Method (_SUB, 0, NotSerialized)  // _SUB: Subsystem ID
            {
                If ((\_SB.PSUB == "IDP06490"))
                {
                    Return ("IDP06490")
                }
                ElseIf ((\_SB.PSUB == "CRD06490"))
                {
                    Return ("CRD06490")
                }
                ElseIf ((\_SB.PSUB == "IOT06490"))
                {
                    Return ("IDP06490")
                }
            }

            Method (_HRV, 0, NotSerialized)  // _HRV: Hardware Revision
            {
                Name (RETV, Zero)
                RETV = ((\_SB.SJTG >> 0x0C) & 0xFFFF)
                Return (RETV) /* \_SB_.PEP0._HRV.RETV */
            }

            Method (_DSM, 4, NotSerialized)  // _DSM: Device-Specific Method
            {
                While (One)
                {
                    Name (_T_0, Buffer (0x01)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                    {
                         0x00                                             // .
                    })
                    CopyObject (ToBuffer (Arg0), _T_0) /* \_SB_.PEP0._DSM._T_0 */
                    If ((_T_0 == ToUUID ("8d5ca34c-ae83-4a2a-9dd1-a74ffead548b") /* Unknown UUID */))
                    {
                        While (One)
                        {
                            Name (_T_1, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_1 = ToInteger (Arg2)
                            If ((_T_1 == Zero))
                            {
                                While (One)
                                {
                                    Name (_T_2, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                                    _T_2 = ToInteger (Arg1)
                                    If ((_T_2 == Zero))
                                    {
                                        Return (0x7E)
                                    }

                                    Break
                                }

                                Return (Zero)
                            }
                            ElseIf ((_T_1 == One))
                            {
                                Name (SUBI, Package (0x06)
                                {
                                    Package (0x03)
                                    {
                                        "adsp", 
                                        One, 
                                        0x02
                                    }, 

                                    Package (0x03)
                                    {
                                        "slpi", 
                                        Zero, 
                                        0x03
                                    }, 

                                    Package (0x03)
                                    {
                                        "cdsp", 
                                        One, 
                                        0x04
                                    }, 

                                    Package (0x03)
                                    {
                                        "modem", 
                                        One, 
                                        0x05
                                    }, 

                                    Package (0x03)
                                    {
                                        "spss", 
                                        Zero, 
                                        0x06
                                    }, 

                                    Package (0x03)
                                    {
                                        "wpss", 
                                        One, 
                                        0x07
                                    }
                                })
                                Return (SUBI) /* \_SB_.PEP0._DSM.SUBI */
                            }
                            ElseIf ((_T_1 == 0x02))
                            {
                                If (CondRefOf (\_SB.ADSP))
                                {
                                    If (CondRefOf (\_SB.ADSP._STA))
                                    {
                                        Return (\_SB.ADSP._STA ())
                                    }
                                    Else
                                    {
                                        Return (0x0F)
                                    }
                                }
                                Else
                                {
                                    Return (Zero)
                                }
                            }
                            ElseIf ((_T_1 == 0x03))
                            {
                                If (CondRefOf (\_SB.SCSS))
                                {
                                    If (CondRefOf (\_SB.SCSS._STA))
                                    {
                                        Return (\_SB.SCSS._STA) /* External reference */
                                    }
                                    Else
                                    {
                                        Return (0x0F)
                                    }
                                }
                                Else
                                {
                                    Return (Zero)
                                }
                            }
                            ElseIf ((_T_1 == 0x04))
                            {
                                If (CondRefOf (\_SB.NSP0))
                                {
                                    If (CondRefOf (\_SB.NSP0._STA))
                                    {
                                        Return (\_SB.NSP0._STA ())
                                    }
                                    Else
                                    {
                                        Return (0x0F)
                                    }
                                }
                                Else
                                {
                                    Return (Zero)
                                }
                            }
                            ElseIf ((_T_1 == 0x05))
                            {
                                If (CondRefOf (\_SB.AMSS))
                                {
                                    If (CondRefOf (\_SB.AMSS._STA))
                                    {
                                        Return (\_SB.AMSS._STA ())
                                    }
                                    Else
                                    {
                                        Return (0x0F)
                                    }
                                }
                                Else
                                {
                                    Return (Zero)
                                }
                            }
                            ElseIf ((_T_1 == 0x06))
                            {
                                If (CondRefOf (\_SB.SPSS))
                                {
                                    If (CondRefOf (\_SB.SPSS._STA))
                                    {
                                        Return (\_SB.SPSS._STA) /* External reference */
                                    }
                                    Else
                                    {
                                        Return (0x0F)
                                    }
                                }
                                Else
                                {
                                    Return (Zero)
                                }
                            }
                            ElseIf ((_T_1 == 0x07))
                            {
                                If (CondRefOf (\_SB.WPSS))
                                {
                                    If (CondRefOf (\_SB.WPSS._STA))
                                    {
                                        Return (\_SB.WPSS._STA ())
                                    }
                                    Else
                                    {
                                        Return (0x0F)
                                    }
                                }
                                Else
                                {
                                    Return (Zero)
                                }
                            }
                            Else
                            {
                                Return (Zero)
                            }

                            Break
                        }
                    }
                    Else
                    {
                        Return (Zero)
                    }

                    Break
                }
            }

            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Return (Buffer (0x65)
                {
                    /* 0000 */  0x89, 0x06, 0x00, 0x11, 0x01, 0x1A, 0x02, 0x00,  // ........
                    /* 0008 */  0x00, 0x89, 0x06, 0x00, 0x11, 0x01, 0x1C, 0x02,  // ........
                    /* 0010 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x11, 0x01, 0x1B,  // ........
                    /* 0018 */  0x02, 0x00, 0x00, 0x89, 0x06, 0x00, 0x11, 0x01,  // ........
                    /* 0020 */  0x1D, 0x02, 0x00, 0x00, 0x89, 0x06, 0x00, 0x01,  // ........
                    /* 0028 */  0x01, 0x25, 0x00, 0x00, 0x00, 0x89, 0x06, 0x00,  // .%......
                    /* 0030 */  0x01, 0x01, 0x3E, 0x00, 0x00, 0x00, 0x89, 0x06,  // ..>.....
                    /* 0038 */  0x00, 0x01, 0x01, 0x3F, 0x00, 0x00, 0x00, 0x89,  // ...?....
                    /* 0040 */  0x06, 0x00, 0x01, 0x01, 0x33, 0x00, 0x00, 0x00,  // ....3...
                    /* 0048 */  0x89, 0x06, 0x00, 0x03, 0x01, 0x65, 0x02, 0x00,  // .....e..
                    /* 0050 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x0D, 0x01,  // ........
                    /* 0058 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x13,  // ........
                    /* 0060 */  0x01, 0x00, 0x00, 0x79, 0x00                     // ...y.
                })
            }

            Field (\_SB.ABD.ROP1, BufferAcc, NoLock, Preserve)
            {
                Connection (
                    I2cSerialBusV2 (0x0001, ControllerInitiated, 0x00000000,
                        AddressingMode7Bit, "\\_SB.ABD",
                        0x00, ResourceConsumer, , Exclusive,
                        )
                ), 
                AccessAs (BufferAcc, AttribRawBytes (0x15)), 
                FLD0,   168
            }

            Method (GEPT, 0, NotSerialized)
            {
                Name (BUFF, Buffer (0x04){})
                CreateByteField (BUFF, Zero, STAT)
                CreateWordField (BUFF, 0x02, DATA)
                DATA = One
                Return (DATA) /* \_SB_.PEP0.GEPT.DATA */
            }

            Name (ROST, Zero)
            Method (NPUR, 1, NotSerialized)
            {
                \_SB.AGR0._PUR [One] = Arg0
                Notify (\_SB.AGR0, 0x80) // Status Change
            }

            Method (INTR, 0, NotSerialized)
            {
                Name (RBUF, Package (0x18)
                {
                    0x02, 
                    One, 
                    0x03, 
                    One, 
                    0x06, 
                    0x17911008, 
                    One, 
                    Zero, 
                    0x86000000, 
                    0x00200000, 
                    Zero, 
                    Zero, 
                    0x0C300000, 
                    0x1000, 
                    Zero, 
                    Zero, 
                    0x01FD4000, 
                    0x08, 
                    Zero, 
                    Zero, 
                    0x17C0000C, 
                    Zero, 
                    Zero, 
                    Zero
                })
                Return (RBUF) /* \_SB_.PEP0.INTR.RBUF */
            }

            Method (STND, 0, NotSerialized)
            {
                Return (STNX) /* \_SB_.PEP0.STNX */
            }

            Name (STNX, Package (0x0C)
            {
                "DMPO", 
                "MMVD", 
                "DMSB", 
                "DMPA", 
                "DMPB", 
                "DMDS", 
                "DMPL", 
                "DMWE", 
                "XMPL", 
                "XMPT", 
                "DMEP", 
                "XMDH"
            })
            Name (DCVS, Zero)
            Method (PGDS, 0, NotSerialized)
            {
                Return (DCVS) /* \_SB_.PEP0.DCVS */
            }

            Name (PPPP, Package (0x36)
            {
                Package (0x01)
                {
                    "PPP_RESOURCE_ID_SMPS1_B"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_SMPS2_B"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_SMPS7_B"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_SMPS8_B"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_SMPS1_C"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO1_B"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO2_B"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO3_B"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO6_B"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO7_B"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO8_B"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO9_B"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO11_B"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO12_B"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO13_B"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO14_B"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO15_B"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO16_B"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO17_B"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO18_B"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO19_B"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO1_C"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO2_C"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO3_C"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO4_C"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO5_C"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO6_C"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO7_C"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO8_C"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO9_C"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO10_C"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO11_C"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO12_C"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO13_C"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO1_P"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO2_P"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO3_P"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO4_P"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO5_P"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO6_P"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO7_P"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO1_Q"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO2_Q"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO3_Q"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO4_Q"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO5_Q"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO6_Q"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_LDO7_Q"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_CXO_BUFFERS_BBCLK2_A"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_CXO_BUFFERS_RFCLK1_A"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_CXO_BUFFERS_RFCLK2_A"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_BUCK_BOOST1_C"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_FIXED_VREG1"
                }, 

                Package (0x01)
                {
                    "PPP_RESOURCE_ID_FIXED_VREG2"
                }
            })
            Method (PPPM, 0, NotSerialized)
            {
                Return (PPPP) /* \_SB_.PEP0.PPPP */
            }

            Name (PRRP, Package (0x00){})
            Method (PPRR, 0, NotSerialized)
            {
                Return (PRRP) /* \_SB_.PEP0.PRRP */
            }

            Name (FPDP, Zero)
            Method (FPMD, 0, NotSerialized)
            {
                Return (FPDP) /* \_SB_.PEP0.FPDP */
            }

            Method (DPRF, 0, NotSerialized)
            {
                Return (\_SB.DPP0) /* External reference */
            }

            Method (DMRF, 0, NotSerialized)
            {
                Return (\_SB.DPP1) /* External reference */
            }

            Method (MPRF, 0, NotSerialized)
            {
                Return (\_SB.MPP0) /* External reference */
            }

            Method (MMRF, 0, NotSerialized)
            {
                Return (\_SB.MPP1) /* External reference */
            }
        }

        Scope (\_SB.PEP0)
        {
            Method (PREL, 0, NotSerialized)
            {
                Name (PREX, Package (0x03)
                {
                    "DM0G", 
                    "DM4G", 
                    "DM5G"
                })
                Return (PREX) /* \_SB_.PEP0.PREL.PREX */
            }
        }

        Scope (\_SB.PEP0)
        {
            Method (PEPH, 0, NotSerialized)
            {
                Return (Package (0x01)
                {
                    "ACPI\\VEN_QCOM&DEV_0A17"
                })
            }
        }

        Scope (\_SB.PEP0)
        {
            Method (APMD, 0, NotSerialized)
            {
                Return (APCC) /* \_SB_.PEP0.APCC */
            }

            Name (APCC, Package (0x01)
            {
                Package (0x06)
                {
                    "DEVICE", 
                    "\\_SB.ADSP.ADCM.AUCD.QCRT", 
                    Package (0x03)
                    {
                        "COMPONENT", 
                        Zero, 
                        Package (0x02)
                        {
                            "FSTATE", 
                            Zero
                        }
                    }, 

                    Package (0x03)
                    {
                        "DSTATE", 
                        Zero, 
                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_xo", 
                                0x80
                            }
                        }
                    }, 

                    Package (0x03)
                    {
                        "DSTATE", 
                        One, 
                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_xo", 
                                Zero
                            }
                        }
                    }, 

                    Package (0x03)
                    {
                        "DSTATE", 
                        0x02, 
                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_xo", 
                                Zero
                            }
                        }
                    }
                }
            })
        }

        Scope (\_SB.PEP0)
        {
            Method (G0MD, 0, NotSerialized)
            {
                Name (GPCC, Package (0x01)
                {
                    Package (0x04)
                    {
                        "DEVICE", 
                        0x82, 
                        "\\_SB.GPU0", 
                        Package (0x0C)
                        {
                            "COMPONENT", 
                            Zero, 
                            Package (0x03)
                            {
                                "FSTATE", 
                                Zero, 
                                Package (0x1B)
                                {
                                    "EXIT", 
                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_APPSS_PROC", 
                                            "ICBID_SLAVE_DISPLAY_CFG", 
                                            0x047868C0, 
                                            Zero
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "gcc_disp_xo_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "gcc_disp_ahb_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_xo_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "dsi_phy_refgen_en", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "FOOTSWITCH", 
                                        Package (0x03)
                                        {
                                            "disp_cc_mdss_core_gdsc", 
                                            One, 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_MDP0", 
                                            "ICBID_SLAVE_EBI1", 
                                            0x2FAF0800, 
                                            Zero
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "gcc_disp_hf_axi_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_rscc_ahb_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_rscc_vsync_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_ahb_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_vsync_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_mdp_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_pclk0_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_esc0_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_byte0_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_byte0_intf_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x04)
                                        {
                                            "disp_cc_mdss_mdp_clk", 
                                            0x03, 
                                            0x16A65700, 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_APPSS_PROC", 
                                            "ICBID_SLAVE_DISPLAY_CFG", 
                                            0x047868C0, 
                                            Zero
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "PMICVREGVOTE", 
                                        Package (0x08)
                                        {
                                            "PPP_RESOURCE_ID_LDO6_B", 
                                            One, 
                                            0x00124F80, 
                                            One, 
                                            0x07, 
                                            Zero, 
                                            "HLOS_DRV", 
                                            "REQUIRED"
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "PMICVREGVOTE", 
                                        Package (0x08)
                                        {
                                            "PPP_RESOURCE_ID_LDO10_C", 
                                            One, 
                                            0x000D6D80, 
                                            One, 
                                            0x07, 
                                            Zero, 
                                            "HLOS_DRV", 
                                            "REQUIRED"
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "PMICVREGVOTE", 
                                        Package (0x08)
                                        {
                                            "PPP_RESOURCE_ID_LDO8_C", 
                                            One, 
                                            0x001B7740, 
                                            One, 
                                            0x07, 
                                            Zero, 
                                            "HLOS_DRV", 
                                            "REQUIRED"
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "PMICVREGVOTE", 
                                        Package (0x08)
                                        {
                                            "PPP_RESOURCE_ID_LDO12_C", 
                                            One, 
                                            0x001B7740, 
                                            One, 
                                            0x07, 
                                            Zero, 
                                            "HLOS_DRV", 
                                            "REQUIRED"
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "TLMMGPIO", 
                                        Package (0x06)
                                        {
                                            0x50, 
                                            One, 
                                            One, 
                                            Zero, 
                                            Zero, 
                                            Zero
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "TLMMGPIO", 
                                        Package (0x06)
                                        {
                                            0x51, 
                                            One, 
                                            Zero, 
                                            One, 
                                            Zero, 
                                            Zero
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "PMICGPIO", 
                                        Package (0x08)
                                        {
                                            "IOCTL_PM_GPIO_CONFIG_DIGITAL_OUTPUT", 
                                            0x02, 
                                            0x07, 
                                            Zero, 
                                            Zero, 
                                            0x02, 
                                            One, 
                                            0x04
                                        }
                                    }
                                }
                            }, 

                            Package (0x02)
                            {
                                "FSTATE", 
                                One
                            }, 

                            Package (0x02)
                            {
                                "INIT_FSTATE", 
                                Zero
                            }, 

                            Package (0x02)
                            {
                                "PRELOAD_FSTATE", 
                                One
                            }, 

                            Package (0x04)
                            {
                                "PSTATE_SET", 
                                Zero, 
                                Package (0x02)
                                {
                                    "PSTATE", 
                                    Zero
                                }, 

                                Package (0x02)
                                {
                                    "PRELOAD_PSTATE", 
                                    Zero
                                }
                            }, 

                            Package (0x04)
                            {
                                "PSTATE_SET", 
                                One, 
                                Package (0x0D)
                                {
                                    "PSTATE", 
                                    Zero, 
                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_MDP0", 
                                            "ICBID_SLAVE_EBI1", 
                                            0x2FAF0800, 
                                            Zero
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "gcc_disp_hf_axi_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_rscc_ahb_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_rscc_vsync_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_ahb_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_vsync_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_mdp_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_pclk0_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_esc0_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_byte0_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_byte0_intf_clk", 
                                            One
                                        }
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PRELOAD_PSTATE", 
                                    Zero
                                }
                            }, 

                            Package (0x04)
                            {
                                "PSTATE_SET", 
                                0x02, 
                                Package (0x04)
                                {
                                    "PSTATE", 
                                    Zero, 
                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x04)
                                        {
                                            "disp_cc_mdss_mdp_clk", 
                                            0x03, 
                                            0x16A65700, 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_APPSS_PROC", 
                                            "ICBID_SLAVE_DISPLAY_CFG", 
                                            0x047868C0, 
                                            Zero
                                        }
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PRELOAD_PSTATE", 
                                    Zero
                                }
                            }, 

                            Package (0x04)
                            {
                                "PSTATE_SET", 
                                0x03, 
                                Package (0x04)
                                {
                                    "PSTATE", 
                                    Zero, 
                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_MDP0", 
                                            "ICBID_SLAVE_MNOC_HF_MEM_NOC", 
                                            Zero, 
                                            0x77359400
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_MNOC_HF_MEM_NOC", 
                                            "ICBID_SLAVE_EBI1", 
                                            Zero, 
                                            0x77359400
                                        }
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PRELOAD_PSTATE", 
                                    Zero
                                }
                            }, 

                            Package (0x04)
                            {
                                "PSTATE_SET", 
                                0x04, 
                                Package (0x04)
                                {
                                    "PSTATE", 
                                    Zero, 
                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_MDP0", 
                                            "ICBID_SLAVE_MNOC_HF_MEM_NOC", 
                                            Zero, 
                                            Zero
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_MNOC_HF_MEM_NOC", 
                                            "ICBID_SLAVE_EBI1", 
                                            0x2FAF0800, 
                                            Zero
                                        }
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PRELOAD_PSTATE", 
                                    Zero
                                }
                            }, 

                            Package (0x04)
                            {
                                "PSTATE_SET", 
                                0x05, 
                                Package (0x07)
                                {
                                    "PSTATE", 
                                    Zero, 
                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_rscc_ahb_clk", 
                                            0x02
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_ahb_clk", 
                                            0x02
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "gcc_disp_xo_clk", 
                                            0x02
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "gcc_disp_ahb_clk", 
                                            0x02
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_xo_clk", 
                                            0x02
                                        }
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PRELOAD_PSTATE", 
                                    Zero
                                }
                            }
                        }
                    }
                })
                Return (GPCC) /* \_SB_.PEP0.G0MD.GPCC */
            }

            Method (G4MD, 0, NotSerialized)
            {
                Name (GPCC, Package (0x01)
                {
                    Package (0x04)
                    {
                        "DEVICE", 
                        0x82, 
                        "\\_SB.GPU0", 
                        Package (0x0B)
                        {
                            "COMPONENT", 
                            0x04, 
                            Package (0x03)
                            {
                                "FSTATE", 
                                Zero, 
                                Package (0x16)
                                {
                                    "EXIT", 
                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_APPSS_PROC", 
                                            "ICBID_SLAVE_DISPLAY_CFG", 
                                            0x047868C0, 
                                            Zero
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "gcc_disp_xo_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "gcc_disp_ahb_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_xo_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "FOOTSWITCH", 
                                        Package (0x03)
                                        {
                                            "disp_cc_mdss_core_gdsc", 
                                            One, 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_MDP0", 
                                            "ICBID_SLAVE_EBI1", 
                                            0x2FAF0800, 
                                            Zero
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "gcc_disp_hf_axi_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_rscc_ahb_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_rscc_vsync_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_ahb_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_vsync_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_mdp_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_edp_aux_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_edp_pixel_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_edp_link_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_edp_link_intf_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x04)
                                        {
                                            "disp_cc_mdss_mdp_clk", 
                                            0x03, 
                                            0x16A65700, 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_APPSS_PROC", 
                                            "ICBID_SLAVE_DISPLAY_CFG", 
                                            0x047868C0, 
                                            Zero
                                        }
                                    }, 

                                    Package (0x03)
                                    {
                                        "TLMMGPIO_V2", 
                                        One, 
                                        Package (0x06)
                                        {
                                            0x3C, 
                                            Zero, 
                                            One, 
                                            Zero, 
                                            Zero, 
                                            Zero
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "PMICVREGVOTE", 
                                        Package (0x08)
                                        {
                                            "PPP_RESOURCE_ID_LDO10_C", 
                                            One, 
                                            0x000D6D80, 
                                            One, 
                                            0x07, 
                                            Zero, 
                                            "HLOS_DRV", 
                                            "REQUIRED"
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "PMICVREGVOTE", 
                                        Package (0x08)
                                        {
                                            "PPP_RESOURCE_ID_LDO6_B", 
                                            One, 
                                            0x00124F80, 
                                            One, 
                                            0x07, 
                                            Zero, 
                                            "HLOS_DRV", 
                                            "REQUIRED"
                                        }
                                    }
                                }
                            }, 

                            Package (0x02)
                            {
                                "FSTATE", 
                                One
                            }, 

                            Package (0x02)
                            {
                                "INIT_FSTATE", 
                                Zero
                            }, 

                            Package (0x02)
                            {
                                "PRELOAD_FSTATE", 
                                One
                            }, 

                            Package (0x04)
                            {
                                "PSTATE_SET", 
                                Zero, 
                                Package (0x02)
                                {
                                    "PSTATE", 
                                    Zero
                                }, 

                                Package (0x02)
                                {
                                    "PRELOAD_PSTATE", 
                                    Zero
                                }
                            }, 

                            Package (0x04)
                            {
                                "PSTATE_SET", 
                                One, 
                                Package (0x04)
                                {
                                    "PSTATE", 
                                    Zero, 
                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x04)
                                        {
                                            "disp_cc_mdss_mdp_clk", 
                                            0x03, 
                                            0x16A65700, 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_APPSS_PROC", 
                                            "ICBID_SLAVE_DISPLAY_CFG", 
                                            0x047868C0, 
                                            Zero
                                        }
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PRELOAD_PSTATE", 
                                    Zero
                                }
                            }, 

                            Package (0x04)
                            {
                                "PSTATE_SET", 
                                0x02, 
                                Package (0x04)
                                {
                                    "PSTATE", 
                                    Zero, 
                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_MDP0", 
                                            "ICBID_SLAVE_MNOC_HF_MEM_NOC", 
                                            Zero, 
                                            0x77359400
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_MNOC_HF_MEM_NOC", 
                                            "ICBID_SLAVE_EBI1", 
                                            Zero, 
                                            0x77359400
                                        }
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PRELOAD_PSTATE", 
                                    Zero
                                }
                            }, 

                            Package (0x04)
                            {
                                "PSTATE_SET", 
                                0x03, 
                                Package (0x04)
                                {
                                    "PSTATE", 
                                    Zero, 
                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_MDP0", 
                                            "ICBID_SLAVE_MNOC_HF_MEM_NOC", 
                                            Zero, 
                                            Zero
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_MNOC_HF_MEM_NOC", 
                                            "ICBID_SLAVE_EBI1", 
                                            0x2FAF0800, 
                                            Zero
                                        }
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PRELOAD_PSTATE", 
                                    Zero
                                }
                            }, 

                            Package (0x04)
                            {
                                "PSTATE_SET", 
                                0x04, 
                                Package (0x06)
                                {
                                    "PSTATE", 
                                    Zero, 
                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_edp_aux_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_edp_pixel_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_edp_link_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_edp_link_intf_clk", 
                                            One
                                        }
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PRELOAD_PSTATE", 
                                    Zero
                                }
                            }
                        }
                    }
                })
                Return (GPCC) /* \_SB_.PEP0.G4MD.GPCC */
            }

            Method (G5MD, 0, NotSerialized)
            {
                Name (GPCC, Package (0x01)
                {
                    Package (0x04)
                    {
                        "DEVICE", 
                        0x82, 
                        "\\_SB.GPU0", 
                        Package (0x0B)
                        {
                            "COMPONENT", 
                            0x05, 
                            Package (0x03)
                            {
                                "FSTATE", 
                                Zero, 
                                Package (0x1C)
                                {
                                    "EXIT", 
                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_APPSS_PROC", 
                                            "ICBID_SLAVE_DISPLAY_CFG", 
                                            0x047868C0, 
                                            Zero
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "gcc_disp_xo_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "gcc_disp_ahb_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_xo_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "FOOTSWITCH", 
                                        Package (0x03)
                                        {
                                            "disp_cc_mdss_core_gdsc", 
                                            One, 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_MDP0", 
                                            "ICBID_SLAVE_EBI1", 
                                            0x2FAF0800, 
                                            Zero
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "gcc_disp_hf_axi_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "gcc_usb3_prim_phy_pipe_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "gcc_usb30_prim_sleep_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "gcc_usb3_prim_phy_aux_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "gcc_usb3_prim_phy_com_aux_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_rscc_ahb_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_rscc_vsync_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_ahb_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_vsync_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_mdp_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_dp_aux_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_dp_pixel_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_dp_link_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_dp_link_intf_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x04)
                                        {
                                            "disp_cc_mdss_mdp_clk", 
                                            0x03, 
                                            0x16A65700, 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_APPSS_PROC", 
                                            "ICBID_SLAVE_DISPLAY_CFG", 
                                            0x047868C0, 
                                            Zero
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "PMICVREGVOTE", 
                                        Package (0x08)
                                        {
                                            "PPP_RESOURCE_ID_LDO6_B", 
                                            One, 
                                            0x00124F80, 
                                            One, 
                                            0x07, 
                                            Zero, 
                                            "HLOS_DRV", 
                                            "REQUIRED"
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "PMICVREGVOTE", 
                                        Package (0x08)
                                        {
                                            "PPP_RESOURCE_ID_LDO1_B", 
                                            One, 
                                            0x000D6D80, 
                                            One, 
                                            0x07, 
                                            Zero, 
                                            "HLOS_DRV", 
                                            "REQUIRED"
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "PMICVREGVOTE", 
                                        Package (0x08)
                                        {
                                            "PPP_RESOURCE_ID_LDO1_C", 
                                            One, 
                                            0x001B7740, 
                                            One, 
                                            0x07, 
                                            Zero, 
                                            "HLOS_DRV", 
                                            "REQUIRED"
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "PMICVREGVOTE", 
                                        Package (0x08)
                                        {
                                            "PPP_RESOURCE_ID_LDO2_B", 
                                            One, 
                                            0x002EE000, 
                                            One, 
                                            0x07, 
                                            Zero, 
                                            "HLOS_DRV", 
                                            "REQUIRED"
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "PMICVREGVOTE", 
                                        Package (0x08)
                                        {
                                            "PPP_RESOURCE_ID_LDO10_C", 
                                            One, 
                                            0x000D6D80, 
                                            One, 
                                            0x07, 
                                            Zero, 
                                            "HLOS_DRV", 
                                            "REQUIRED"
                                        }
                                    }
                                }
                            }, 

                            Package (0x02)
                            {
                                "FSTATE", 
                                One
                            }, 

                            Package (0x02)
                            {
                                "INIT_FSTATE", 
                                Zero
                            }, 

                            Package (0x02)
                            {
                                "PRELOAD_FSTATE", 
                                One
                            }, 

                            Package (0x04)
                            {
                                "PSTATE_SET", 
                                Zero, 
                                Package (0x02)
                                {
                                    "PSTATE", 
                                    Zero
                                }, 

                                Package (0x02)
                                {
                                    "PRELOAD_PSTATE", 
                                    Zero
                                }
                            }, 

                            Package (0x04)
                            {
                                "PSTATE_SET", 
                                One, 
                                Package (0x04)
                                {
                                    "PSTATE", 
                                    Zero, 
                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x04)
                                        {
                                            "disp_cc_mdss_mdp_clk", 
                                            0x03, 
                                            0x16A65700, 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_APPSS_PROC", 
                                            "ICBID_SLAVE_DISPLAY_CFG", 
                                            0x047868C0, 
                                            Zero
                                        }
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PRELOAD_PSTATE", 
                                    Zero
                                }
                            }, 

                            Package (0x04)
                            {
                                "PSTATE_SET", 
                                0x02, 
                                Package (0x04)
                                {
                                    "PSTATE", 
                                    Zero, 
                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_MDP0", 
                                            "ICBID_SLAVE_MNOC_HF_MEM_NOC", 
                                            Zero, 
                                            0x77359400
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_MNOC_HF_MEM_NOC", 
                                            "ICBID_SLAVE_EBI1", 
                                            Zero, 
                                            0x77359400
                                        }
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PRELOAD_PSTATE", 
                                    Zero
                                }
                            }, 

                            Package (0x04)
                            {
                                "PSTATE_SET", 
                                0x03, 
                                Package (0x04)
                                {
                                    "PSTATE", 
                                    Zero, 
                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_MDP0", 
                                            "ICBID_SLAVE_MNOC_HF_MEM_NOC", 
                                            Zero, 
                                            Zero
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "BUSARB", 
                                        Package (0x05)
                                        {
                                            0x03, 
                                            "ICBID_MASTER_MNOC_HF_MEM_NOC", 
                                            "ICBID_SLAVE_EBI1", 
                                            0x2FAF0800, 
                                            Zero
                                        }
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PRELOAD_PSTATE", 
                                    Zero
                                }
                            }, 

                            Package (0x04)
                            {
                                "PSTATE_SET", 
                                0x04, 
                                Package (0x06)
                                {
                                    "PSTATE", 
                                    Zero, 
                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_dp_aux_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_dp_pixel_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_dp_link_clk", 
                                            One
                                        }
                                    }, 

                                    Package (0x02)
                                    {
                                        "CLOCK", 
                                        Package (0x02)
                                        {
                                            "disp_cc_mdss_dp_link_intf_clk", 
                                            One
                                        }
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PRELOAD_PSTATE", 
                                    Zero
                                }
                            }
                        }
                    }
                })
                Return (GPCC) /* \_SB_.PEP0.G5MD.GPCC */
            }
        }

        Scope (\_SB.PEP0)
        {
            Method (MPMD, 0, NotSerialized)
            {
                Return (MPCC) /* \_SB_.PEP0.MPCC */
            }

            Name (MPCC, Package (0x00){})
        }

        Scope (\_SB.PEP0)
        {
            Method (OPMD, 0, NotSerialized)
            {
                Return (OPCC) /* \_SB_.PEP0.OPCC */
            }

            Name (OPCC, Package (0x00){})
        }

        Scope (\_SB.PEP0)
        {
            Method (LPMD, 0, NotSerialized)
            {
                Return (LPCC) /* \_SB_.PEP0.LPCC */
            }

            Name (LPCC, Package (0x05)
            {
                Package (0x07)
                {
                    "DEVICE", 
                    "\\_SB.URS0", 
                    Package (0x05)
                    {
                        "COMPONENT", 
                        Zero, 
                        Package (0x02)
                        {
                            "FSTATE", 
                            Zero
                        }, 

                        Package (0x02)
                        {
                            "PSTATE", 
                            Zero
                        }, 

                        Package (0x02)
                        {
                            "PSTATE", 
                            One
                        }
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        Zero
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        One
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        0x02
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        0x03
                    }
                }, 

                Package (0x08)
                {
                    "DEVICE", 
                    "\\_SB.URS0.USB0", 
                    Package (0x04)
                    {
                        "COMPONENT", 
                        Zero, 
                        Package (0x02)
                        {
                            "FSTATE", 
                            Zero
                        }, 

                        Package (0x02)
                        {
                            "PSTATE", 
                            Zero
                        }
                    }, 

                    Package (0x13)
                    {
                        "DSTATE", 
                        Zero, 
                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_C", 
                                One, 
                                0x001B7740, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_B", 
                                One, 
                                0x000DEA80, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO2_B", 
                                One, 
                                0x002EE000, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO6_B", 
                                One, 
                                0x00124F80, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                0x000D6D80, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_gdsc", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_sleep_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_pipe_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_aggre_usb3_prim_axi_clk", 
                                0x08, 
                                0xC8, 
                                0x09
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_cfg_noc_usb3_prim_axi_clk", 
                                0x08, 
                                0xC8, 
                                0x09
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb30_prim_master_clk", 
                                0x08, 
                                0xC8, 
                                0x09
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb30_prim_mock_utmi_clk", 
                                0x08, 
                                0x4B00, 
                                0x07
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb3_prim_phy_aux_clk", 
                                0x08, 
                                0x4B00, 
                                0x07
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb3_prim_phy_com_aux_clk", 
                                0x08, 
                                0x4B00, 
                                0x07
                            }
                        }, 

                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_cx", 
                                0x0100
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_USB3_0", 
                                "ICBID_SLAVE_EBI1", 
                                0x28000000, 
                                0x28000000
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_USB3_0", 
                                0x0BEBC200, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x13)
                    {
                        "DSTATE", 
                        One, 
                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_pipe_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_usb3_prim_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_cfg_noc_usb3_prim_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb30_prim_master_clk", 
                                0x03, 
                                0x2580, 
                                0x05
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_master_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_mock_utmi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_aux_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_com_aux_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_USB3_0", 
                                "ICBID_SLAVE_EBI1", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_USB3_0", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_gdsc", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_cx", 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_C", 
                                One, 
                                0x001B7740, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_B", 
                                One, 
                                0x000DEA80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO2_B", 
                                One, 
                                0x002EE000, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO6_B", 
                                One, 
                                0x00124F80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                0x000D6D80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x13)
                    {
                        "DSTATE", 
                        0x02, 
                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_pipe_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb30_prim_master_clk", 
                                0x03, 
                                0x00927C00, 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_master_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_usb3_prim_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_cfg_noc_usb3_prim_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_mock_utmi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_aux_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_com_aux_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_USB3_0", 
                                "ICBID_SLAVE_EBI1", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_USB3_0", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_gdsc", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_cx", 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_C", 
                                One, 
                                0x001B7740, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_B", 
                                One, 
                                0x000DEA80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO2_B", 
                                One, 
                                0x002EE000, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO6_B", 
                                One, 
                                0x00124F80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                0x000D6D80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x14)
                    {
                        "DSTATE", 
                        0x03, 
                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_sleep_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_pipe_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_usb3_prim_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_cfg_noc_usb3_prim_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb30_prim_master_clk", 
                                0x03, 
                                0x00927C00, 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_master_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_mock_utmi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_aux_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_com_aux_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_USB3_0", 
                                "ICBID_SLAVE_EBI1", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_USB3_0", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_gdsc", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_cx", 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO2_B", 
                                One, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_B", 
                                One, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_C", 
                                One, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO6_B", 
                                One, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "ABANDON_DSTATE", 
                        0x03
                    }
                }, 

                Package (0x08)
                {
                    "DEVICE", 
                    "\\_SB.USB0", 
                    Package (0x04)
                    {
                        "COMPONENT", 
                        Zero, 
                        Package (0x02)
                        {
                            "FSTATE", 
                            Zero
                        }, 

                        Package (0x02)
                        {
                            "PSTATE", 
                            Zero
                        }
                    }, 

                    Package (0x13)
                    {
                        "DSTATE", 
                        Zero, 
                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_C", 
                                One, 
                                0x001B7740, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_B", 
                                One, 
                                0x000DEA80, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO2_B", 
                                One, 
                                0x002EE000, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO6_B", 
                                One, 
                                0x00124F80, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                0x000D6D80, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_gdsc", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_sleep_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_pipe_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_aggre_usb3_prim_axi_clk", 
                                0x08, 
                                0xC8, 
                                0x09
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_cfg_noc_usb3_prim_axi_clk", 
                                0x08, 
                                0xC8, 
                                0x09
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb30_prim_master_clk", 
                                0x08, 
                                0xC8, 
                                0x09
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb30_prim_mock_utmi_clk", 
                                0x08, 
                                0x4B00, 
                                0x07
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb3_prim_phy_aux_clk", 
                                0x08, 
                                0x4B00, 
                                0x07
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb3_prim_phy_com_aux_clk", 
                                0x08, 
                                0x4B00, 
                                0x07
                            }
                        }, 

                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_cx", 
                                0x0100
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_USB3_0", 
                                "ICBID_SLAVE_EBI1", 
                                0x28000000, 
                                0x28000000
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_USB3_0", 
                                0x0BEBC200, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x13)
                    {
                        "DSTATE", 
                        One, 
                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_pipe_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_usb3_prim_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_cfg_noc_usb3_prim_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb30_prim_master_clk", 
                                0x03, 
                                0x2580, 
                                0x05
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_master_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_mock_utmi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_aux_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_com_aux_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_USB3_0", 
                                "ICBID_SLAVE_EBI1", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_USB3_0", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_gdsc", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_cx", 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_C", 
                                One, 
                                0x001B7740, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_B", 
                                One, 
                                0x000DEA80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO2_B", 
                                One, 
                                0x002EE000, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO6_B", 
                                One, 
                                0x00124F80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                0x000D6D80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x13)
                    {
                        "DSTATE", 
                        0x02, 
                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_pipe_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb30_prim_master_clk", 
                                0x03, 
                                0x00927C00, 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_master_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_usb3_prim_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_cfg_noc_usb3_prim_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_mock_utmi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_aux_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_com_aux_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_USB3_0", 
                                "ICBID_SLAVE_EBI1", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_USB3_0", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_gdsc", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_cx", 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_C", 
                                One, 
                                0x001B7740, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_B", 
                                One, 
                                0x000DEA80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO2_B", 
                                One, 
                                0x002EE000, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO6_B", 
                                One, 
                                0x00124F80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                0x000D6D80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x14)
                    {
                        "DSTATE", 
                        0x03, 
                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_sleep_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_pipe_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_usb3_prim_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_cfg_noc_usb3_prim_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb30_prim_master_clk", 
                                0x03, 
                                0x00927C00, 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_master_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_mock_utmi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_aux_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_com_aux_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_USB3_0", 
                                "ICBID_SLAVE_EBI1", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_USB3_0", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_gdsc", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_cx", 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO2_B", 
                                One, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_B", 
                                One, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_C", 
                                One, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO6_B", 
                                One, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "ABANDON_DSTATE", 
                        0x03
                    }
                }, 

                Package (0x08)
                {
                    "DEVICE", 
                    "\\_SB.URS0.UFN0", 
                    Package (0x04)
                    {
                        "COMPONENT", 
                        Zero, 
                        Package (0x02)
                        {
                            "FSTATE", 
                            Zero
                        }, 

                        Package (0x02)
                        {
                            "PSTATE", 
                            Zero
                        }
                    }, 

                    Package (0x13)
                    {
                        "DSTATE", 
                        Zero, 
                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_C", 
                                One, 
                                0x001B7740, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_B", 
                                One, 
                                0x000DEA80, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO2_B", 
                                One, 
                                0x002EE000, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO6_B", 
                                One, 
                                0x00124F80, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                0x000D6D80, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_gdsc", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_sleep_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_pipe_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_aggre_usb3_prim_axi_clk", 
                                0x08, 
                                0xC8, 
                                0x09
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_cfg_noc_usb3_prim_axi_clk", 
                                0x08, 
                                0xC8, 
                                0x09
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb30_prim_master_clk", 
                                0x08, 
                                0xC8, 
                                0x09
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb30_prim_mock_utmi_clk", 
                                0x08, 
                                0x4B00, 
                                0x07
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb3_prim_phy_aux_clk", 
                                0x08, 
                                0x4B00, 
                                0x07
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb3_prim_phy_com_aux_clk", 
                                0x08, 
                                0x4B00, 
                                0x07
                            }
                        }, 

                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_cx", 
                                0x0100
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_USB3_0", 
                                "ICBID_SLAVE_EBI1", 
                                0x28000000, 
                                0x28000000
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_USB3_0", 
                                0x0BEBC200, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        One
                    }, 

                    Package (0x13)
                    {
                        "DSTATE", 
                        0x02, 
                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_pipe_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb30_prim_master_clk", 
                                0x03, 
                                0x00927C00, 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_master_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_usb3_prim_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_cfg_noc_usb3_prim_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_mock_utmi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_aux_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_com_aux_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_USB3_0", 
                                "ICBID_SLAVE_EBI1", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_USB3_0", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_gdsc", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_cx", 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_C", 
                                One, 
                                0x001B7740, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_B", 
                                One, 
                                0x000DEA80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO2_B", 
                                One, 
                                0x002EE000, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO6_B", 
                                One, 
                                0x00124F80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                0x000D6D80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x14)
                    {
                        "DSTATE", 
                        0x03, 
                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_sleep_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_pipe_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_usb3_prim_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_cfg_noc_usb3_prim_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb30_prim_master_clk", 
                                0x03, 
                                0x00927C00, 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_master_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_mock_utmi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_aux_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb3_prim_phy_com_aux_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_USB3_0", 
                                "ICBID_SLAVE_EBI1", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_USB3_0", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_usb30_prim_gdsc", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_cx", 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO2_B", 
                                One, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_B", 
                                One, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_C", 
                                One, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO6_B", 
                                One, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "ABANDON_DSTATE", 
                        0x03
                    }
                }, 

                Package (0x08)
                {
                    "DEVICE", 
                    "\\_SB.USB1", 
                    Package (0x04)
                    {
                        "COMPONENT", 
                        Zero, 
                        Package (0x02)
                        {
                            "FSTATE", 
                            Zero
                        }, 

                        Package (0x02)
                        {
                            "PSTATE", 
                            Zero
                        }
                    }, 

                    Package (0x0F)
                    {
                        "DSTATE", 
                        Zero, 
                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_C", 
                                One, 
                                0x001B7740, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                0x000D6D80, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO2_B", 
                                One, 
                                0x002EE000, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_usb30_sec_gdsc", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_sec_sleep_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_aggre_usb3_sec_axi_clk", 
                                0x08, 
                                0x78, 
                                0x09
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_cfg_noc_usb3_sec_axi_clk", 
                                0x08, 
                                0x78, 
                                0x09
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb30_sec_master_clk", 
                                0x08, 
                                0x78, 
                                0x09
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb30_sec_mock_utmi_clk", 
                                0x08, 
                                0x4B00, 
                                0x07
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_USB2", 
                                "ICBID_SLAVE_EBI1", 
                                0x03938700, 
                                0x03938700
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_USB2", 
                                0x0BEBC200, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_cx", 
                                0x0100
                            }
                        }, 

                        Package (0x02)
                        {
                            "TLMMGPIO", 
                            Package (0x06)
                            {
                                0x3F, 
                                One, 
                                Zero, 
                                One, 
                                Zero, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x0F)
                    {
                        "DSTATE", 
                        One, 
                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_usb30_sec_gdsc", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb30_sec_master_clk", 
                                0x03, 
                                0x2580, 
                                0x05
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_sec_master_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_cfg_noc_usb3_sec_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_usb3_sec_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_sec_mock_utmi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_USB2", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_USB2", 
                                "ICBID_SLAVE_EBI1", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_cx", 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_C", 
                                One, 
                                0x001B7740, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                0x000D6D80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO2_B", 
                                One, 
                                0x002EE000, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "TLMMGPIO", 
                            Package (0x06)
                            {
                                0x3F, 
                                One, 
                                Zero, 
                                One, 
                                Zero, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x0F)
                    {
                        "DSTATE", 
                        0x02, 
                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_usb3_sec_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_cfg_noc_usb3_sec_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb30_sec_master_clk", 
                                0x03, 
                                0x00927C00, 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_sec_master_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_sec_mock_utmi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_USB2", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_USB2", 
                                "ICBID_SLAVE_EBI1", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_usb30_sec_gdsc", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_cx", 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_C", 
                                One, 
                                0x001B7740, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                0x000D6D80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO2_B", 
                                One, 
                                0x002EE000, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "TLMMGPIO", 
                            Package (0x06)
                            {
                                0x3F, 
                                One, 
                                Zero, 
                                One, 
                                Zero, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x10)
                    {
                        "DSTATE", 
                        0x03, 
                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_sec_sleep_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_usb3_sec_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_cfg_noc_usb3_sec_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_usb30_sec_master_clk", 
                                0x03, 
                                0x00927C00, 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_sec_master_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_usb30_sec_mock_utmi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_USB2", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_USB2", 
                                "ICBID_SLAVE_EBI1", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_usb30_sec_gdsc", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_cx", 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO2_B", 
                                One, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO1_C", 
                                One, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "TLMMGPIO", 
                            Package (0x06)
                            {
                                0x3F, 
                                Zero, 
                                Zero, 
                                One, 
                                Zero, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "ABANDON_DSTATE", 
                        0x03
                    }
                }
            })
        }

        Scope (\_SB.PEP0)
        {
            Method (BPMD, 0, NotSerialized)
            {
                If ((STOR == One))
                {
                    If ((PUS3 == One))
                    {
                        Return (CPCC) /* \_SB_.PEP0.CPCC */
                    }
                    Else
                    {
                        Return (BPCC) /* \_SB_.PEP0.BPCC */
                    }
                }
                ElseIf ((STOR == 0x02))
                {
                    Return (DPCC) /* \_SB_.PEP0.DPCC */
                }
                Else
                {
                    Return (FPCC) /* \_SB_.PEP0.FPCC */
                }
            }

            Method (SDMD, 0, NotSerialized)
            {
                Return (SDCC) /* \_SB_.PEP0.SDCC */
            }

            Name (BPCC, Package (0x01)
            {
                Package (0x07)
                {
                    "DEVICE", 
                    "\\_SB.UFS0", 
                    Package (0x07)
                    {
                        "COMPONENT", 
                        Zero, 
                        Package (0x05)
                        {
                            "FSTATE", 
                            Zero, 
                            Package (0x02)
                            {
                                "PSTATE_ADJUST", 
                                Package (0x02)
                                {
                                    Zero, 
                                    Zero
                                }
                            }, 

                            Package (0x02)
                            {
                                "PSTATE_ADJUST", 
                                Package (0x02)
                                {
                                    One, 
                                    Zero
                                }
                            }, 

                            Package (0x02)
                            {
                                "PSTATE_ADJUST", 
                                Package (0x02)
                                {
                                    0x02, 
                                    Zero
                                }
                            }
                        }, 

                        Package (0x05)
                        {
                            "FSTATE", 
                            One, 
                            Package (0x02)
                            {
                                "PSTATE_ADJUST", 
                                Package (0x02)
                                {
                                    0x02, 
                                    One
                                }
                            }, 

                            Package (0x02)
                            {
                                "PSTATE_ADJUST", 
                                Package (0x02)
                                {
                                    One, 
                                    One
                                }
                            }, 

                            Package (0x02)
                            {
                                "PSTATE_ADJUST", 
                                Package (0x02)
                                {
                                    Zero, 
                                    One
                                }
                            }
                        }, 

                        Package (0x04)
                        {
                            "PSTATE_SET", 
                            Zero, 
                            Package (0x03)
                            {
                                "PSTATE", 
                                Zero, 
                                Package (0x02)
                                {
                                    "FOOTSWITCH", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_gdsc", 
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                One, 
                                Package (0x02)
                                {
                                    "FOOTSWITCH", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_gdsc", 
                                        0x02
                                    }
                                }
                            }
                        }, 

                        Package (0x04)
                        {
                            "PSTATE_SET", 
                            One, 
                            Package (0x0D)
                            {
                                "PSTATE", 
                                Zero, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x04)
                                    {
                                        "gcc_ufs_phy_axi_clk", 
                                        0x08, 
                                        0x11E1A300, 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x04)
                                    {
                                        "gcc_ufs_phy_unipro_core_clk", 
                                        0x08, 
                                        0x11E1A300, 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_ice_core_clk", 
                                        One
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x03)
                                    {
                                        "gcc_ufs_phy_ice_core_clk", 
                                        0x09, 
                                        0x12
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x04)
                                    {
                                        "gcc_ufs_phy_ice_core_clk", 
                                        0x03, 
                                        0x11E1A300, 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_aggre_ufs_phy_axi_clk", 
                                        One
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_ahb_clk", 
                                        One
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_phy_aux_clk", 
                                        One
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_tx_symbol_0_clk", 
                                        One
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_rx_symbol_0_clk", 
                                        One
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_rx_symbol_1_clk", 
                                        One
                                    }
                                }
                            }, 

                            Package (0x0B)
                            {
                                "PSTATE", 
                                One, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_aggre_ufs_phy_axi_clk", 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_ahb_clk", 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_phy_aux_clk", 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_tx_symbol_0_clk", 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_rx_symbol_0_clk", 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_rx_symbol_1_clk", 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_ice_core_clk", 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_unipro_core_clk", 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_axi_clk", 
                                        0x02
                                    }
                                }
                            }
                        }, 

                        Package (0x04)
                        {
                            "PSTATE_SET", 
                            0x02, 
                            Package (0x04)
                            {
                                "PSTATE", 
                                Zero, 
                                Package (0x02)
                                {
                                    "BUSARB", 
                                    Package (0x05)
                                    {
                                        0x03, 
                                        "ICBID_MASTER_UFS_MEM", 
                                        "ICBID_SLAVE_EBI1", 
                                        0x47868C00, 
                                        0x47868C00
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "BUSARB", 
                                    Package (0x05)
                                    {
                                        0x03, 
                                        "ICBID_MASTER_APPSS_PROC", 
                                        "ICBID_SLAVE_UFS_MEM_CFG", 
                                        0x11D260C0, 
                                        Zero
                                    }
                                }
                            }, 

                            Package (0x04)
                            {
                                "PSTATE", 
                                One, 
                                Package (0x02)
                                {
                                    "BUSARB", 
                                    Package (0x05)
                                    {
                                        0x03, 
                                        "ICBID_MASTER_APPSS_PROC", 
                                        "ICBID_SLAVE_UFS_MEM_CFG", 
                                        Zero, 
                                        Zero
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "BUSARB", 
                                    Package (0x05)
                                    {
                                        0x03, 
                                        "ICBID_MASTER_UFS_MEM", 
                                        "ICBID_SLAVE_EBI1", 
                                        Zero, 
                                        Zero
                                    }
                                }
                            }
                        }
                    }, 

                    Package (0x0A)
                    {
                        "DSTATE", 
                        Zero, 
                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                0x02, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO7_B", 
                                One, 
                                0x002C4FC0, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO9_B", 
                                One, 
                                0x00124F80, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO6_B", 
                                One, 
                                0x00124F80, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                0x000D6D80, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "DELAY", 
                            Package (0x01)
                            {
                                0x23
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                One, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x09)
                    {
                        "DSTATE", 
                        0x03, 
                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                One, 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                0x000D6D80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO6_B", 
                                One, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO9_B", 
                                One, 
                                0x00124F80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO7_B", 
                                One, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                Zero, 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                0x02, 
                                One
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "CRASHDUMP_EXCEPTION", 
                        Package (0x02)
                        {
                            "EXECUTE_FUNCTION", 
                            Package (0x01)
                            {
                                "ExecuteOcdEMMCExceptions"
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "CRASHDUMP_DSTATE", 
                        Zero
                    }
                }
            })
            Name (CPCC, Package (0x01)
            {
                Package (0x06)
                {
                    "DEVICE", 
                    "\\_SB.UFS0", 
                    Package (0x07)
                    {
                        "COMPONENT", 
                        Zero, 
                        Package (0x05)
                        {
                            "FSTATE", 
                            Zero, 
                            Package (0x02)
                            {
                                "PSTATE_ADJUST", 
                                Package (0x02)
                                {
                                    Zero, 
                                    Zero
                                }
                            }, 

                            Package (0x02)
                            {
                                "PSTATE_ADJUST", 
                                Package (0x02)
                                {
                                    One, 
                                    Zero
                                }
                            }, 

                            Package (0x02)
                            {
                                "PSTATE_ADJUST", 
                                Package (0x02)
                                {
                                    0x02, 
                                    Zero
                                }
                            }
                        }, 

                        Package (0x05)
                        {
                            "FSTATE", 
                            One, 
                            Package (0x02)
                            {
                                "PSTATE_ADJUST", 
                                Package (0x02)
                                {
                                    0x02, 
                                    One
                                }
                            }, 

                            Package (0x02)
                            {
                                "PSTATE_ADJUST", 
                                Package (0x02)
                                {
                                    One, 
                                    One
                                }
                            }, 

                            Package (0x02)
                            {
                                "PSTATE_ADJUST", 
                                Package (0x02)
                                {
                                    Zero, 
                                    One
                                }
                            }
                        }, 

                        Package (0x04)
                        {
                            "PSTATE_SET", 
                            Zero, 
                            Package (0x03)
                            {
                                "PSTATE", 
                                Zero, 
                                Package (0x02)
                                {
                                    "FOOTSWITCH", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_gdsc", 
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                One, 
                                Package (0x02)
                                {
                                    "FOOTSWITCH", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_gdsc", 
                                        0x02
                                    }
                                }
                            }
                        }, 

                        Package (0x04)
                        {
                            "PSTATE_SET", 
                            One, 
                            Package (0x0D)
                            {
                                "PSTATE", 
                                Zero, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x04)
                                    {
                                        "gcc_ufs_phy_axi_clk", 
                                        0x08, 
                                        0x11E1A300, 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x04)
                                    {
                                        "gcc_ufs_phy_unipro_core_clk", 
                                        0x08, 
                                        0x11E1A300, 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_ice_core_clk", 
                                        One
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x03)
                                    {
                                        "gcc_ufs_phy_ice_core_clk", 
                                        0x09, 
                                        0x12
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x04)
                                    {
                                        "gcc_ufs_phy_ice_core_clk", 
                                        0x03, 
                                        0x11E1A300, 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_aggre_ufs_phy_axi_clk", 
                                        One
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_ahb_clk", 
                                        One
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_phy_aux_clk", 
                                        One
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_tx_symbol_0_clk", 
                                        One
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_rx_symbol_0_clk", 
                                        One
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_rx_symbol_1_clk", 
                                        One
                                    }
                                }
                            }, 

                            Package (0x0B)
                            {
                                "PSTATE", 
                                One, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_aggre_ufs_phy_axi_clk", 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_ahb_clk", 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_phy_aux_clk", 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_tx_symbol_0_clk", 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_rx_symbol_0_clk", 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_rx_symbol_1_clk", 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_ice_core_clk", 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_unipro_core_clk", 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_ufs_phy_axi_clk", 
                                        0x02
                                    }
                                }
                            }
                        }, 

                        Package (0x04)
                        {
                            "PSTATE_SET", 
                            0x02, 
                            Package (0x04)
                            {
                                "PSTATE", 
                                Zero, 
                                Package (0x02)
                                {
                                    "BUSARB", 
                                    Package (0x05)
                                    {
                                        0x03, 
                                        "ICBID_MASTER_UFS_MEM", 
                                        "ICBID_SLAVE_EBI1", 
                                        0x8F0D1800, 
                                        0x8F0D1800
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "BUSARB", 
                                    Package (0x05)
                                    {
                                        0x03, 
                                        "ICBID_MASTER_APPSS_PROC", 
                                        "ICBID_SLAVE_UFS_MEM_CFG", 
                                        0x11D260C0, 
                                        Zero
                                    }
                                }
                            }, 

                            Package (0x04)
                            {
                                "PSTATE", 
                                One, 
                                Package (0x02)
                                {
                                    "BUSARB", 
                                    Package (0x05)
                                    {
                                        0x03, 
                                        "ICBID_MASTER_APPSS_PROC", 
                                        "ICBID_SLAVE_UFS_MEM_CFG", 
                                        Zero, 
                                        Zero
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "BUSARB", 
                                    Package (0x05)
                                    {
                                        0x03, 
                                        "ICBID_MASTER_UFS_MEM", 
                                        "ICBID_SLAVE_EBI1", 
                                        Zero, 
                                        Zero
                                    }
                                }
                            }
                        }
                    }, 

                    Package (0x0A)
                    {
                        "DSTATE", 
                        Zero, 
                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                0x02, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO7_B", 
                                One, 
                                0x00263540, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO9_B", 
                                One, 
                                0x00124F80, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO6_B", 
                                One, 
                                0x00124F80, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                0x000D6D80, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "DELAY", 
                            Package (0x01)
                            {
                                0x23
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                One, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x09)
                    {
                        "DSTATE", 
                        0x03, 
                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                One, 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                0x000D6D80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO6_B", 
                                One, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO9_B", 
                                One, 
                                0x00124F80, 
                                One, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO7_B", 
                                One, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                Zero, 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                0x02, 
                                One
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "CRASHDUMP_DSTATE", 
                        Zero
                    }
                }
            })
            Name (FPCC, Package (0x01)
            {
                Package (0x06)
                {
                    "DEVICE", 
                    "\\_SB.UFS0", 
                    Package (0x04)
                    {
                        "COMPONENT", 
                        Zero, 
                        Package (0x02)
                        {
                            "FSTATE", 
                            Zero
                        }, 

                        Package (0x02)
                        {
                            "FSTATE", 
                            One
                        }
                    }, 

                    Package (0x02)
                    {
                        "PRELOAD_DSTATE", 
                        0x03
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        Zero
                    }, 

                    Package (0x04)
                    {
                        "DSTATE", 
                        0x03, 
                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO9_B", 
                                One, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO7_B", 
                                One, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }
                    }
                }
            })
            Name (DPCC, Package (0x01)
            {
                Package (0x07)
                {
                    "DEVICE", 
                    "\\_SB.SDC1", 
                    Package (0x0A)
                    {
                        "COMPONENT", 
                        Zero, 
                        Package (0x02)
                        {
                            "FSTATE", 
                            Zero
                        }, 

                        Package (0x02)
                        {
                            "FSTATE", 
                            One
                        }, 

                        Package (0x1A)
                        {
                            "PSTATE_SET", 
                            Zero, 
                            Package (0x03)
                            {
                                "PSTATE", 
                                Zero, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                One, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x02, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x03, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x04, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x05, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x06, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x07, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x08, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x09, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x0A, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x0B, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x0C, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x0D, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x0E, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x0F, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x10, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x11, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x12, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x13, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x14, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x15, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x05)
                            {
                                "PSTATE", 
                                0x16, 
                                Package (0x02)
                                {
                                    "PMICVREGVOTE", 
                                    Package (0x06)
                                    {
                                        "PPP_RESOURCE_ID_LDO7_B", 
                                        One, 
                                        0x002D2A80, 
                                        One, 
                                        0x07, 
                                        Zero
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PMICVREGVOTE", 
                                    Package (0x06)
                                    {
                                        "PPP_RESOURCE_ID_LDO19_B", 
                                        One, 
                                        0x001B7740, 
                                        One, 
                                        0x07, 
                                        Zero
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        0x23
                                    }
                                }
                            }, 

                            Package (0x04)
                            {
                                "PSTATE", 
                                0x17, 
                                Package (0x02)
                                {
                                    "PMICVREGVOTE", 
                                    Package (0x06)
                                    {
                                        "PPP_RESOURCE_ID_LDO7_B", 
                                        One, 
                                        Zero, 
                                        Zero, 
                                        Zero, 
                                        Zero
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        0x23
                                    }
                                }
                            }
                        }, 

                        Package (0x0A)
                        {
                            "PSTATE_SET", 
                            One, 
                            Package (0x03)
                            {
                                "PSTATE", 
                                Zero, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_sdcc1_apps_clk", 
                                        0x02
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                One, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x04)
                                    {
                                        "gcc_sdcc1_apps_clk", 
                                        0x08, 
                                        0x00061A80, 
                                        0x02
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x02, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x04)
                                    {
                                        "gcc_sdcc1_apps_clk", 
                                        0x08, 
                                        0x01312D00, 
                                        0x02
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x03, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x04)
                                    {
                                        "gcc_sdcc1_apps_clk", 
                                        0x08, 
                                        0x017D7840, 
                                        0x02
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x04, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x04)
                                    {
                                        "gcc_sdcc1_apps_clk", 
                                        0x08, 
                                        0x02FAF080, 
                                        0x02
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x05, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x04)
                                    {
                                        "gcc_sdcc1_apps_clk", 
                                        0x08, 
                                        0x05F5E100, 
                                        0x02
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x06, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x04)
                                    {
                                        "gcc_sdcc1_apps_clk", 
                                        0x08, 
                                        0x0B71B000, 
                                        0x02
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x07, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x04)
                                    {
                                        "gcc_sdcc1_apps_clk", 
                                        0x08, 
                                        0x16E36000, 
                                        0x02
                                    }
                                }
                            }
                        }, 

                        Package (0x07)
                        {
                            "PSTATE_SET", 
                            0x02, 
                            Package (0x03)
                            {
                                "PSTATE", 
                                Zero, 
                                Package (0x02)
                                {
                                    "BUSARB", 
                                    Package (0x05)
                                    {
                                        0x03, 
                                        "ICBID_MASTER_SDCC_1", 
                                        "ICBID_SLAVE_EBI1", 
                                        0x2FAF0800, 
                                        0x17D78400
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                One, 
                                Package (0x02)
                                {
                                    "BUSARB", 
                                    Package (0x05)
                                    {
                                        0x03, 
                                        "ICBID_MASTER_SDCC_1", 
                                        "ICBID_SLAVE_EBI1", 
                                        0x17D78400, 
                                        0x0BEBC200
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x02, 
                                Package (0x02)
                                {
                                    "BUSARB", 
                                    Package (0x05)
                                    {
                                        0x03, 
                                        "ICBID_MASTER_SDCC_1", 
                                        "ICBID_SLAVE_EBI1", 
                                        0x0BEBC200, 
                                        0x05F5E100
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x03, 
                                Package (0x02)
                                {
                                    "BUSARB", 
                                    Package (0x05)
                                    {
                                        0x03, 
                                        "ICBID_MASTER_SDCC_1", 
                                        "ICBID_SLAVE_EBI1", 
                                        0x02625A00, 
                                        0x01312D00
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x04, 
                                Package (0x02)
                                {
                                    "BUSARB", 
                                    Package (0x05)
                                    {
                                        0x03, 
                                        "ICBID_MASTER_SDCC_1", 
                                        "ICBID_SLAVE_EBI1", 
                                        Zero, 
                                        Zero
                                    }
                                }
                            }
                        }, 

                        Package (0x04)
                        {
                            "PSTATE_SET", 
                            0x03, 
                            Package (0x05)
                            {
                                "PSTATE", 
                                Zero, 
                                Package (0x02)
                                {
                                    "PSTATE_ADJUST", 
                                    Package (0x02)
                                    {
                                        One, 
                                        0x07
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PSTATE_ADJUST", 
                                    Package (0x02)
                                    {
                                        0x02, 
                                        Zero
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PSTATE_ADJUST", 
                                    Package (0x02)
                                    {
                                        0x05, 
                                        0x03
                                    }
                                }
                            }, 

                            Package (0x05)
                            {
                                "PSTATE", 
                                One, 
                                Package (0x02)
                                {
                                    "PSTATE_ADJUST", 
                                    Package (0x02)
                                    {
                                        One, 
                                        0x05
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PSTATE_ADJUST", 
                                    Package (0x02)
                                    {
                                        0x02, 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PSTATE_ADJUST", 
                                    Package (0x02)
                                    {
                                        0x05, 
                                        One
                                    }
                                }
                            }
                        }, 

                        Package (0x04)
                        {
                            "PSTATE_SET", 
                            0x04, 
                            Package (0x03)
                            {
                                "PSTATE", 
                                Zero, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_sdcc1_ahb_clk", 
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                One, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_sdcc1_ahb_clk", 
                                        0x02
                                    }
                                }
                            }
                        }, 

                        Package (0x06)
                        {
                            "PSTATE_SET", 
                            0x05, 
                            Package (0x03)
                            {
                                "PSTATE", 
                                Zero, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_sdcc1_ice_core_clk", 
                                        0x02
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                One, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x04)
                                    {
                                        "gcc_sdcc1_ice_core_clk", 
                                        0x08, 
                                        0x05F5E100, 
                                        0x02
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x02, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x04)
                                    {
                                        "gcc_sdcc1_ice_core_clk", 
                                        0x08, 
                                        0x08F0D180, 
                                        0x02
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x03, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x04)
                                    {
                                        "gcc_sdcc1_ice_core_clk", 
                                        0x08, 
                                        0x11E1A300, 
                                        0x02
                                    }
                                }
                            }
                        }
                    }, 

                    Package (0x0A)
                    {
                        "DSTATE", 
                        Zero, 
                        Package (0x02)
                        {
                            "TLMMPORT", 
                            Package (0x03)
                            {
                                0x001B3000, 
                                0x0001FFFF, 
                                0x1FE4
                            }
                        }, 

                        Package (0x02)
                        {
                            "TLMMPORT", 
                            Package (0x03)
                            {
                                0x001B3004, 
                                0x0001FFFF, 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                One, 
                                0x07
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                0x02, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                0x05, 
                                0x03
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x01)
                        {
                            "PSTATE_RESTORE"
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                Zero, 
                                0x16
                            }
                        }
                    }, 

                    Package (0x09)
                    {
                        "DSTATE", 
                        0x03, 
                        Package (0x01)
                        {
                            "PSTATE_SAVE"
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                0x05, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                0x04, 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                One, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                0x02, 
                                0x04
                            }
                        }, 

                        Package (0x02)
                        {
                            "TLMMPORT", 
                            Package (0x03)
                            {
                                0x001B3000, 
                                0x0001FFFF, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "TLMMPORT", 
                            Package (0x03)
                            {
                                0x001B3004, 
                                0x0001FFFF, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "CRASHDUMP_EXCEPTION", 
                        Package (0x02)
                        {
                            "EXECUTE_FUNCTION", 
                            Package (0x01)
                            {
                                "ExecuteOcdEMMCExceptions"
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "CRASHDUMP_DSTATE", 
                        Zero
                    }
                }
            })
            Name (SDCC, Package (0x01)
            {
                Package (0x07)
                {
                    "DEVICE", 
                    "\\_SB.SDC2", 
                    Package (0x09)
                    {
                        "COMPONENT", 
                        Zero, 
                        Package (0x02)
                        {
                            "FSTATE", 
                            Zero
                        }, 

                        Package (0x02)
                        {
                            "FSTATE", 
                            One
                        }, 

                        Package (0x19)
                        {
                            "PSTATE_SET", 
                            Zero, 
                            Package (0x03)
                            {
                                "PSTATE", 
                                Zero, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                One, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x02, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x03, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x04, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x05, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x06, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x07, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x08, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x09, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x0B, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x0C, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x0D, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x0E, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x0F, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x10, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x11, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x12, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x13, 
                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        One
                                    }
                                }
                            }, 

                            Package (0x08)
                            {
                                "PSTATE", 
                                0x14, 
                                Package (0x02)
                                {
                                    "PMICVREGVOTE", 
                                    Package (0x06)
                                    {
                                        "PPP_RESOURCE_ID_LDO9_C", 
                                        One, 
                                        Zero, 
                                        Zero, 
                                        Zero, 
                                        Zero
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PMICVREGVOTE", 
                                    Package (0x06)
                                    {
                                        "PPP_RESOURCE_ID_LDO6_C", 
                                        One, 
                                        Zero, 
                                        Zero, 
                                        Zero, 
                                        Zero
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        0x23
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PMICVREGVOTE", 
                                    Package (0x06)
                                    {
                                        "PPP_RESOURCE_ID_LDO9_C", 
                                        One, 
                                        0x002D2A80, 
                                        One, 
                                        0x07, 
                                        Zero
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PMICVREGVOTE", 
                                    Package (0x06)
                                    {
                                        "PPP_RESOURCE_ID_LDO6_C", 
                                        One, 
                                        0x002D0370, 
                                        One, 
                                        0x07, 
                                        Zero
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        0x23
                                    }
                                }
                            }, 

                            Package (0x04)
                            {
                                "PSTATE", 
                                0x15, 
                                Package (0x02)
                                {
                                    "PMICVREGVOTE", 
                                    Package (0x06)
                                    {
                                        "PPP_RESOURCE_ID_LDO6_C", 
                                        One, 
                                        0x001B7740, 
                                        One, 
                                        0x07, 
                                        Zero
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        0x23
                                    }
                                }
                            }, 

                            Package (0x05)
                            {
                                "PSTATE", 
                                0x16, 
                                Package (0x02)
                                {
                                    "PMICVREGVOTE", 
                                    Package (0x06)
                                    {
                                        "PPP_RESOURCE_ID_LDO9_C", 
                                        One, 
                                        0x002D2A80, 
                                        One, 
                                        0x07, 
                                        Zero
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PMICVREGVOTE", 
                                    Package (0x06)
                                    {
                                        "PPP_RESOURCE_ID_LDO6_C", 
                                        One, 
                                        0x002D0370, 
                                        One, 
                                        0x07, 
                                        Zero
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        0x23
                                    }
                                }
                            }, 

                            Package (0x05)
                            {
                                "PSTATE", 
                                0x17, 
                                Package (0x02)
                                {
                                    "PMICVREGVOTE", 
                                    Package (0x06)
                                    {
                                        "PPP_RESOURCE_ID_LDO9_C", 
                                        One, 
                                        Zero, 
                                        Zero, 
                                        Zero, 
                                        Zero
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PMICVREGVOTE", 
                                    Package (0x06)
                                    {
                                        "PPP_RESOURCE_ID_LDO6_C", 
                                        One, 
                                        Zero, 
                                        Zero, 
                                        Zero, 
                                        Zero
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "DELAY", 
                                    Package (0x01)
                                    {
                                        0x23
                                    }
                                }
                            }
                        }, 

                        Package (0x05)
                        {
                            "PSTATE_SET", 
                            One, 
                            Package (0x03)
                            {
                                "PSTATE", 
                                Zero, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_sdcc2_apps_clk", 
                                        0x02
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                One, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x04)
                                    {
                                        "gcc_sdcc2_apps_clk", 
                                        0x08, 
                                        0x05F5E100, 
                                        0x02
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x02, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x04)
                                    {
                                        "gcc_sdcc2_apps_clk", 
                                        0x08, 
                                        0x0C0A4680, 
                                        0x02
                                    }
                                }
                            }
                        }, 

                        Package (0x05)
                        {
                            "PSTATE_SET", 
                            0x02, 
                            Package (0x03)
                            {
                                "PSTATE", 
                                Zero, 
                                Package (0x02)
                                {
                                    "BUSARB", 
                                    Package (0x05)
                                    {
                                        0x03, 
                                        "ICBID_MASTER_SDCC_2", 
                                        "ICBID_SLAVE_EBI1", 
                                        0x17D78400, 
                                        0x0BEBC200
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                One, 
                                Package (0x02)
                                {
                                    "BUSARB", 
                                    Package (0x05)
                                    {
                                        0x03, 
                                        "ICBID_MASTER_SDCC_2", 
                                        "ICBID_SLAVE_EBI1", 
                                        0x0BEBC200, 
                                        0x05F5E100
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                0x02, 
                                Package (0x02)
                                {
                                    "BUSARB", 
                                    Package (0x05)
                                    {
                                        0x03, 
                                        "ICBID_MASTER_SDCC_2", 
                                        "ICBID_SLAVE_EBI1", 
                                        Zero, 
                                        Zero
                                    }
                                }
                            }
                        }, 

                        Package (0x04)
                        {
                            "PSTATE_SET", 
                            0x03, 
                            Package (0x04)
                            {
                                "PSTATE", 
                                Zero, 
                                Package (0x02)
                                {
                                    "PSTATE_ADJUST", 
                                    Package (0x02)
                                    {
                                        One, 
                                        0x02
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PSTATE_ADJUST", 
                                    Package (0x02)
                                    {
                                        0x02, 
                                        Zero
                                    }
                                }
                            }, 

                            Package (0x04)
                            {
                                "PSTATE", 
                                One, 
                                Package (0x02)
                                {
                                    "PSTATE_ADJUST", 
                                    Package (0x02)
                                    {
                                        One, 
                                        One
                                    }
                                }, 

                                Package (0x02)
                                {
                                    "PSTATE_ADJUST", 
                                    Package (0x02)
                                    {
                                        0x02, 
                                        One
                                    }
                                }
                            }
                        }, 

                        Package (0x04)
                        {
                            "PSTATE_SET", 
                            0x04, 
                            Package (0x03)
                            {
                                "PSTATE", 
                                Zero, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_sdcc2_ahb_clk", 
                                        One
                                    }
                                }
                            }, 

                            Package (0x03)
                            {
                                "PSTATE", 
                                One, 
                                Package (0x02)
                                {
                                    "CLOCK", 
                                    Package (0x02)
                                    {
                                        "gcc_sdcc2_ahb_clk", 
                                        0x02
                                    }
                                }
                            }
                        }
                    }, 

                    Package (0x07)
                    {
                        "DSTATE", 
                        Zero, 
                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                Zero, 
                                0x16
                            }
                        }, 

                        Package (0x02)
                        {
                            "TLMMPORT", 
                            Package (0x03)
                            {
                                0x001B4000, 
                                0x7FFF, 
                                0x1FE4
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                0x02, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                One, 
                                0x02
                            }
                        }
                    }, 

                    Package (0x07)
                    {
                        "DSTATE", 
                        0x03, 
                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                One, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                0x04, 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                0x02, 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "TLMMPORT", 
                            Package (0x03)
                            {
                                0x001B4000, 
                                0x7FFF, 
                                0x0A00
                            }
                        }, 

                        Package (0x02)
                        {
                            "PSTATE_ADJUST", 
                            Package (0x02)
                            {
                                Zero, 
                                0x17
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "CRASHDUMP_EXCEPTION", 
                        Package (0x02)
                        {
                            "EXECUTE_FUNCTION", 
                            Package (0x01)
                            {
                                "ExecuteOcdSdCardExceptions"
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "CRASHDUMP_DSTATE", 
                        Zero
                    }
                }
            })
        }

        Scope (\_SB.PEP0)
        {
            Method (EWMD, 0, NotSerialized)
            {
                Return (WBRC) /* \_SB_.PEP0.WBRC */
            }

            Name (WBRC, Package (0x01)
            {
                Package (0x07)
                {
                    "DEVICE", 
                    "\\_SB.WPSS.QWLN", 
                    Package (0x03)
                    {
                        "COMPONENT", 
                        Zero, 
                        Package (0x02)
                        {
                            "FSTATE", 
                            Zero
                        }
                    }, 

                    Package (0x06)
                    {
                        "DSTATE", 
                        Zero, 
                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_xo", 
                                0x80
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS1_B", 
                                0x02, 
                                0x001C9080, 
                                One, 
                                0x06, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS7_B", 
                                0x02, 
                                0x000E86C0, 
                                One, 
                                0x06, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS8_B", 
                                0x02, 
                                0x00132A40, 
                                One, 
                                0x06, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x06)
                    {
                        "DSTATE", 
                        0x02, 
                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_xo", 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS1_B", 
                                0x02, 
                                Zero, 
                                Zero, 
                                0x05, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS7_B", 
                                0x02, 
                                Zero, 
                                Zero, 
                                0x05, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS8_B", 
                                0x02, 
                                Zero, 
                                Zero, 
                                0x05, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x06)
                    {
                        "DSTATE", 
                        0x03, 
                        Package (0x02)
                        {
                            "NPARESOURCE", 
                            Package (0x03)
                            {
                                One, 
                                "/arc/client/rail_xo", 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS1_B", 
                                0x02, 
                                Zero, 
                                Zero, 
                                0x05, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS7_B", 
                                0x02, 
                                Zero, 
                                Zero, 
                                0x05, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS8_B", 
                                0x02, 
                                Zero, 
                                Zero, 
                                0x05, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "ABANDON_DSTATE", 
                        0x02
                    }
                }
            })
        }

        Scope (\_SB.PEP0)
        {
            Method (PEMD, 0, NotSerialized)
            {
                If (((((\_SB.SKUV == 0x1F) || (\_SB.SKUV == 0x20)) || (\_SB.SKUV == 
                    0x21)) || (\_SB.SKUV == 0x22)))
                {
                    Return (PEMC) /* \_SB_.PEP0.PEMC */
                }
                Else
                {
                    Return (PEMX) /* \_SB_.PEP0.PEMX */
                }
            }

            Name (PEMC, Package (0x04)
            {
                Package (0x0A)
                {
                    "DEVICE", 
                    "\\_SB.PCI0", 
                    Package (0x04)
                    {
                        "COMPONENT", 
                        Zero, 
                        Package (0x02)
                        {
                            "FSTATE", 
                            Zero
                        }, 

                        Package (0x02)
                        {
                            "FSTATE", 
                            One
                        }
                    }, 

                    Package (0x1A)
                    {
                        "DSTATE", 
                        Zero, 
                        Package (0x02)
                        {
                            "TLMMGPIO", 
                            Package (0x06)
                            {
                                0x54, 
                                One, 
                                Zero, 
                                One, 
                                0x03, 
                                0x04
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS1_B", 
                                0x02, 
                                0x001C5200, 
                                One, 
                                0x06, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS7_B", 
                                0x02, 
                                0x000E86C0, 
                                One, 
                                0x06, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS8_B", 
                                0x02, 
                                0x00132A40, 
                                One, 
                                0x06, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO11_C", 
                                One, 
                                0x002AB980, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO19_B", 
                                One, 
                                0x001B7740, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_gdsc", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_PCIE_0_CFG", 
                                0x047868C0, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_PCIE_0", 
                                "ICBID_SLAVE_EBI1", 
                                0xB2D05E00, 
                                0xB2D05E00
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x09)
                            {
                                "gcc_pcie_0_pipe_clk", 
                                0x06, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_noc_pcie_center_sf_axi_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_aux_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_pipe_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_slv_axi_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_slv_q2a_axi_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_ddrss_pcie_sf_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_throttle_core_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_throttle_pcie_ahb_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_mstr_axi_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_noc_pcie_0_axi_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_cfg_ahb_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_pcie_0_aux_clk", 
                                0x08, 
                                0x0124F800, 
                                0x03
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_pcie0_phy_rchng_clk", 
                                0x08, 
                                0x05F5E100, 
                                0x03
                            }
                        }, 

                        Package (0x02)
                        {
                            "TLMMGPIO", 
                            Package (0x07)
                            {
                                0x58, 
                                Zero, 
                                One, 
                                Zero, 
                                0x03, 
                                Zero, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        One
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        0x02
                    }, 

                    Package (0x17)
                    {
                        "DSTATE", 
                        0x03, 
                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_aux_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_slv_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_slv_q2a_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_mstr_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_throttle_core_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_throttle_pcie_ahb_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_ddrss_pcie_sf_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_noc_pcie_0_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_cfg_ahb_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie0_phy_rchng_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_pipe_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_noc_pcie_center_sf_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x09)
                            {
                                "gcc_pcie_0_pipe_clk", 
                                0x06, 
                                Zero, 
                                Zero, 
                                0x02, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_PCIE_0_CFG", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_PCIE_0", 
                                "ICBID_SLAVE_EBI1", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_gdsc", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO11_C", 
                                One, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO19_B", 
                                One, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS1_B", 
                                0x02, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS7_B", 
                                0x02, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS8_B", 
                                0x02, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "CRASHDUMP_EXCEPTION", 
                        Package (0x02)
                        {
                            "EXECUTE_FUNCTION", 
                            Package (0x01)
                            {
                                "ExecuteOcdPCIeExceptions"
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "CRASHDUMP_DSTATE", 
                        Zero
                    }, 

                    Package (0x02)
                    {
                        "ABANDON_DSTATE", 
                        Zero
                    }
                }, 

                Package (0x08)
                {
                    "DEVICE", 
                    "\\_SB.PCI0.RP1", 
                    Package (0x04)
                    {
                        "COMPONENT", 
                        Zero, 
                        Package (0x02)
                        {
                            "FSTATE", 
                            Zero
                        }, 

                        Package (0x02)
                        {
                            "FSTATE", 
                            One
                        }
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        Zero
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        One
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        0x02
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        0x03
                    }, 

                    Package (0x02)
                    {
                        "ABANDON_DSTATE", 
                        Zero
                    }
                }, 

                Package (0x0A)
                {
                    "DEVICE", 
                    "\\_SB.PCI1", 
                    Package (0x04)
                    {
                        "COMPONENT", 
                        Zero, 
                        Package (0x02)
                        {
                            "FSTATE", 
                            Zero
                        }, 

                        Package (0x02)
                        {
                            "FSTATE", 
                            One
                        }
                    }, 

                    Package (0x18)
                    {
                        "DSTATE", 
                        Zero, 
                        Package (0x02)
                        {
                            "TLMMGPIO", 
                            Package (0x07)
                            {
                                0x13, 
                                One, 
                                Zero, 
                                One, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "DELAY", 
                            Package (0x01)
                            {
                                0x64
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO6_B", 
                                One, 
                                0x00124F80, 
                                One, 
                                One, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                0x000D6D80, 
                                One, 
                                One, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_gdsc", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_PCIE_1_CFG", 
                                0x047868C0, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_PCIE_1", 
                                "ICBID_SLAVE_EBI1", 
                                0xB2D05E00, 
                                0xB2D05E00
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x09)
                            {
                                "gcc_pcie_1_pipe_clk", 
                                0x06, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_noc_pcie_center_sf_axi_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_aux_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_pipe_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_slv_axi_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_slv_q2a_axi_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_ddrss_pcie_sf_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_throttle_core_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_throttle_pcie_ahb_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_mstr_axi_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_noc_pcie_1_axi_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_cfg_ahb_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_pcie_1_aux_clk", 
                                0x08, 
                                0x0124F800, 
                                0x03
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_pcie1_phy_rchng_clk", 
                                0x08, 
                                0x05F5E100, 
                                0x03
                            }
                        }, 

                        Package (0x02)
                        {
                            "TLMMGPIO", 
                            Package (0x07)
                            {
                                0x4F, 
                                Zero, 
                                0x03, 
                                Zero, 
                                0x03, 
                                Zero, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        One
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        0x02
                    }, 

                    Package (0x16)
                    {
                        "DSTATE", 
                        0x03, 
                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_aux_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_slv_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_slv_q2a_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_mstr_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_throttle_core_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_throttle_pcie_ahb_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_ddrss_pcie_sf_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_noc_pcie_1_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_cfg_ahb_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie1_phy_rchng_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_pipe_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_noc_pcie_center_sf_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x09)
                            {
                                "gcc_pcie_1_pipe_clk", 
                                0x06, 
                                Zero, 
                                Zero, 
                                0x02, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_PCIE_1_CFG", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_PCIE_1", 
                                "ICBID_SLAVE_EBI1", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_gdsc", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO6_B", 
                                One, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "TLMMGPIO", 
                            Package (0x07)
                            {
                                0x13, 
                                Zero, 
                                Zero, 
                                One, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "DELAY", 
                            Package (0x01)
                            {
                                0x64
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "CRASHDUMP_EXCEPTION", 
                        Package (0x02)
                        {
                            "EXECUTE_FUNCTION", 
                            Package (0x01)
                            {
                                "ExecuteOcdPCIeExceptions"
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "CRASHDUMP_DSTATE", 
                        Zero
                    }, 

                    Package (0x02)
                    {
                        "ABANDON_DSTATE", 
                        Zero
                    }
                }, 

                Package (0x08)
                {
                    "DEVICE", 
                    "\\_SB.PCI1.RP1", 
                    Package (0x04)
                    {
                        "COMPONENT", 
                        Zero, 
                        Package (0x02)
                        {
                            "FSTATE", 
                            Zero
                        }, 

                        Package (0x02)
                        {
                            "FSTATE", 
                            One
                        }
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        Zero
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        One
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        0x02
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        0x03
                    }, 

                    Package (0x02)
                    {
                        "ABANDON_DSTATE", 
                        Zero
                    }
                }
            })
            Name (PEMX, Package (0x04)
            {
                Package (0x0A)
                {
                    "DEVICE", 
                    "\\_SB.PCI0", 
                    Package (0x04)
                    {
                        "COMPONENT", 
                        Zero, 
                        Package (0x02)
                        {
                            "FSTATE", 
                            Zero
                        }, 

                        Package (0x02)
                        {
                            "FSTATE", 
                            One
                        }
                    }, 

                    Package (0x1A)
                    {
                        "DSTATE", 
                        Zero, 
                        Package (0x02)
                        {
                            "TLMMGPIO", 
                            Package (0x06)
                            {
                                0x54, 
                                One, 
                                Zero, 
                                One, 
                                0x03, 
                                0x04
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS1_B", 
                                0x02, 
                                0x001C5200, 
                                One, 
                                0x06, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS7_B", 
                                0x02, 
                                0x000E86C0, 
                                One, 
                                0x06, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS8_B", 
                                0x02, 
                                0x00132A40, 
                                One, 
                                0x06, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO11_C", 
                                One, 
                                0x002AB980, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO19_B", 
                                One, 
                                0x001B7740, 
                                One, 
                                0x07, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_gdsc", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_PCIE_0_CFG", 
                                0x047868C0, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_PCIE_0", 
                                "ICBID_SLAVE_EBI1", 
                                0xB2D05E00, 
                                0xB2D05E00
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x09)
                            {
                                "gcc_pcie_0_pipe_clk", 
                                0x06, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_noc_pcie_center_sf_axi_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_aux_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_pipe_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_slv_axi_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_slv_q2a_axi_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_ddrss_pcie_sf_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_throttle_core_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_throttle_pcie_ahb_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_mstr_axi_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_noc_pcie_0_axi_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_cfg_ahb_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_pcie_0_aux_clk", 
                                0x08, 
                                0x0124F800, 
                                0x03
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_pcie0_phy_rchng_clk", 
                                0x08, 
                                0x05F5E100, 
                                0x03
                            }
                        }, 

                        Package (0x02)
                        {
                            "TLMMGPIO", 
                            Package (0x07)
                            {
                                0x58, 
                                Zero, 
                                One, 
                                Zero, 
                                0x03, 
                                Zero, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        One
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        0x02
                    }, 

                    Package (0x17)
                    {
                        "DSTATE", 
                        0x03, 
                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_aux_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_slv_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_slv_q2a_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_mstr_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_throttle_core_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_throttle_pcie_ahb_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_ddrss_pcie_sf_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_noc_pcie_0_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_cfg_ahb_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie0_phy_rchng_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_pipe_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_noc_pcie_center_sf_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x09)
                            {
                                "gcc_pcie_0_pipe_clk", 
                                0x06, 
                                Zero, 
                                Zero, 
                                0x02, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_PCIE_0_CFG", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_PCIE_0", 
                                "ICBID_SLAVE_EBI1", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_pcie_0_gdsc", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO11_C", 
                                One, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO19_B", 
                                One, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS1_B", 
                                0x02, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS7_B", 
                                0x02, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_SMPS8_B", 
                                0x02, 
                                Zero, 
                                Zero, 
                                0x04, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "CRASHDUMP_EXCEPTION", 
                        Package (0x02)
                        {
                            "EXECUTE_FUNCTION", 
                            Package (0x01)
                            {
                                "ExecuteOcdPCIeExceptions"
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "CRASHDUMP_DSTATE", 
                        Zero
                    }, 

                    Package (0x02)
                    {
                        "ABANDON_DSTATE", 
                        Zero
                    }
                }, 

                Package (0x08)
                {
                    "DEVICE", 
                    "\\_SB.PCI0.RP1", 
                    Package (0x04)
                    {
                        "COMPONENT", 
                        Zero, 
                        Package (0x02)
                        {
                            "FSTATE", 
                            Zero
                        }, 

                        Package (0x02)
                        {
                            "FSTATE", 
                            One
                        }
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        Zero
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        One
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        0x02
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        0x03
                    }, 

                    Package (0x02)
                    {
                        "ABANDON_DSTATE", 
                        Zero
                    }
                }, 

                Package (0x0A)
                {
                    "DEVICE", 
                    "\\_SB.PCI1", 
                    Package (0x04)
                    {
                        "COMPONENT", 
                        Zero, 
                        Package (0x02)
                        {
                            "FSTATE", 
                            Zero
                        }, 

                        Package (0x02)
                        {
                            "FSTATE", 
                            One
                        }
                    }, 

                    Package (0x18)
                    {
                        "DSTATE", 
                        Zero, 
                        Package (0x02)
                        {
                            "TLMMGPIO", 
                            Package (0x07)
                            {
                                0x33, 
                                One, 
                                Zero, 
                                One, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "DELAY", 
                            Package (0x01)
                            {
                                0x64
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO6_B", 
                                One, 
                                0x00124F80, 
                                One, 
                                One, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                0x000D6D80, 
                                One, 
                                One, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_gdsc", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_PCIE_1_CFG", 
                                0x047868C0, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_PCIE_1", 
                                "ICBID_SLAVE_EBI1", 
                                0xB2D05E00, 
                                0xB2D05E00
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x09)
                            {
                                "gcc_pcie_1_pipe_clk", 
                                0x06, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_noc_pcie_center_sf_axi_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_aux_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_pipe_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_slv_axi_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_slv_q2a_axi_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_ddrss_pcie_sf_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_throttle_core_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_throttle_pcie_ahb_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_mstr_axi_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_noc_pcie_1_axi_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_cfg_ahb_clk", 
                                One
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_pcie_1_aux_clk", 
                                0x08, 
                                0x0124F800, 
                                0x03
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x04)
                            {
                                "gcc_pcie1_phy_rchng_clk", 
                                0x08, 
                                0x05F5E100, 
                                0x03
                            }
                        }, 

                        Package (0x02)
                        {
                            "TLMMGPIO", 
                            Package (0x07)
                            {
                                0x4F, 
                                Zero, 
                                0x03, 
                                Zero, 
                                0x03, 
                                Zero, 
                                Zero
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        One
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        0x02
                    }, 

                    Package (0x16)
                    {
                        "DSTATE", 
                        0x03, 
                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_aux_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_slv_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_slv_q2a_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_mstr_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_throttle_core_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_throttle_pcie_ahb_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_ddrss_pcie_sf_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_noc_pcie_1_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_cfg_ahb_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie1_phy_rchng_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_pipe_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x02)
                            {
                                "gcc_aggre_noc_pcie_center_sf_axi_clk", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "CLOCK", 
                            Package (0x09)
                            {
                                "gcc_pcie_1_pipe_clk", 
                                0x06, 
                                Zero, 
                                Zero, 
                                0x02, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_APPSS_PROC", 
                                "ICBID_SLAVE_PCIE_1_CFG", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "BUSARB", 
                            Package (0x05)
                            {
                                0x03, 
                                "ICBID_MASTER_PCIE_1", 
                                "ICBID_SLAVE_EBI1", 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "FOOTSWITCH", 
                            Package (0x02)
                            {
                                "gcc_pcie_1_gdsc", 
                                0x02
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO10_C", 
                                One, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "PMICVREGVOTE", 
                            Package (0x06)
                            {
                                "PPP_RESOURCE_ID_LDO6_B", 
                                One, 
                                Zero, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "TLMMGPIO", 
                            Package (0x07)
                            {
                                0x33, 
                                Zero, 
                                Zero, 
                                One, 
                                Zero, 
                                Zero, 
                                Zero
                            }
                        }, 

                        Package (0x02)
                        {
                            "DELAY", 
                            Package (0x01)
                            {
                                0x64
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "CRASHDUMP_EXCEPTION", 
                        Package (0x02)
                        {
                            "EXECUTE_FUNCTION", 
                            Package (0x01)
                            {
                                "ExecuteOcdPCIeExceptions"
                            }
                        }
                    }, 

                    Package (0x02)
                    {
                        "CRASHDUMP_DSTATE", 
                        Zero
                    }, 

                    Package (0x02)
                    {
                        "ABANDON_DSTATE", 
                        Zero
                    }
                }, 

                Package (0x08)
                {
                    "DEVICE", 
                    "\\_SB.PCI1.RP1", 
                    Package (0x04)
                    {
                        "COMPONENT", 
                        Zero, 
                        Package (0x02)
                        {
                            "FSTATE", 
                            Zero
                        }, 

                        Package (0x02)
                        {
                            "FSTATE", 
                            One
                        }
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        Zero
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        One
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        0x02
                    }, 

                    Package (0x02)
                    {
                        "DSTATE", 
                        0x03
                    }, 

                    Package (0x02)
                    {
                        "ABANDON_DSTATE", 
                        Zero
                    }
                }
            })
        }

        Device (BAM1)
        {
            Name (_HID, "QCOM0A0A")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, One)  // _UID: Unique ID
            Name (_CCA, Zero)  // _CCA: Cache Coherency Attribute
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x17)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x40, 0xDC, 0x01,  // .....@..
                    /* 0008 */  0x00, 0x40, 0x02, 0x00, 0x89, 0x06, 0x00, 0x01,  // .@......
                    /* 0010 */  0x01, 0x30, 0x01, 0x00, 0x00, 0x79, 0x00         // .0...y.
                })
                Return (RBUF) /* \_SB_.BAM1._CRS.RBUF */
            }
        }

        Device (BAM5)
        {
            Name (_HID, "QCOM0A0A")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, 0x05)  // _UID: Unique ID
            Name (_CCA, Zero)  // _CCA: Cache Coherency Attribute
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x17)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x40, 0xA8, 0x03,  // .....@..
                    /* 0008 */  0x00, 0x20, 0x03, 0x00, 0x89, 0x06, 0x00, 0x01,  // . ......
                    /* 0010 */  0x01, 0xC4, 0x00, 0x00, 0x00, 0x79, 0x00         // .....y.
                })
                Return (RBUF) /* \_SB_.BAM5._CRS.RBUF */
            }
        }

        Device (BAME)
        {
            Name (_HID, "QCOM0A0A")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, 0x0E)  // _UID: Unique ID
            Name (_CCA, Zero)  // _CCA: Cache Coherency Attribute
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x17)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x40, 0x06, 0x06,  // .....@..
                    /* 0008 */  0x00, 0x50, 0x01, 0x00, 0x89, 0x06, 0x00, 0x01,  // .P......
                    /* 0010 */  0x01, 0xC7, 0x00, 0x00, 0x00, 0x79, 0x00         // .....y.
                })
                Return (RBUF) /* \_SB_.BAME._CRS.RBUF */
            }
        }

        Device (BAMF)
        {
            Name (_HID, "QCOM0A0A")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, 0x0F)  // _UID: Unique ID
            Name (_CCA, Zero)  // _CCA: Cache Coherency Attribute
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x17)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x40, 0x70, 0x0A,  // .....@p.
                    /* 0008 */  0x00, 0x70, 0x01, 0x00, 0x89, 0x06, 0x00, 0x01,  // .p......
                    /* 0010 */  0x01, 0xA4, 0x00, 0x00, 0x00, 0x79, 0x00         // .....y.
                })
                Return (RBUF) /* \_SB_.BAMF._CRS.RBUF */
            }
        }

        Device (I2C2)
        {
            Name (_HID, "QCOM0A10")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, 0x02)  // _UID: Unique ID
            Name (_DEP, Package (0x03)  // _DEP: Dependencies
            {
                \_SB.PEP0, , 
                \_SB.QGP0, , 
                \_SB.MMU0, 
            })
            Name (_CCA, Zero)  // _CCA: Cache Coherency Attribute
            Name (_STR, Unicode ("QUP_0_SE_2,Shared"))  // _STR: Description String
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x17)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x80, 0x98, 0x00,  // .....@..
                    /* 0008 */  0x00, 0x40, 0x00, 0x00, 0x89, 0x06, 0x00, 0x01,  // .@......
                    /* 0010 */  0x01, 0x7A, 0x02, 0x00, 0x00, 0x79, 0x00         // .z...y.
                })
                Return (RBUF) /* \_SB_.I2C2._CRS.RBUF */
            }
        }

        Device (I2C4)
        {
            Name (_HID, "QCOM0A10")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, 0x04)  // _UID: Unique ID
            Name (_DEP, Package (0x01)  // _DEP: Dependencies
            {
                \_SB.PEP0, 
            })
            Name (_CCA, Zero)  // _CCA: Cache Coherency Attribute
            Name (_STR, Unicode ("QUP_0_SE_3"))  // _STR: Description String
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x17)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0xC0, 0x98, 0x00,  // ........
                    /* 0008 */  0x00, 0x40, 0x00, 0x00, 0x89, 0x06, 0x00, 0x01,  // .@......
                    /* 0010 */  0x01, 0x7C, 0x02, 0x00, 0x00, 0x79, 0x00         // .|...y.
                })
                Return (RBUF) /* \_SB_.I2C4._CRS.RBUF */
            }
        }

        Device (UAR5)
        {
            Name (_HID, "QCOM0A16")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, 0x05)  // _UID: Unique ID
            Name (_DEP, Package (0x01)  // _DEP: Dependencies
            {
                \_SB.PEP0, 
            })
            Name (_CCA, Zero)  // _CCA: Cache Coherency Attribute
            Name (_STR, Unicode ("QUP_0_SE_4"))  // _STR: Description String
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x3A)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0x99, 0x00,  // ........
                    /* 0008 */  0x00, 0x40, 0x00, 0x00, 0x89, 0x06, 0x00, 0x01,  // .@......
                    /* 0010 */  0x01, 0x7D, 0x02, 0x00, 0x00, 0x8C, 0x20, 0x00,  // .}.... .
                    /* 0018 */  0x01, 0x00, 0x01, 0x00, 0x03, 0x00, 0x02, 0x00,  // ........
                    /* 0020 */  0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x19, 0x00,  // ........
                    /* 0028 */  0x23, 0x00, 0x00, 0x00, 0x13, 0x00, 0x5C, 0x5F,  // #.....\_
                    /* 0030 */  0x53, 0x42, 0x2E, 0x47, 0x49, 0x4F, 0x30, 0x00,  // SB.GIO0.
                    /* 0038 */  0x79, 0x00                                       // y.
                })
                Return (RBUF) /* \_SB_.UAR5._CRS.RBUF */
            }
        }

        Device (UARD)
        {
            Name (_HID, "QCOM0A16")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, 0x06)  // _UID: Unique ID
            Name (_DEP, Package (0x01)  // _DEP: Dependencies
            {
                \_SB.PEP0, 
            })
            Name (_CCA, Zero)  // _CCA: Cache Coherency Attribute
            Name (_STR, Unicode ("QUP_0_SE_5,DBG"))  // _STR: Description String
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x3A)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x40, 0x99, 0x00,  // .....@..
                    /* 0008 */  0x00, 0x40, 0x00, 0x00, 0x89, 0x06, 0x00, 0x01,  // .@......
                    /* 0010 */  0x01, 0x7E, 0x02, 0x00, 0x00, 0x8C, 0x20, 0x00,  // .~.... .
                    /* 0018 */  0x01, 0x00, 0x01, 0x00, 0x03, 0x00, 0x02, 0x00,  // ........
                    /* 0020 */  0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x19, 0x00,  // ........
                    /* 0028 */  0x23, 0x00, 0x00, 0x00, 0x17, 0x00, 0x5C, 0x5F,  // #.....\_
                    /* 0030 */  0x53, 0x42, 0x2E, 0x47, 0x49, 0x4F, 0x30, 0x00,  // SB.GIO0.
                    /* 0038 */  0x79, 0x00                                       // y.
                })
                Return (RBUF) /* \_SB_.UARD._CRS.RBUF */
            }
        }

        Device (UAR8)
        {
            Name (_HID, "QCOM0A16")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, 0x08)  // _UID: Unique ID
            Name (_DEP, Package (0x01)  // _DEP: Dependencies
            {
                \_SB.PEP0, 
            })
            Name (_CCA, Zero)  // _CCA: Cache Coherency Attribute
            Name (_STR, Unicode ("QUP_0_SE_7,4W,BT"))  // _STR: Description String
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x3A)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0xC0, 0x99, 0x00,  // ........
                    /* 0008 */  0x00, 0x40, 0x00, 0x00, 0x89, 0x06, 0x00, 0x01,  // .@......
                    /* 0010 */  0x01, 0x80, 0x02, 0x00, 0x00, 0x8C, 0x20, 0x00,  // ...... .
                    /* 0018 */  0x01, 0x00, 0x01, 0x00, 0x03, 0x00, 0x02, 0x00,  // ........
                    /* 0020 */  0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x19, 0x00,  // ........
                    /* 0028 */  0x23, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x5C, 0x5F,  // #.....\_
                    /* 0030 */  0x53, 0x42, 0x2E, 0x47, 0x49, 0x4F, 0x30, 0x00,  // SB.GIO0.
                    /* 0038 */  0x79, 0x00                                       // y.
                })
                Return (RBUF) /* \_SB_.UAR8._CRS.RBUF */
            }

            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0B)
            }
        }

        Device (RPEN)
        {
            Name (_HID, "QCOM06E1")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }
        }

        Device (PILC)
        {
            Name (_HID, "QCOM06E0")  // _HID: Hardware ID
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }
        }

        Device (CDI)
        {
            Name (_DEP, Package (0x02)  // _DEP: Dependencies
            {
                \_SB.PILC, , 
                \_SB.RPEN, 
            })
            Name (_HID, "QCOM0A2F")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }
        }

        Device (ADSP)
        {
            Name (_DEP, Package (0x09)  // _DEP: Dependencies
            {
                \_SB.PEP0, , 
                \_SB.PILC, , 
                \_SB.GLNK, , 
                \_SB.IPC0, , 
                \_SB.RPEN, , 
                \_SB.SSDD, , 
                \_SB.ARPC, , 
                \_SB.TFTP, , 
                \_SB.PDSR, 
            })
            Name (_HID, "QCOM0A1B")  // _HID: Hardware ID
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }

            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x0B)
                {
                    /* 0000 */  0x89, 0x06, 0x00, 0x03, 0x01, 0x06, 0x02, 0x00,  // ........
                    /* 0008 */  0x00, 0x79, 0x00                                 // .y.
                })
                Return (RBUF) /* \_SB_.ADSP._CRS.RBUF */
            }

            Device (SLM1)
            {
                Name (_ADR, Zero)  // _ADR: Address
                Name (_CCA, Zero)  // _CCA: Cache Coherency Attribute
                Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
                {
                    Name (RBUF, Buffer (0x17)
                    {
                        /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0xAC, 0x03,  // ........
                        /* 0008 */  0x00, 0xC0, 0x02, 0x00, 0x89, 0x06, 0x00, 0x01,  // ........
                        /* 0010 */  0x01, 0xC3, 0x00, 0x00, 0x00, 0x79, 0x00         // .....y.
                    })
                    Return (RBUF) /* \_SB_.ADSP.SLM1._CRS.RBUF */
                }
            }

            Device (ADCM)
            {
                Name (_ADR, One)  // _ADR: Address
                Name (_DEP, Package (0x02)  // _DEP: Dependencies
                {
                    \_SB.MMU0, , 
                    \_SB.IMM0, 
                })
                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (0x0F)
                }

                Method (CHLD, 0, NotSerialized)
                {
                    Return (Package (0x01)
                    {
                        "ADCM\\QCOM0AC1"
                    })
                }

                Device (AUCD)
                {
                    Name (_ADR, Zero)  // _ADR: Address
                    Method (_STA, 0, NotSerialized)  // _STA: Status
                    {
                        Return (0x0F)
                    }

                    Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
                    {
                        If (((((\_SB.SKUV == 0x1F) || (\_SB.SKUV == 0x20)) || (\_SB.SKUV == 
                            0x21)) || (\_SB.SKUV == 0x22)))
                        {
                            If ((\_SB.PLST != 0x09))
                            {
                                Name (RBUF, Buffer (0x63)
                                {
                                    /* 0000 */  0x8C, 0x20, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00,  // . ......
                                    /* 0008 */  0x00, 0x03, 0x40, 0x06, 0x00, 0x00, 0x17, 0x00,  // ..@.....
                                    /* 0010 */  0x00, 0x19, 0x00, 0x23, 0x00, 0x00, 0x00, 0x9E,  // ...#....
                                    /* 0018 */  0x00, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x47, 0x49,  // .\_SB.GI
                                    /* 0020 */  0x4F, 0x30, 0x00, 0x8C, 0x20, 0x00, 0x01, 0x00,  // O0.. ...
                                    /* 0028 */  0x01, 0x00, 0x15, 0x00, 0x02, 0x00, 0x00, 0x00,  // ........
                                    /* 0030 */  0x00, 0x17, 0x00, 0x00, 0x19, 0x00, 0x23, 0x00,  // ......#.
                                    /* 0038 */  0x00, 0x00, 0x00, 0x01, 0x5C, 0x5F, 0x53, 0x42,  // ....\_SB
                                    /* 0040 */  0x2E, 0x47, 0x49, 0x4F, 0x30, 0x00, 0x89, 0x06,  // .GIO0...
                                    /* 0048 */  0x00, 0x03, 0x01, 0x10, 0x02, 0x00, 0x00, 0x89,  // ........
                                    /* 0050 */  0x06, 0x00, 0x03, 0x01, 0xBB, 0x00, 0x00, 0x00,  // ........
                                    /* 0058 */  0x89, 0x06, 0x00, 0x03, 0x01, 0xCA, 0x00, 0x00,  // ........
                                    /* 0060 */  0x00, 0x79, 0x00                                 // .y.
                                })
                                Return (RBUF) /* \_SB_.ADSP.ADCM.AUCD._CRS.RBUF */
                            }
                            Else
                            {
                                Name (BUFF, Buffer (0x86)
                                {
                                    /* 0000 */  0x8C, 0x20, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00,  // . ......
                                    /* 0008 */  0x00, 0x03, 0x40, 0x06, 0x00, 0x00, 0x17, 0x00,  // ..@.....
                                    /* 0010 */  0x00, 0x19, 0x00, 0x23, 0x00, 0x00, 0x00, 0x53,  // ...#...S
                                    /* 0018 */  0x00, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x47, 0x49,  // .\_SB.GI
                                    /* 0020 */  0x4F, 0x30, 0x00, 0x8C, 0x20, 0x00, 0x01, 0x01,  // O0.. ...
                                    /* 0028 */  0x01, 0x00, 0x00, 0x00, 0x03, 0x40, 0x06, 0x00,  // .....@..
                                    /* 0030 */  0x00, 0x17, 0x00, 0x00, 0x19, 0x00, 0x23, 0x00,  // ......#.
                                    /* 0038 */  0x00, 0x00, 0x9E, 0x00, 0x5C, 0x5F, 0x53, 0x42,  // ....\_SB
                                    /* 0040 */  0x2E, 0x47, 0x49, 0x4F, 0x30, 0x00, 0x8C, 0x20,  // .GIO0.. 
                                    /* 0048 */  0x00, 0x01, 0x00, 0x01, 0x00, 0x15, 0x00, 0x02,  // ........
                                    /* 0050 */  0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x19,  // ........
                                    /* 0058 */  0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x01, 0x5C,  // .#.....\
                                    /* 0060 */  0x5F, 0x53, 0x42, 0x2E, 0x47, 0x49, 0x4F, 0x30,  // _SB.GIO0
                                    /* 0068 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x10, 0x02,  // ........
                                    /* 0070 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0xBB,  // ........
                                    /* 0078 */  0x00, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01,  // ........
                                    /* 0080 */  0xCA, 0x00, 0x00, 0x00, 0x79, 0x00               // ....y.
                                })
                                Return (BUFF) /* \_SB_.ADSP.ADCM.AUCD._CRS.BUFF */
                            }
                        }
                        Else
                        {
                            Name (DBUF, Buffer (0x63)
                            {
                                /* 0000 */  0x8C, 0x20, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00,  // . ......
                                /* 0008 */  0x00, 0x03, 0x40, 0x06, 0x00, 0x00, 0x17, 0x00,  // ..@.....
                                /* 0010 */  0x00, 0x19, 0x00, 0x23, 0x00, 0x00, 0x00, 0x53,  // ...#...S
                                /* 0018 */  0x00, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x47, 0x49,  // .\_SB.GI
                                /* 0020 */  0x4F, 0x30, 0x00, 0x8C, 0x20, 0x00, 0x01, 0x00,  // O0.. ...
                                /* 0028 */  0x01, 0x00, 0x15, 0x00, 0x02, 0x00, 0x00, 0x00,  // ........
                                /* 0030 */  0x00, 0x17, 0x00, 0x00, 0x19, 0x00, 0x23, 0x00,  // ......#.
                                /* 0038 */  0x00, 0x00, 0x00, 0x01, 0x5C, 0x5F, 0x53, 0x42,  // ....\_SB
                                /* 0040 */  0x2E, 0x47, 0x49, 0x4F, 0x30, 0x00, 0x89, 0x06,  // .GIO0...
                                /* 0048 */  0x00, 0x03, 0x01, 0x10, 0x02, 0x00, 0x00, 0x89,  // ........
                                /* 0050 */  0x06, 0x00, 0x03, 0x01, 0xBB, 0x00, 0x00, 0x00,  // ........
                                /* 0058 */  0x89, 0x06, 0x00, 0x03, 0x01, 0xCA, 0x00, 0x00,  // ........
                                /* 0060 */  0x00, 0x79, 0x00                                 // .y.
                            })
                            Return (DBUF) /* \_SB_.ADSP.ADCM.AUCD._CRS.DBUF */
                        }
                    }

                    Method (CHLD, 0, NotSerialized)
                    {
                        Name (CH, Package (0x01)
                        {
                            "AUCD\\QCOM0A29"
                        })
                        Return (CH) /* \_SB_.ADSP.ADCM.AUCD.CHLD.CH__ */
                    }

                    Device (QCRT)
                    {
                        Method (_STA, 0, NotSerialized)  // _STA: Status
                        {
                            Return (0x0F)
                        }

                        Name (_ADR, Zero)  // _ADR: Address
                    }
                }
            }
        }

        Device (AMSS)
        {
            Name (_CCA, Zero)  // _CCA: Cache Coherency Attribute
            Name (_DEP, Package (0x09)  // _DEP: Dependencies
            {
                \_SB.PEP0, , 
                \_SB.GLNK, , 
                \_SB.PILC, , 
                \_SB.RFS0, , 
                \_SB.RPEN, , 
                \_SB.SSDD, , 
                \_SB.IPC0, , 
                \_SB.TFTP, , 
                \_SB.PDSR, 
            })
            Name (_HID, "QCOM0A1C")  // _HID: Hardware ID
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x0B)
                {
                    /* 0000 */  0x89, 0x06, 0x00, 0x03, 0x01, 0x28, 0x01, 0x00,  // .....(..
                    /* 0008 */  0x00, 0x79, 0x00                                 // .y.
                })
                Return (RBUF) /* \_SB_.AMSS._CRS.RBUF */
            }
        }

        Device (QSM)
        {
            Name (_HID, "QCOM0A1E")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (0x04)  // _DEP: Dependencies
            {
                \_SB.GLNK, , 
                \_SB.IPC0, , 
                \_SB.PILC, , 
                \_SB.RPEN, 
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }
        }

        Device (SSDD)
        {
            Name (_HID, "QCOM0A20")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.GLNK, 
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }
        }

        Device (PDSR)
        {
            Name (_HID, "QCOM06DF")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (0x03)  // _DEP: Dependencies
            {
                \_SB.PEP0, , 
                \_SB.GLNK, , 
                \_SB.IPC0, 
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }
        }

        Device (NSP0)
        {
            Name (_DEP, Package (0x08)  // _DEP: Dependencies
            {
                \_SB.PEP0, , 
                \_SB.PILC, , 
                \_SB.GLNK, , 
                \_SB.IPC0, , 
                \_SB.RPEN, , 
                \_SB.SSDD, , 
                \_SB.ARPC, , 
                \_SB.PDSR, 
            })
            Name (_HID, "QCOM0AB0")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x0B)
                {
                    /* 0000 */  0x89, 0x06, 0x00, 0x03, 0x01, 0x62, 0x02, 0x00,  // .....b..
                    /* 0008 */  0x00, 0x79, 0x00                                 // .y.
                })
                Return (RBUF) /* \_SB_.NSP0._CRS.RBUF */
            }
        }

        Device (WPSS)
        {
            Name (_DEP, Package (0x08)  // _DEP: Dependencies
            {
                \_SB.PEP0, , 
                \_SB.PILC, , 
                \_SB.GLNK, , 
                \_SB.IPC0, , 
                \_SB.RPEN, , 
                \_SB.SSDD, , 
                \_SB.TFTP, , 
                \_SB.PDSR, 
            })
            Name (_HID, "QCOM0AE2")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If ((\_SB.PSUB == "IDP06490"))
                {
                    If ((\_SB.PLST == Zero))
                    {
                        Return (0x0F)
                    }
                    Else
                    {
                        Return (Zero)
                    }
                }
                ElseIf ((\_SB.PSUB == "IOT06490"))
                {
                    If (((((\_SB.PLST == One) || (\_SB.PLST == 0x02)) || (\_SB.PLST == 
                        0x03)) || (\_SB.PLST == 0x05)))
                    {
                        Return (0x0F)
                    }
                    Else
                    {
                        Return (Zero)
                    }
                }
                Else
                {
                    Return (Zero)
                }
            }

            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x0B)
                {
                    /* 0000 */  0x89, 0x06, 0x00, 0x03, 0x01, 0x6B, 0x02, 0x00,  // .....k..
                    /* 0008 */  0x00, 0x79, 0x00                                 // .y.
                })
                Return (RBUF) /* \_SB_.WPSS._CRS.RBUF */
            }

            Device (QWLN)
            {
                Name (_ADR, Zero)  // _ADR: Address
                Name (_DEP, Package (0x03)  // _DEP: Dependencies
                {
                    \_SB.PEP0, , 
                    \_SB.MMU0, , 
                    \_SB.IPC0, 
                })
                Name (_PRW, Package (0x02)  // _PRW: Power Resources for Wake
                {
                    Zero, 
                    Zero
                })
                Name (_S0W, 0x03)  // _S0W: S0 Device Wake State
                Name (_S4W, 0x03)  // _S4W: S4 Device Wake State
                Name (_PRR, Package (One)  // _PRR: Power Resource for Reset
                {
                    \_SB.WPSS.QWLN.WRST, 
                })
                Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
                {
                    Name (RBUF, Buffer (0x013A)
                    {
                        /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x40, 0x00, 0xA1, 0x17,  // ....@...
                        /* 0008 */  0x10, 0x00, 0x00, 0x00, 0x86, 0x09, 0x00, 0x01,  // ........
                        /* 0010 */  0x00, 0x00, 0xC0, 0x80, 0x00, 0x00, 0xC0, 0x00,  // ........
                        /* 0018 */  0x89, 0x06, 0x00, 0x03, 0x01, 0x20, 0x03, 0x00,  // ..... ..
                        /* 0020 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x21, 0x03,  // ......!.
                        /* 0028 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x22,  // ......."
                        /* 0030 */  0x03, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01,  // ........
                        /* 0038 */  0x23, 0x03, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03,  // #.......
                        /* 0040 */  0x01, 0x24, 0x03, 0x00, 0x00, 0x89, 0x06, 0x00,  // .$......
                        /* 0048 */  0x03, 0x01, 0x25, 0x03, 0x00, 0x00, 0x89, 0x06,  // ..%.....
                        /* 0050 */  0x00, 0x03, 0x01, 0x26, 0x03, 0x00, 0x00, 0x89,  // ...&....
                        /* 0058 */  0x06, 0x00, 0x03, 0x01, 0x27, 0x03, 0x00, 0x00,  // ....'...
                        /* 0060 */  0x89, 0x06, 0x00, 0x03, 0x01, 0x28, 0x03, 0x00,  // .....(..
                        /* 0068 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x29, 0x03,  // ......).
                        /* 0070 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x2A,  // .......*
                        /* 0078 */  0x03, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01,  // ........
                        /* 0080 */  0x2B, 0x03, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03,  // +.......
                        /* 0088 */  0x01, 0x2C, 0x03, 0x00, 0x00, 0x89, 0x06, 0x00,  // .,......
                        /* 0090 */  0x03, 0x01, 0x2D, 0x03, 0x00, 0x00, 0x89, 0x06,  // ..-.....
                        /* 0098 */  0x00, 0x03, 0x01, 0x2E, 0x03, 0x00, 0x00, 0x89,  // ........
                        /* 00A0 */  0x06, 0x00, 0x03, 0x01, 0x2F, 0x03, 0x00, 0x00,  // ..../...
                        /* 00A8 */  0x89, 0x06, 0x00, 0x03, 0x01, 0x30, 0x03, 0x00,  // .....0..
                        /* 00B0 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x31, 0x03,  // ......1.
                        /* 00B8 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x32,  // .......2
                        /* 00C0 */  0x03, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01,  // ........
                        /* 00C8 */  0x33, 0x03, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03,  // 3.......
                        /* 00D0 */  0x01, 0x34, 0x03, 0x00, 0x00, 0x89, 0x06, 0x00,  // .4......
                        /* 00D8 */  0x03, 0x01, 0x35, 0x03, 0x00, 0x00, 0x89, 0x06,  // ..5.....
                        /* 00E0 */  0x00, 0x03, 0x01, 0x36, 0x03, 0x00, 0x00, 0x89,  // ...6....
                        /* 00E8 */  0x06, 0x00, 0x03, 0x01, 0x37, 0x03, 0x00, 0x00,  // ....7...
                        /* 00F0 */  0x89, 0x06, 0x00, 0x03, 0x01, 0x38, 0x03, 0x00,  // .....8..
                        /* 00F8 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x39, 0x03,  // ......9.
                        /* 0100 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x3A,  // .......:
                        /* 0108 */  0x03, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01,  // ........
                        /* 0110 */  0x3B, 0x03, 0x00, 0x00, 0x89, 0x06, 0x00, 0x13,  // ;.......
                        /* 0118 */  0x01, 0x3C, 0x03, 0x00, 0x00, 0x89, 0x06, 0x00,  // .<......
                        /* 0120 */  0x03, 0x01, 0x3D, 0x03, 0x00, 0x00, 0x89, 0x06,  // ..=.....
                        /* 0128 */  0x00, 0x03, 0x01, 0x3E, 0x03, 0x00, 0x00, 0x89,  // ...>....
                        /* 0130 */  0x06, 0x00, 0x03, 0x01, 0x3F, 0x03, 0x00, 0x00,  // ....?...
                        /* 0138 */  0x79, 0x00                                       // y.
                    })
                    Return (RBUF) /* \_SB_.WPSS.QWLN._CRS.RBUF */
                }

                PowerResource (WRST, 0x05, 0x0000)
                {
                    Method (_STA, 0, NotSerialized)  // _STA: Status
                    {
                        Return (0x0F)
                    }

                    Method (_ON, 0, NotSerialized)  // _ON_: Power On
                    {
                    }

                    Method (_OFF, 0, NotSerialized)  // _OFF: Power Off
                    {
                    }

                    Method (_RST, 0, NotSerialized)  // _RST: Device Reset
                    {
                    }
                }
            }

            Scope (\_SB)
            {
                Device (WLTM)
                {
                    Name (_HID, "QCOM0AD5")  // _HID: Hardware ID
                    Name (_CID, "QCOMFFE0")  // _CID: Compatible ID
                    Alias (\_SB.PSUB, _SUB)
                    Name (_DEP, Package (0x02)  // _DEP: Dependencies
                    {
                        \_SB.WPSS, , 
                        \_SB.SBTD, 
                    })
                }
            }
        }

        Device (CSW0)
        {
            Name (_HID, "QCOM0AC3")  // _HID: Hardware ID
            Name (_CID, "QCOMFFE0")  // _CID: Compatible ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (0x02)  // _DEP: Dependencies
            {
                \_SB.NSP0, , 
                \_SB.SBTD, 
            })
        }

        Device (SBTD)
        {
            Name (_HID, "QCOM06E5")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
        }

        Device (TFTP)
        {
            Name (_HID, "QCOM06DC")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.IPC0, 
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }
        }

        Device (QCSK)
        {
            Name (_HID, "QCOM0AAC")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (Zero)
            }
        }

        Scope (\_SB.ADSP)
        {
        }

        Scope (\_SB.AMSS)
        {
        }

        Scope (\_SB.PILC)
        {
        }

        Scope (\_SB.CDI)
        {
        }

        Scope (\_SB.RPEN)
        {
        }

        Scope (\_SB.NSP0)
        {
            Name (_CID, "QCOMFFE8")  // _CID: Compatible ID
        }

        Scope (\_SB.AMSS)
        {
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x1F) || (\_SB.SKUV == 0x21)))
                {
                    Return (0x0F)
                }
                ElseIf (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (Zero)
                }
            }

            Method (_SUB, 0, NotSerialized)  // _SUB: Subsystem ID
            {
                Return (\_SB.PSUB)
            }
        }

        Scope (\_SB.PILC)
        {
            Method (_SUB, 0, NotSerialized)  // _SUB: Subsystem ID
            {
                Return (\_SB.PSUB)
            }
        }

        Scope (\_SB.ADSP)
        {
            Method (_SUB, 0, NotSerialized)  // _SUB: Subsystem ID
            {
                If (((((\_SB.SKUV == 0x1F) || (\_SB.SKUV == 0x20)) || (\_SB.SKUV == 
                    0x21)) || (\_SB.SKUV == 0x22)))
                {
                    If ((\_SB.PLST != 0x09))
                    {
                        Return ("IWSA6490")
                    }
                    Else
                    {
                        Return ("IWCD6490")
                    }
                }
                Else
                {
                    Return (\_SB.PSUB)
                }
            }
        }

        Scope (\_SB.NSP0)
        {
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If ((\_SB.SJTG == 0x102150E1))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Scope (\_SB.CSW0)
        {
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If ((\_SB.SJTG == 0x102150E1))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Device (LLC)
        {
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.PEP0, 
            })
            Name (_HID, "QCOM0A83")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Alias (\_SB.SVMJ, _HRV)
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Return (Buffer (0x17)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0x60, 0x09,  // ......`.
                    /* 0008 */  0x00, 0x00, 0x05, 0x00, 0x89, 0x06, 0x00, 0x03,  // ........
                    /* 0010 */  0x01, 0x2A, 0x01, 0x00, 0x00, 0x79, 0x00         // .*...y.
                })
            }
        }

        Device (MMU0)
        {
            Name (_HID, "QCOM0A09")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Alias (\_SB.PSUB, _SUB)
            Alias (\_SB.SVMJ, _HRV)
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.PEP0, 
            })
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Return (Buffer (0x0257)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0x00, 0x15,  // ........
                    /* 0008 */  0x00, 0x00, 0x10, 0x00, 0x89, 0x06, 0x00, 0x03,  // ........
                    /* 0010 */  0x01, 0x80, 0x00, 0x00, 0x00, 0x89, 0x06, 0x00,  // ........
                    /* 0018 */  0x03, 0x01, 0x81, 0x00, 0x00, 0x00, 0x89, 0x06,  // ........
                    /* 0020 */  0x00, 0x03, 0x01, 0x82, 0x00, 0x00, 0x00, 0x89,  // ........
                    /* 0028 */  0x06, 0x00, 0x03, 0x01, 0x83, 0x00, 0x00, 0x00,  // ........
                    /* 0030 */  0x89, 0x06, 0x00, 0x03, 0x01, 0x84, 0x00, 0x00,  // ........
                    /* 0038 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x85, 0x00,  // ........
                    /* 0040 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x86,  // ........
                    /* 0048 */  0x00, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01,  // ........
                    /* 0050 */  0x87, 0x00, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03,  // ........
                    /* 0058 */  0x01, 0x88, 0x00, 0x00, 0x00, 0x89, 0x06, 0x00,  // ........
                    /* 0060 */  0x03, 0x01, 0x89, 0x00, 0x00, 0x00, 0x89, 0x06,  // ........
                    /* 0068 */  0x00, 0x03, 0x01, 0x8A, 0x00, 0x00, 0x00, 0x89,  // ........
                    /* 0070 */  0x06, 0x00, 0x03, 0x01, 0x8B, 0x00, 0x00, 0x00,  // ........
                    /* 0078 */  0x89, 0x06, 0x00, 0x03, 0x01, 0x8C, 0x00, 0x00,  // ........
                    /* 0080 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x8D, 0x00,  // ........
                    /* 0088 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x8E,  // ........
                    /* 0090 */  0x00, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01,  // ........
                    /* 0098 */  0x8F, 0x00, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03,  // ........
                    /* 00A0 */  0x01, 0x90, 0x00, 0x00, 0x00, 0x89, 0x06, 0x00,  // ........
                    /* 00A8 */  0x03, 0x01, 0x91, 0x00, 0x00, 0x00, 0x89, 0x06,  // ........
                    /* 00B0 */  0x00, 0x03, 0x01, 0x92, 0x00, 0x00, 0x00, 0x89,  // ........
                    /* 00B8 */  0x06, 0x00, 0x03, 0x01, 0x93, 0x00, 0x00, 0x00,  // ........
                    /* 00C0 */  0x89, 0x06, 0x00, 0x03, 0x01, 0x94, 0x00, 0x00,  // ........
                    /* 00C8 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x95, 0x00,  // ........
                    /* 00D0 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x96,  // ........
                    /* 00D8 */  0x00, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01,  // ........
                    /* 00E0 */  0xD5, 0x00, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03,  // ........
                    /* 00E8 */  0x01, 0xD6, 0x00, 0x00, 0x00, 0x89, 0x06, 0x00,  // ........
                    /* 00F0 */  0x03, 0x01, 0xD7, 0x00, 0x00, 0x00, 0x89, 0x06,  // ........
                    /* 00F8 */  0x00, 0x03, 0x01, 0xD8, 0x00, 0x00, 0x00, 0x89,  // ........
                    /* 0100 */  0x06, 0x00, 0x03, 0x01, 0xD9, 0x00, 0x00, 0x00,  // ........
                    /* 0108 */  0x89, 0x06, 0x00, 0x03, 0x01, 0xDA, 0x00, 0x00,  // ........
                    /* 0110 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0xDB, 0x00,  // ........
                    /* 0118 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0xDC,  // ........
                    /* 0120 */  0x00, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01,  // ........
                    /* 0128 */  0xDD, 0x00, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03,  // ........
                    /* 0130 */  0x01, 0xDE, 0x00, 0x00, 0x00, 0x89, 0x06, 0x00,  // ........
                    /* 0138 */  0x03, 0x01, 0xDF, 0x00, 0x00, 0x00, 0x89, 0x06,  // ........
                    /* 0140 */  0x00, 0x03, 0x01, 0xE0, 0x00, 0x00, 0x00, 0x89,  // ........
                    /* 0148 */  0x06, 0x00, 0x03, 0x01, 0x5B, 0x01, 0x00, 0x00,  // ....[...
                    /* 0150 */  0x89, 0x06, 0x00, 0x03, 0x01, 0x5C, 0x01, 0x00,  // .....\..
                    /* 0158 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x5D, 0x01,  // ......].
                    /* 0160 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x5E,  // .......^
                    /* 0168 */  0x01, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01,  // ........
                    /* 0170 */  0x5F, 0x01, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03,  // _.......
                    /* 0178 */  0x01, 0x60, 0x01, 0x00, 0x00, 0x89, 0x06, 0x00,  // .`......
                    /* 0180 */  0x03, 0x01, 0x61, 0x01, 0x00, 0x00, 0x89, 0x06,  // ..a.....
                    /* 0188 */  0x00, 0x03, 0x01, 0x62, 0x01, 0x00, 0x00, 0x89,  // ...b....
                    /* 0190 */  0x06, 0x00, 0x03, 0x01, 0x63, 0x01, 0x00, 0x00,  // ....c...
                    /* 0198 */  0x89, 0x06, 0x00, 0x03, 0x01, 0x64, 0x01, 0x00,  // .....d..
                    /* 01A0 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x65, 0x01,  // ......e.
                    /* 01A8 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x66,  // .......f
                    /* 01B0 */  0x01, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01,  // ........
                    /* 01B8 */  0x67, 0x01, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03,  // g.......
                    /* 01C0 */  0x01, 0x68, 0x01, 0x00, 0x00, 0x89, 0x06, 0x00,  // .h......
                    /* 01C8 */  0x03, 0x01, 0x69, 0x01, 0x00, 0x00, 0x89, 0x06,  // ..i.....
                    /* 01D0 */  0x00, 0x03, 0x01, 0x6A, 0x01, 0x00, 0x00, 0x89,  // ...j....
                    /* 01D8 */  0x06, 0x00, 0x03, 0x01, 0x6B, 0x01, 0x00, 0x00,  // ....k...
                    /* 01E0 */  0x89, 0x06, 0x00, 0x03, 0x01, 0x6C, 0x01, 0x00,  // .....l..
                    /* 01E8 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x6D, 0x01,  // ......m.
                    /* 01F0 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x6E,  // .......n
                    /* 01F8 */  0x01, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01,  // ........
                    /* 0200 */  0x6F, 0x01, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03,  // o.......
                    /* 0208 */  0x01, 0x70, 0x01, 0x00, 0x00, 0x89, 0x06, 0x00,  // .p......
                    /* 0210 */  0x03, 0x01, 0x71, 0x01, 0x00, 0x00, 0x89, 0x06,  // ..q.....
                    /* 0218 */  0x00, 0x03, 0x01, 0x72, 0x01, 0x00, 0x00, 0x89,  // ...r....
                    /* 0220 */  0x06, 0x00, 0x03, 0x01, 0x73, 0x01, 0x00, 0x00,  // ....s...
                    /* 0228 */  0x89, 0x06, 0x00, 0x03, 0x01, 0x74, 0x01, 0x00,  // .....t..
                    /* 0230 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x75, 0x01,  // ......u.
                    /* 0238 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x76,  // .......v
                    /* 0240 */  0x01, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01,  // ........
                    /* 0248 */  0x77, 0x01, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03,  // w.......
                    /* 0250 */  0x01, 0x78, 0x01, 0x00, 0x00, 0x79, 0x00         // .x...y.
                })
            }
        }

        Device (MMU1)
        {
            Name (_HID, "QCOM0A09")  // _HID: Hardware ID
            Name (_UID, One)  // _UID: Unique ID
            Alias (\_SB.PSUB, _SUB)
            Alias (\_SB.SVMJ, _HRV)
            Name (_DEP, Package (0x01)  // _DEP: Dependencies
            {
                \_SB.PEP0, 
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }

            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Return (Buffer (0x68)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0xDA, 0x03,  // ........
                    /* 0008 */  0x00, 0x00, 0x02, 0x00, 0x89, 0x06, 0x00, 0x03,  // ........
                    /* 0010 */  0x01, 0xC6, 0x02, 0x00, 0x00, 0x89, 0x06, 0x00,  // ........
                    /* 0018 */  0x03, 0x01, 0xC7, 0x02, 0x00, 0x00, 0x89, 0x06,  // ........
                    /* 0020 */  0x00, 0x03, 0x01, 0xC8, 0x02, 0x00, 0x00, 0x89,  // ........
                    /* 0028 */  0x06, 0x00, 0x03, 0x01, 0xC9, 0x02, 0x00, 0x00,  // ........
                    /* 0030 */  0x89, 0x06, 0x00, 0x03, 0x01, 0xCA, 0x02, 0x00,  // ........
                    /* 0038 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0xCB, 0x02,  // ........
                    /* 0040 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0xCC,  // ........
                    /* 0048 */  0x02, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01,  // ........
                    /* 0050 */  0xCD, 0x02, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03,  // ........
                    /* 0058 */  0x01, 0xCE, 0x02, 0x00, 0x00, 0x89, 0x06, 0x00,  // ........
                    /* 0060 */  0x03, 0x01, 0xCF, 0x02, 0x00, 0x00, 0x79, 0x00   // ......y.
                })
            }
        }

        Device (IMM0)
        {
            Name (_HID, "QCOM068F")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, Zero)  // _UID: Unique ID
        }

        Device (IMM1)
        {
            Name (_HID, "QCOM068F")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, One)  // _UID: Unique ID
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }
        }

        Device (GPU0)
        {
            Name (_HID, "QCOM0A36")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_CLS, Package (0x03) { 0x03, 0x00, 0x00 })  // _CLS: Class Code (Display Controller)
            Device (MON0)
            {
                Method (_ADR, 0, NotSerialized)  // _ADR: Address
                {
                    Return (Zero)
                }
            }

            Name (_DEP, Package (0x0A)  // _DEP: Dependencies
            {
                \_SB.MMU0, , 
                \_SB.MMU1, , 
                \_SB.IMM0, , 
                \_SB.IMM1, , 
                \_SB.PEP0, , 
                \_SB.PMIC, , 
                \_SB.PILC, , 
                \_SB.RPEN, , 
                \_SB.TREE, , 
                \_SB.SCM0, 
            })
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (ABUF, Buffer (0x0121)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0xE0, 0x0A,  // ........
                    /* 0008 */  0x00, 0x00, 0x20, 0x00, 0x86, 0x09, 0x00, 0x01,  // .. .....
                    /* 0010 */  0x00, 0x00, 0x8E, 0x08, 0x00, 0x00, 0x01, 0x00,  // ........
                    /* 0018 */  0x89, 0x06, 0x00, 0x01, 0x01, 0x73, 0x00, 0x00,  // .....s..
                    /* 0020 */  0x00, 0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0xD0,  // ........
                    /* 0028 */  0x03, 0x10, 0xF0, 0x03, 0x00, 0x86, 0x09, 0x00,  // ........
                    /* 0030 */  0x01, 0x00, 0x00, 0xD6, 0x03, 0x00, 0xF0, 0x03,  // ........
                    /* 0038 */  0x00, 0x89, 0x06, 0x00, 0x01, 0x01, 0x4C, 0x01,  // ......L.
                    /* 0040 */  0x00, 0x00, 0x86, 0x09, 0x00, 0x01, 0x00, 0x00,  // ........
                    /* 0048 */  0x29, 0x0B, 0x00, 0x00, 0x01, 0x00, 0x86, 0x09,  // ).......
                    /* 0050 */  0x00, 0x01, 0x00, 0x00, 0x49, 0x0B, 0x00, 0x00,  // ....I...
                    /* 0058 */  0x01, 0x00, 0x86, 0x09, 0x00, 0x01, 0x00, 0x00,  // ........
                    /* 0060 */  0xD9, 0x03, 0x00, 0x90, 0x00, 0x00, 0x86, 0x09,  // ........
                    /* 0068 */  0x00, 0x01, 0x00, 0x00, 0xDE, 0x03, 0x00, 0x00,  // ........
                    /* 0070 */  0x01, 0x00, 0x86, 0x09, 0x00, 0x01, 0x00, 0x00,  // ........
                    /* 0078 */  0x20, 0x0C, 0xFF, 0xFF, 0x00, 0x00, 0x86, 0x09,  //  .......
                    /* 0080 */  0x00, 0x01, 0x00, 0x00, 0xA0, 0x0A, 0x00, 0x00,  // ........
                    /* 0088 */  0x20, 0x00, 0x89, 0x06, 0x00, 0x01, 0x01, 0xCE,  //  .......
                    /* 0090 */  0x00, 0x00, 0x00, 0x8C, 0x20, 0x00, 0x01, 0x01,  // .... ...
                    /* 0098 */  0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,  // ........
                    /* 00A0 */  0x00, 0x17, 0x00, 0x00, 0x19, 0x00, 0x23, 0x00,  // ......#.
                    /* 00A8 */  0x00, 0x00, 0x2C, 0x00, 0x5C, 0x5F, 0x53, 0x42,  // ..,.\_SB
                    /* 00B0 */  0x2E, 0x47, 0x49, 0x4F, 0x30, 0x00, 0x8C, 0x20,  // .GIO0.. 
                    /* 00B8 */  0x00, 0x01, 0x01, 0x01, 0x00, 0x02, 0x00, 0x03,  // ........
                    /* 00C0 */  0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x19,  // ........
                    /* 00C8 */  0x00, 0x23, 0x00, 0x00, 0x00, 0x09, 0x00, 0x5C,  // .#.....\
                    /* 00D0 */  0x5F, 0x53, 0x42, 0x2E, 0x47, 0x49, 0x4F, 0x30,  // _SB.GIO0
                    /* 00D8 */  0x00, 0x8C, 0x20, 0x00, 0x01, 0x01, 0x01, 0x00,  // .. .....
                    /* 00E0 */  0x02, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x17,  // ........
                    /* 00E8 */  0x00, 0x00, 0x19, 0x00, 0x23, 0x00, 0x00, 0x00,  // ....#...
                    /* 00F0 */  0x0A, 0x00, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x47,  // ..\_SB.G
                    /* 00F8 */  0x49, 0x4F, 0x30, 0x00, 0x8C, 0x20, 0x00, 0x01,  // IO0.. ..
                    /* 0100 */  0x01, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x00,  // ........
                    /* 0108 */  0x00, 0x00, 0x17, 0x00, 0x00, 0x19, 0x00, 0x23,  // .......#
                    /* 0110 */  0x00, 0x00, 0x00, 0x87, 0x00, 0x5C, 0x5F, 0x53,  // .....\_S
                    /* 0118 */  0x42, 0x2E, 0x47, 0x49, 0x4F, 0x30, 0x00, 0x79,  // B.GIO0.y
                    /* 0120 */  0x00                                             // .
                })
                Return (ABUF) /* \_SB_.GPU0._CRS.ABUF */
            }

            Method (RESI, 0, NotSerialized)
            {
                Name (AINF, Package (0x0F)
                {
                    0x03, 
                    Zero, 
                    Package (0x03)
                    {
                        "RESOURCE", 
                        "MDP_REGS", 
                        "DISPLAY"
                    }, 

                    Package (0x03)
                    {
                        "RESOURCE", 
                        "DP_PHY_REGS", 
                        "DISPLAY"
                    }, 

                    Package (0x03)
                    {
                        "RESOURCE", 
                        "VSYNC_INTERRUPT", 
                        "DISPLAY"
                    }, 

                    Package (0x03)
                    {
                        "RESOURCE", 
                        "GFX_REGS", 
                        "GRAPHICS"
                    }, 

                    Package (0x03)
                    {
                        "RESOURCE", 
                        "GFX_REG_CONT", 
                        "GRAPHICS"
                    }, 

                    Package (0x03)
                    {
                        "RESOURCE", 
                        "GFX_INTERRUPT", 
                        "GRAPHICS"
                    }, 

                    Package (0x03)
                    {
                        "RESOURCE", 
                        "GPU_PDC_SEQ_MEM", 
                        "GRAPHICS"
                    }, 

                    Package (0x03)
                    {
                        "RESOURCE", 
                        "GPU_PDC_REGS", 
                        "GRAPHICS"
                    }, 

                    Package (0x03)
                    {
                        "RESOURCE", 
                        "GPU_CC", 
                        "GRAPHICS"
                    }, 

                    Package (0x03)
                    {
                        "RESOURCE", 
                        "GPU_RSCC", 
                        "GRAPHICS"
                    }, 

                    Package (0x03)
                    {
                        "RESOURCE", 
                        "GPU_RPMH_CPRF", 
                        "GRAPHICS"
                    }, 

                    Package (0x03)
                    {
                        "RESOURCE", 
                        "VIDEO_REGS", 
                        "VIDEO"
                    }, 

                    Package (0x03)
                    {
                        "RESOURCE", 
                        "VIDC_INTERRUPT", 
                        "VIDEO"
                    }
                })
                Return (AINF) /* \_SB_.GPU0.RESI.AINF */
            }

            Method (_ROM, 2, NotSerialized)  // _ROM: Read-Only Memory
            {
                Name (PCFG, Buffer (0x0D54)
                {
                    /* 0000 */  0x3C, 0x3F, 0x78, 0x6D, 0x6C, 0x20, 0x76, 0x65,  // <?xml ve
                    /* 0008 */  0x72, 0x73, 0x69, 0x6F, 0x6E, 0x3D, 0x27, 0x31,  // rsion='1
                    /* 0010 */  0x2E, 0x30, 0x27, 0x20, 0x65, 0x6E, 0x63, 0x6F,  // .0' enco
                    /* 0018 */  0x64, 0x69, 0x6E, 0x67, 0x3D, 0x27, 0x75, 0x74,  // ding='ut
                    /* 0020 */  0x66, 0x2D, 0x38, 0x27, 0x3F, 0x3E, 0x0A, 0x3C,  // f-8'?>.<
                    /* 0028 */  0x50, 0x61, 0x6E, 0x65, 0x6C, 0x44, 0x65, 0x73,  // PanelDes
                    /* 0030 */  0x63, 0x72, 0x69, 0x70, 0x74, 0x69, 0x6F, 0x6E,  // cription
                    /* 0038 */  0x3E, 0x55, 0x53, 0x42, 0x2D, 0x43, 0x20, 0x44,  // >USB-C D
                    /* 0040 */  0x69, 0x73, 0x70, 0x6C, 0x61, 0x79, 0x50, 0x6F,  // isplayPo
                    /* 0048 */  0x72, 0x74, 0x20, 0x41, 0x6C, 0x74, 0x20, 0x4D,  // rt Alt M
                    /* 0050 */  0x6F, 0x64, 0x65, 0x20, 0x28, 0x31, 0x39, 0x32,  // ode (192
                    /* 0058 */  0x30, 0x78, 0x31, 0x30, 0x38, 0x30, 0x20, 0x32,  // 0x1080 2
                    /* 0060 */  0x34, 0x62, 0x70, 0x70, 0x29, 0x3C, 0x2F, 0x50,  // 4bpp)</P
                    /* 0068 */  0x61, 0x6E, 0x65, 0x6C, 0x44, 0x65, 0x73, 0x63,  // anelDesc
                    /* 0070 */  0x72, 0x69, 0x70, 0x74, 0x69, 0x6F, 0x6E, 0x3E,  // ription>
                    /* 0078 */  0x0A, 0x3C, 0x47, 0x72, 0x6F, 0x75, 0x70, 0x20,  // .<Group 
                    /* 0080 */  0x69, 0x64, 0x3D, 0x27, 0x45, 0x44, 0x49, 0x44,  // id='EDID
                    /* 0088 */  0x20, 0x43, 0x6F, 0x6E, 0x66, 0x69, 0x67, 0x75,  //  Configu
                    /* 0090 */  0x72, 0x61, 0x74, 0x69, 0x6F, 0x6E, 0x27, 0x3E,  // ration'>
                    /* 0098 */  0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x4D, 0x61,  // .    <Ma
                    /* 00A0 */  0x6E, 0x75, 0x66, 0x61, 0x63, 0x74, 0x75, 0x72,  // nufactur
                    /* 00A8 */  0x65, 0x49, 0x44, 0x3E, 0x30, 0x78, 0x45, 0x34,  // eID>0xE4
                    /* 00B0 */  0x33, 0x30, 0x3C, 0x2F, 0x4D, 0x61, 0x6E, 0x75,  // 30</Manu
                    /* 00B8 */  0x66, 0x61, 0x63, 0x74, 0x75, 0x72, 0x65, 0x49,  // factureI
                    /* 00C0 */  0x44, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C,  // D>.    <
                    /* 00C8 */  0x50, 0x72, 0x6F, 0x64, 0x75, 0x63, 0x74, 0x43,  // ProductC
                    /* 00D0 */  0x6F, 0x64, 0x65, 0x3E, 0x30, 0x78, 0x41, 0x39,  // ode>0xA9
                    /* 00D8 */  0x39, 0x30, 0x3C, 0x2F, 0x50, 0x72, 0x6F, 0x64,  // 90</Prod
                    /* 00E0 */  0x75, 0x63, 0x74, 0x43, 0x6F, 0x64, 0x65, 0x3E,  // uctCode>
                    /* 00E8 */  0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x53, 0x65,  // .    <Se
                    /* 00F0 */  0x72, 0x69, 0x61, 0x6C, 0x4E, 0x75, 0x6D, 0x62,  // rialNumb
                    /* 00F8 */  0x65, 0x72, 0x3E, 0x30, 0x78, 0x30, 0x30, 0x30,  // er>0x000
                    /* 0100 */  0x30, 0x30, 0x31, 0x3C, 0x2F, 0x53, 0x65, 0x72,  // 001</Ser
                    /* 0108 */  0x69, 0x61, 0x6C, 0x4E, 0x75, 0x6D, 0x62, 0x65,  // ialNumbe
                    /* 0110 */  0x72, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C,  // r>.    <
                    /* 0118 */  0x57, 0x65, 0x65, 0x6B, 0x6F, 0x66, 0x4D, 0x61,  // WeekofMa
                    /* 0120 */  0x6E, 0x75, 0x66, 0x61, 0x63, 0x74, 0x75, 0x72,  // nufactur
                    /* 0128 */  0x65, 0x3E, 0x30, 0x78, 0x30, 0x31, 0x3C, 0x2F,  // e>0x01</
                    /* 0130 */  0x57, 0x65, 0x65, 0x6B, 0x6F, 0x66, 0x4D, 0x61,  // WeekofMa
                    /* 0138 */  0x6E, 0x75, 0x66, 0x61, 0x63, 0x74, 0x75, 0x72,  // nufactur
                    /* 0140 */  0x65, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C,  // e>.    <
                    /* 0148 */  0x59, 0x65, 0x61, 0x72, 0x6F, 0x66, 0x4D, 0x61,  // YearofMa
                    /* 0150 */  0x6E, 0x75, 0x66, 0x61, 0x63, 0x74, 0x75, 0x72,  // nufactur
                    /* 0158 */  0x65, 0x3E, 0x30, 0x78, 0x31, 0x45, 0x3C, 0x2F,  // e>0x1E</
                    /* 0160 */  0x59, 0x65, 0x61, 0x72, 0x6F, 0x66, 0x4D, 0x61,  // YearofMa
                    /* 0168 */  0x6E, 0x75, 0x66, 0x61, 0x63, 0x74, 0x75, 0x72,  // nufactur
                    /* 0170 */  0x65, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C,  // e>.    <
                    /* 0178 */  0x45, 0x44, 0x49, 0x44, 0x56, 0x65, 0x72, 0x73,  // EDIDVers
                    /* 0180 */  0x69, 0x6F, 0x6E, 0x3E, 0x31, 0x3C, 0x2F, 0x45,  // ion>1</E
                    /* 0188 */  0x44, 0x49, 0x44, 0x56, 0x65, 0x72, 0x73, 0x69,  // DIDVersi
                    /* 0190 */  0x6F, 0x6E, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20,  // on>.    
                    /* 0198 */  0x3C, 0x45, 0x44, 0x49, 0x44, 0x52, 0x65, 0x76,  // <EDIDRev
                    /* 01A0 */  0x69, 0x73, 0x69, 0x6F, 0x6E, 0x3E, 0x34, 0x3C,  // ision>4<
                    /* 01A8 */  0x2F, 0x45, 0x44, 0x49, 0x44, 0x52, 0x65, 0x76,  // /EDIDRev
                    /* 01B0 */  0x69, 0x73, 0x69, 0x6F, 0x6E, 0x3E, 0x0A, 0x20,  // ision>. 
                    /* 01B8 */  0x20, 0x20, 0x20, 0x3C, 0x48, 0x6F, 0x72, 0x69,  //    <Hori
                    /* 01C0 */  0x7A, 0x6F, 0x6E, 0x74, 0x61, 0x6C, 0x53, 0x63,  // zontalSc
                    /* 01C8 */  0x72, 0x65, 0x65, 0x6E, 0x53, 0x69, 0x7A, 0x65,  // reenSize
                    /* 01D0 */  0x3E, 0x30, 0x78, 0x30, 0x42, 0x3C, 0x2F, 0x48,  // >0x0B</H
                    /* 01D8 */  0x6F, 0x72, 0x69, 0x7A, 0x6F, 0x6E, 0x74, 0x61,  // orizonta
                    /* 01E0 */  0x6C, 0x53, 0x63, 0x72, 0x65, 0x65, 0x6E, 0x53,  // lScreenS
                    /* 01E8 */  0x69, 0x7A, 0x65, 0x3E, 0x0A, 0x20, 0x20, 0x20,  // ize>.   
                    /* 01F0 */  0x20, 0x3C, 0x56, 0x65, 0x72, 0x74, 0x69, 0x63,  //  <Vertic
                    /* 01F8 */  0x61, 0x6C, 0x53, 0x63, 0x72, 0x65, 0x65, 0x6E,  // alScreen
                    /* 0200 */  0x53, 0x69, 0x7A, 0x65, 0x3E, 0x30, 0x78, 0x30,  // Size>0x0
                    /* 0208 */  0x38, 0x3C, 0x2F, 0x56, 0x65, 0x72, 0x74, 0x69,  // 8</Verti
                    /* 0210 */  0x63, 0x61, 0x6C, 0x53, 0x63, 0x72, 0x65, 0x65,  // calScree
                    /* 0218 */  0x6E, 0x53, 0x69, 0x7A, 0x65, 0x3E, 0x0A, 0x20,  // nSize>. 
                    /* 0220 */  0x20, 0x20, 0x20, 0x3C, 0x44, 0x69, 0x73, 0x70,  //    <Disp
                    /* 0228 */  0x6C, 0x61, 0x79, 0x54, 0x72, 0x61, 0x6E, 0x73,  // layTrans
                    /* 0230 */  0x66, 0x65, 0x72, 0x43, 0x68, 0x61, 0x72, 0x61,  // ferChara
                    /* 0238 */  0x63, 0x74, 0x65, 0x72, 0x69, 0x73, 0x74, 0x69,  // cteristi
                    /* 0240 */  0x63, 0x73, 0x3E, 0x30, 0x78, 0x37, 0x38, 0x3C,  // cs>0x78<
                    /* 0248 */  0x2F, 0x44, 0x69, 0x73, 0x70, 0x6C, 0x61, 0x79,  // /Display
                    /* 0250 */  0x54, 0x72, 0x61, 0x6E, 0x73, 0x66, 0x65, 0x72,  // Transfer
                    /* 0258 */  0x43, 0x68, 0x61, 0x72, 0x61, 0x63, 0x74, 0x65,  // Characte
                    /* 0260 */  0x72, 0x69, 0x73, 0x74, 0x69, 0x63, 0x73, 0x3E,  // ristics>
                    /* 0268 */  0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x46, 0x65,  // .    <Fe
                    /* 0270 */  0x61, 0x74, 0x75, 0x72, 0x65, 0x53, 0x75, 0x70,  // atureSup
                    /* 0278 */  0x70, 0x6F, 0x72, 0x74, 0x3E, 0x30, 0x78, 0x32,  // port>0x2
                    /* 0280 */  0x3C, 0x2F, 0x46, 0x65, 0x61, 0x74, 0x75, 0x72,  // </Featur
                    /* 0288 */  0x65, 0x53, 0x75, 0x70, 0x70, 0x6F, 0x72, 0x74,  // eSupport
                    /* 0290 */  0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x52,  // >.    <R
                    /* 0298 */  0x65, 0x64, 0x2E, 0x47, 0x72, 0x65, 0x65, 0x6E,  // ed.Green
                    /* 02A0 */  0x42, 0x69, 0x74, 0x73, 0x3E, 0x30, 0x78, 0x41,  // Bits>0xA
                    /* 02A8 */  0x34, 0x3C, 0x2F, 0x52, 0x65, 0x64, 0x2E, 0x47,  // 4</Red.G
                    /* 02B0 */  0x72, 0x65, 0x65, 0x6E, 0x42, 0x69, 0x74, 0x73,  // reenBits
                    /* 02B8 */  0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x42,  // >.    <B
                    /* 02C0 */  0x6C, 0x75, 0x65, 0x2E, 0x57, 0x68, 0x69, 0x74,  // lue.Whit
                    /* 02C8 */  0x65, 0x42, 0x69, 0x74, 0x73, 0x3E, 0x30, 0x78,  // eBits>0x
                    /* 02D0 */  0x31, 0x35, 0x3C, 0x2F, 0x42, 0x6C, 0x75, 0x65,  // 15</Blue
                    /* 02D8 */  0x2E, 0x57, 0x68, 0x69, 0x74, 0x65, 0x42, 0x69,  // .WhiteBi
                    /* 02E0 */  0x74, 0x73, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20,  // ts>.    
                    /* 02E8 */  0x3C, 0x52, 0x65, 0x64, 0x58, 0x3E, 0x30, 0x78,  // <RedX>0x
                    /* 02F0 */  0x39, 0x45, 0x3C, 0x2F, 0x52, 0x65, 0x64, 0x58,  // 9E</RedX
                    /* 02F8 */  0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x52,  // >.    <R
                    /* 0300 */  0x65, 0x64, 0x59, 0x3E, 0x30, 0x78, 0x35, 0x35,  // edY>0x55
                    /* 0308 */  0x3C, 0x2F, 0x52, 0x65, 0x64, 0x59, 0x3E, 0x0A,  // </RedY>.
                    /* 0310 */  0x20, 0x20, 0x20, 0x20, 0x3C, 0x47, 0x72, 0x65,  //     <Gre
                    /* 0318 */  0x65, 0x6E, 0x58, 0x3E, 0x30, 0x78, 0x34, 0x45,  // enX>0x4E
                    /* 0320 */  0x3C, 0x2F, 0x47, 0x72, 0x65, 0x65, 0x6E, 0x58,  // </GreenX
                    /* 0328 */  0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x47,  // >.    <G
                    /* 0330 */  0x72, 0x65, 0x65, 0x6E, 0x59, 0x3E, 0x30, 0x78,  // reenY>0x
                    /* 0338 */  0x39, 0x42, 0x3C, 0x2F, 0x47, 0x72, 0x65, 0x65,  // 9B</Gree
                    /* 0340 */  0x6E, 0x59, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20,  // nY>.    
                    /* 0348 */  0x3C, 0x42, 0x6C, 0x75, 0x65, 0x58, 0x3E, 0x30,  // <BlueX>0
                    /* 0350 */  0x78, 0x32, 0x36, 0x3C, 0x2F, 0x42, 0x6C, 0x75,  // x26</Blu
                    /* 0358 */  0x65, 0x58, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20,  // eX>.    
                    /* 0360 */  0x3C, 0x42, 0x6C, 0x75, 0x65, 0x59, 0x3E, 0x30,  // <BlueY>0
                    /* 0368 */  0x78, 0x30, 0x46, 0x3C, 0x2F, 0x42, 0x6C, 0x75,  // x0F</Blu
                    /* 0370 */  0x65, 0x59, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20,  // eY>.    
                    /* 0378 */  0x3C, 0x57, 0x68, 0x69, 0x74, 0x65, 0x58, 0x3E,  // <WhiteX>
                    /* 0380 */  0x30, 0x78, 0x35, 0x30, 0x3C, 0x2F, 0x57, 0x68,  // 0x50</Wh
                    /* 0388 */  0x69, 0x74, 0x65, 0x58, 0x3E, 0x0A, 0x20, 0x20,  // iteX>.  
                    /* 0390 */  0x20, 0x20, 0x3C, 0x57, 0x68, 0x69, 0x74, 0x65,  //   <White
                    /* 0398 */  0x59, 0x3E, 0x30, 0x78, 0x35, 0x34, 0x3C, 0x2F,  // Y>0x54</
                    /* 03A0 */  0x57, 0x68, 0x69, 0x74, 0x65, 0x59, 0x3E, 0x0A,  // WhiteY>.
                    /* 03A8 */  0x20, 0x20, 0x20, 0x20, 0x3C, 0x45, 0x73, 0x74,  //     <Est
                    /* 03B0 */  0x61, 0x62, 0x6C, 0x69, 0x73, 0x68, 0x65, 0x64,  // ablished
                    /* 03B8 */  0x54, 0x69, 0x6D, 0x69, 0x6E, 0x67, 0x73, 0x49,  // TimingsI
                    /* 03C0 */  0x3E, 0x30, 0x78, 0x30, 0x3C, 0x2F, 0x45, 0x73,  // >0x0</Es
                    /* 03C8 */  0x74, 0x61, 0x62, 0x6C, 0x69, 0x73, 0x68, 0x65,  // tablishe
                    /* 03D0 */  0x64, 0x54, 0x69, 0x6D, 0x69, 0x6E, 0x67, 0x73,  // dTimings
                    /* 03D8 */  0x49, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C,  // I>.    <
                    /* 03E0 */  0x45, 0x73, 0x74, 0x61, 0x62, 0x6C, 0x69, 0x73,  // Establis
                    /* 03E8 */  0x68, 0x65, 0x64, 0x54, 0x69, 0x6D, 0x69, 0x6E,  // hedTimin
                    /* 03F0 */  0x67, 0x73, 0x49, 0x49, 0x3E, 0x30, 0x78, 0x30,  // gsII>0x0
                    /* 03F8 */  0x3C, 0x2F, 0x45, 0x73, 0x74, 0x61, 0x62, 0x6C,  // </Establ
                    /* 0400 */  0x69, 0x73, 0x68, 0x65, 0x64, 0x54, 0x69, 0x6D,  // ishedTim
                    /* 0408 */  0x69, 0x6E, 0x67, 0x73, 0x49, 0x49, 0x3E, 0x0A,  // ingsII>.
                    /* 0410 */  0x20, 0x20, 0x20, 0x20, 0x3C, 0x4D, 0x61, 0x6E,  //     <Man
                    /* 0418 */  0x75, 0x66, 0x61, 0x63, 0x74, 0x75, 0x72, 0x65,  // ufacture
                    /* 0420 */  0x73, 0x54, 0x69, 0x6D, 0x69, 0x6E, 0x67, 0x3E,  // sTiming>
                    /* 0428 */  0x30, 0x78, 0x30, 0x3C, 0x2F, 0x4D, 0x61, 0x6E,  // 0x0</Man
                    /* 0430 */  0x75, 0x66, 0x61, 0x63, 0x74, 0x75, 0x72, 0x65,  // ufacture
                    /* 0438 */  0x73, 0x54, 0x69, 0x6D, 0x69, 0x6E, 0x67, 0x3E,  // sTiming>
                    /* 0440 */  0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x53, 0x74,  // .    <St
                    /* 0448 */  0x61, 0x6E, 0x64, 0x61, 0x72, 0x64, 0x54, 0x69,  // andardTi
                    /* 0450 */  0x6D, 0x69, 0x6E, 0x67, 0x73, 0x31, 0x20, 0x2F,  // mings1 /
                    /* 0458 */  0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x53,  // >.    <S
                    /* 0460 */  0x74, 0x61, 0x6E, 0x64, 0x61, 0x72, 0x64, 0x54,  // tandardT
                    /* 0468 */  0x69, 0x6D, 0x69, 0x6E, 0x67, 0x73, 0x32, 0x20,  // imings2 
                    /* 0470 */  0x2F, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C,  // />.    <
                    /* 0478 */  0x53, 0x74, 0x61, 0x6E, 0x64, 0x61, 0x72, 0x64,  // Standard
                    /* 0480 */  0x54, 0x69, 0x6D, 0x69, 0x6E, 0x67, 0x73, 0x33,  // Timings3
                    /* 0488 */  0x20, 0x2F, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20,  //  />.    
                    /* 0490 */  0x3C, 0x53, 0x74, 0x61, 0x6E, 0x64, 0x61, 0x72,  // <Standar
                    /* 0498 */  0x64, 0x54, 0x69, 0x6D, 0x69, 0x6E, 0x67, 0x73,  // dTimings
                    /* 04A0 */  0x34, 0x20, 0x2F, 0x3E, 0x0A, 0x20, 0x20, 0x20,  // 4 />.   
                    /* 04A8 */  0x20, 0x3C, 0x53, 0x74, 0x61, 0x6E, 0x64, 0x61,  //  <Standa
                    /* 04B0 */  0x72, 0x64, 0x54, 0x69, 0x6D, 0x69, 0x6E, 0x67,  // rdTiming
                    /* 04B8 */  0x73, 0x35, 0x20, 0x2F, 0x3E, 0x0A, 0x20, 0x20,  // s5 />.  
                    /* 04C0 */  0x20, 0x20, 0x3C, 0x53, 0x74, 0x61, 0x6E, 0x64,  //   <Stand
                    /* 04C8 */  0x61, 0x72, 0x64, 0x54, 0x69, 0x6D, 0x69, 0x6E,  // ardTimin
                    /* 04D0 */  0x67, 0x73, 0x36, 0x20, 0x2F, 0x3E, 0x0A, 0x20,  // gs6 />. 
                    /* 04D8 */  0x20, 0x20, 0x20, 0x3C, 0x53, 0x74, 0x61, 0x6E,  //    <Stan
                    /* 04E0 */  0x64, 0x61, 0x72, 0x64, 0x54, 0x69, 0x6D, 0x69,  // dardTimi
                    /* 04E8 */  0x6E, 0x67, 0x73, 0x37, 0x20, 0x2F, 0x3E, 0x0A,  // ngs7 />.
                    /* 04F0 */  0x20, 0x20, 0x20, 0x20, 0x3C, 0x53, 0x69, 0x67,  //     <Sig
                    /* 04F8 */  0x6E, 0x61, 0x6C, 0x54, 0x69, 0x6D, 0x69, 0x6E,  // nalTimin
                    /* 0500 */  0x67, 0x49, 0x6E, 0x74, 0x65, 0x72, 0x66, 0x61,  // gInterfa
                    /* 0508 */  0x63, 0x65, 0x20, 0x2F, 0x3E, 0x0A, 0x3C, 0x2F,  // ce />.</
                    /* 0510 */  0x47, 0x72, 0x6F, 0x75, 0x70, 0x3E, 0x0A, 0x3C,  // Group>.<
                    /* 0518 */  0x47, 0x72, 0x6F, 0x75, 0x70, 0x20, 0x69, 0x64,  // Group id
                    /* 0520 */  0x3D, 0x27, 0x44, 0x65, 0x74, 0x61, 0x69, 0x6C,  // ='Detail
                    /* 0528 */  0x65, 0x64, 0x20, 0x54, 0x69, 0x6D, 0x69, 0x6E,  // ed Timin
                    /* 0530 */  0x67, 0x27, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20,  // g'>.    
                    /* 0538 */  0x3C, 0x48, 0x6F, 0x72, 0x69, 0x7A, 0x6F, 0x6E,  // <Horizon
                    /* 0540 */  0x74, 0x61, 0x6C, 0x53, 0x63, 0x72, 0x65, 0x65,  // talScree
                    /* 0548 */  0x6E, 0x53, 0x69, 0x7A, 0x65, 0x4D, 0x4D, 0x3E,  // nSizeMM>
                    /* 0550 */  0x30, 0x78, 0x37, 0x32, 0x3C, 0x2F, 0x48, 0x6F,  // 0x72</Ho
                    /* 0558 */  0x72, 0x69, 0x7A, 0x6F, 0x6E, 0x74, 0x61, 0x6C,  // rizontal
                    /* 0560 */  0x53, 0x63, 0x72, 0x65, 0x65, 0x6E, 0x53, 0x69,  // ScreenSi
                    /* 0568 */  0x7A, 0x65, 0x4D, 0x4D, 0x3E, 0x0A, 0x20, 0x20,  // zeMM>.  
                    /* 0570 */  0x20, 0x20, 0x3C, 0x56, 0x65, 0x72, 0x74, 0x69,  //   <Verti
                    /* 0578 */  0x63, 0x61, 0x6C, 0x53, 0x63, 0x72, 0x65, 0x65,  // calScree
                    /* 0580 */  0x6E, 0x53, 0x69, 0x7A, 0x65, 0x4D, 0x4D, 0x3E,  // nSizeMM>
                    /* 0588 */  0x30, 0x78, 0x35, 0x35, 0x3C, 0x2F, 0x56, 0x65,  // 0x55</Ve
                    /* 0590 */  0x72, 0x74, 0x69, 0x63, 0x61, 0x6C, 0x53, 0x63,  // rticalSc
                    /* 0598 */  0x72, 0x65, 0x65, 0x6E, 0x53, 0x69, 0x7A, 0x65,  // reenSize
                    /* 05A0 */  0x4D, 0x4D, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20,  // MM>.    
                    /* 05A8 */  0x3C, 0x48, 0x6F, 0x72, 0x69, 0x7A, 0x6F, 0x6E,  // <Horizon
                    /* 05B0 */  0x74, 0x61, 0x6C, 0x56, 0x65, 0x72, 0x74, 0x69,  // talVerti
                    /* 05B8 */  0x63, 0x61, 0x6C, 0x53, 0x63, 0x72, 0x65, 0x65,  // calScree
                    /* 05C0 */  0x6E, 0x53, 0x69, 0x7A, 0x65, 0x4D, 0x4D, 0x3E,  // nSizeMM>
                    /* 05C8 */  0x30, 0x78, 0x30, 0x30, 0x3C, 0x2F, 0x48, 0x6F,  // 0x00</Ho
                    /* 05D0 */  0x72, 0x69, 0x7A, 0x6F, 0x6E, 0x74, 0x61, 0x6C,  // rizontal
                    /* 05D8 */  0x56, 0x65, 0x72, 0x74, 0x69, 0x63, 0x61, 0x6C,  // Vertical
                    /* 05E0 */  0x53, 0x63, 0x72, 0x65, 0x65, 0x6E, 0x53, 0x69,  // ScreenSi
                    /* 05E8 */  0x7A, 0x65, 0x4D, 0x4D, 0x3E, 0x0A, 0x3C, 0x2F,  // zeMM>.</
                    /* 05F0 */  0x47, 0x72, 0x6F, 0x75, 0x70, 0x3E, 0x0A, 0x3C,  // Group>.<
                    /* 05F8 */  0x47, 0x72, 0x6F, 0x75, 0x70, 0x20, 0x69, 0x64,  // Group id
                    /* 0600 */  0x3D, 0x27, 0x41, 0x63, 0x74, 0x69, 0x76, 0x65,  // ='Active
                    /* 0608 */  0x20, 0x54, 0x69, 0x6D, 0x69, 0x6E, 0x67, 0x27,  //  Timing'
                    /* 0610 */  0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x48,  // >.    <H
                    /* 0618 */  0x6F, 0x72, 0x69, 0x7A, 0x6F, 0x6E, 0x74, 0x61,  // orizonta
                    /* 0620 */  0x6C, 0x41, 0x63, 0x74, 0x69, 0x76, 0x65, 0x3E,  // lActive>
                    /* 0628 */  0x31, 0x39, 0x32, 0x30, 0x3C, 0x2F, 0x48, 0x6F,  // 1920</Ho
                    /* 0630 */  0x72, 0x69, 0x7A, 0x6F, 0x6E, 0x74, 0x61, 0x6C,  // rizontal
                    /* 0638 */  0x41, 0x63, 0x74, 0x69, 0x76, 0x65, 0x3E, 0x0A,  // Active>.
                    /* 0640 */  0x20, 0x20, 0x20, 0x20, 0x3C, 0x48, 0x6F, 0x72,  //     <Hor
                    /* 0648 */  0x69, 0x7A, 0x6F, 0x6E, 0x74, 0x61, 0x6C, 0x46,  // izontalF
                    /* 0650 */  0x72, 0x6F, 0x6E, 0x74, 0x50, 0x6F, 0x72, 0x63,  // rontPorc
                    /* 0658 */  0x68, 0x3E, 0x38, 0x38, 0x3C, 0x2F, 0x48, 0x6F,  // h>88</Ho
                    /* 0660 */  0x72, 0x69, 0x7A, 0x6F, 0x6E, 0x74, 0x61, 0x6C,  // rizontal
                    /* 0668 */  0x46, 0x72, 0x6F, 0x6E, 0x74, 0x50, 0x6F, 0x72,  // FrontPor
                    /* 0670 */  0x63, 0x68, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20,  // ch>.    
                    /* 0678 */  0x3C, 0x48, 0x6F, 0x72, 0x69, 0x7A, 0x6F, 0x6E,  // <Horizon
                    /* 0680 */  0x74, 0x61, 0x6C, 0x42, 0x61, 0x63, 0x6B, 0x50,  // talBackP
                    /* 0688 */  0x6F, 0x72, 0x63, 0x68, 0x3E, 0x31, 0x34, 0x38,  // orch>148
                    /* 0690 */  0x3C, 0x2F, 0x48, 0x6F, 0x72, 0x69, 0x7A, 0x6F,  // </Horizo
                    /* 0698 */  0x6E, 0x74, 0x61, 0x6C, 0x42, 0x61, 0x63, 0x6B,  // ntalBack
                    /* 06A0 */  0x50, 0x6F, 0x72, 0x63, 0x68, 0x3E, 0x0A, 0x20,  // Porch>. 
                    /* 06A8 */  0x20, 0x20, 0x20, 0x3C, 0x48, 0x6F, 0x72, 0x69,  //    <Hori
                    /* 06B0 */  0x7A, 0x6F, 0x6E, 0x74, 0x61, 0x6C, 0x53, 0x79,  // zontalSy
                    /* 06B8 */  0x6E, 0x63, 0x50, 0x75, 0x6C, 0x73, 0x65, 0x3E,  // ncPulse>
                    /* 06C0 */  0x34, 0x34, 0x3C, 0x2F, 0x48, 0x6F, 0x72, 0x69,  // 44</Hori
                    /* 06C8 */  0x7A, 0x6F, 0x6E, 0x74, 0x61, 0x6C, 0x53, 0x79,  // zontalSy
                    /* 06D0 */  0x6E, 0x63, 0x50, 0x75, 0x6C, 0x73, 0x65, 0x3E,  // ncPulse>
                    /* 06D8 */  0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x48, 0x6F,  // .    <Ho
                    /* 06E0 */  0x72, 0x69, 0x7A, 0x6F, 0x6E, 0x74, 0x61, 0x6C,  // rizontal
                    /* 06E8 */  0x53, 0x79, 0x6E, 0x63, 0x53, 0x6B, 0x65, 0x77,  // SyncSkew
                    /* 06F0 */  0x3E, 0x30, 0x3C, 0x2F, 0x48, 0x6F, 0x72, 0x69,  // >0</Hori
                    /* 06F8 */  0x7A, 0x6F, 0x6E, 0x74, 0x61, 0x6C, 0x53, 0x79,  // zontalSy
                    /* 0700 */  0x6E, 0x63, 0x53, 0x6B, 0x65, 0x77, 0x3E, 0x0A,  // ncSkew>.
                    /* 0708 */  0x20, 0x20, 0x20, 0x20, 0x3C, 0x48, 0x6F, 0x72,  //     <Hor
                    /* 0710 */  0x69, 0x7A, 0x6F, 0x6E, 0x74, 0x61, 0x6C, 0x4C,  // izontalL
                    /* 0718 */  0x65, 0x66, 0x74, 0x42, 0x6F, 0x72, 0x64, 0x65,  // eftBorde
                    /* 0720 */  0x72, 0x3E, 0x30, 0x3C, 0x2F, 0x48, 0x6F, 0x72,  // r>0</Hor
                    /* 0728 */  0x69, 0x7A, 0x6F, 0x6E, 0x74, 0x61, 0x6C, 0x4C,  // izontalL
                    /* 0730 */  0x65, 0x66, 0x74, 0x42, 0x6F, 0x72, 0x64, 0x65,  // eftBorde
                    /* 0738 */  0x72, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C,  // r>.    <
                    /* 0740 */  0x48, 0x6F, 0x72, 0x69, 0x7A, 0x6F, 0x6E, 0x74,  // Horizont
                    /* 0748 */  0x61, 0x6C, 0x52, 0x69, 0x67, 0x68, 0x74, 0x42,  // alRightB
                    /* 0750 */  0x6F, 0x72, 0x64, 0x65, 0x72, 0x3E, 0x30, 0x3C,  // order>0<
                    /* 0758 */  0x2F, 0x48, 0x6F, 0x72, 0x69, 0x7A, 0x6F, 0x6E,  // /Horizon
                    /* 0760 */  0x74, 0x61, 0x6C, 0x52, 0x69, 0x67, 0x68, 0x74,  // talRight
                    /* 0768 */  0x42, 0x6F, 0x72, 0x64, 0x65, 0x72, 0x3E, 0x0A,  // Border>.
                    /* 0770 */  0x20, 0x20, 0x20, 0x20, 0x3C, 0x56, 0x65, 0x72,  //     <Ver
                    /* 0778 */  0x74, 0x69, 0x63, 0x61, 0x6C, 0x41, 0x63, 0x74,  // ticalAct
                    /* 0780 */  0x69, 0x76, 0x65, 0x3E, 0x31, 0x30, 0x38, 0x30,  // ive>1080
                    /* 0788 */  0x3C, 0x2F, 0x56, 0x65, 0x72, 0x74, 0x69, 0x63,  // </Vertic
                    /* 0790 */  0x61, 0x6C, 0x41, 0x63, 0x74, 0x69, 0x76, 0x65,  // alActive
                    /* 0798 */  0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x56,  // >.    <V
                    /* 07A0 */  0x65, 0x72, 0x74, 0x69, 0x63, 0x61, 0x6C, 0x46,  // erticalF
                    /* 07A8 */  0x72, 0x6F, 0x6E, 0x74, 0x50, 0x6F, 0x72, 0x63,  // rontPorc
                    /* 07B0 */  0x68, 0x3E, 0x34, 0x3C, 0x2F, 0x56, 0x65, 0x72,  // h>4</Ver
                    /* 07B8 */  0x74, 0x69, 0x63, 0x61, 0x6C, 0x46, 0x72, 0x6F,  // ticalFro
                    /* 07C0 */  0x6E, 0x74, 0x50, 0x6F, 0x72, 0x63, 0x68, 0x3E,  // ntPorch>
                    /* 07C8 */  0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x56, 0x65,  // .    <Ve
                    /* 07D0 */  0x72, 0x74, 0x69, 0x63, 0x61, 0x6C, 0x42, 0x61,  // rticalBa
                    /* 07D8 */  0x63, 0x6B, 0x50, 0x6F, 0x72, 0x63, 0x68, 0x3E,  // ckPorch>
                    /* 07E0 */  0x33, 0x36, 0x3C, 0x2F, 0x56, 0x65, 0x72, 0x74,  // 36</Vert
                    /* 07E8 */  0x69, 0x63, 0x61, 0x6C, 0x42, 0x61, 0x63, 0x6B,  // icalBack
                    /* 07F0 */  0x50, 0x6F, 0x72, 0x63, 0x68, 0x3E, 0x0A, 0x20,  // Porch>. 
                    /* 07F8 */  0x20, 0x20, 0x20, 0x3C, 0x56, 0x65, 0x72, 0x74,  //    <Vert
                    /* 0800 */  0x69, 0x63, 0x61, 0x6C, 0x53, 0x79, 0x6E, 0x63,  // icalSync
                    /* 0808 */  0x50, 0x75, 0x6C, 0x73, 0x65, 0x3E, 0x35, 0x3C,  // Pulse>5<
                    /* 0810 */  0x2F, 0x56, 0x65, 0x72, 0x74, 0x69, 0x63, 0x61,  // /Vertica
                    /* 0818 */  0x6C, 0x53, 0x79, 0x6E, 0x63, 0x50, 0x75, 0x6C,  // lSyncPul
                    /* 0820 */  0x73, 0x65, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20,  // se>.    
                    /* 0828 */  0x3C, 0x56, 0x65, 0x72, 0x74, 0x69, 0x63, 0x61,  // <Vertica
                    /* 0830 */  0x6C, 0x53, 0x79, 0x6E, 0x63, 0x53, 0x6B, 0x65,  // lSyncSke
                    /* 0838 */  0x77, 0x3E, 0x30, 0x3C, 0x2F, 0x56, 0x65, 0x72,  // w>0</Ver
                    /* 0840 */  0x74, 0x69, 0x63, 0x61, 0x6C, 0x53, 0x79, 0x6E,  // ticalSyn
                    /* 0848 */  0x63, 0x53, 0x6B, 0x65, 0x77, 0x3E, 0x0A, 0x20,  // cSkew>. 
                    /* 0850 */  0x20, 0x20, 0x20, 0x3C, 0x56, 0x65, 0x72, 0x74,  //    <Vert
                    /* 0858 */  0x69, 0x63, 0x61, 0x6C, 0x54, 0x6F, 0x70, 0x42,  // icalTopB
                    /* 0860 */  0x6F, 0x72, 0x64, 0x65, 0x72, 0x3E, 0x30, 0x3C,  // order>0<
                    /* 0868 */  0x2F, 0x56, 0x65, 0x72, 0x74, 0x69, 0x63, 0x61,  // /Vertica
                    /* 0870 */  0x6C, 0x54, 0x6F, 0x70, 0x42, 0x6F, 0x72, 0x64,  // lTopBord
                    /* 0878 */  0x65, 0x72, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20,  // er>.    
                    /* 0880 */  0x3C, 0x56, 0x65, 0x72, 0x74, 0x69, 0x63, 0x61,  // <Vertica
                    /* 0888 */  0x6C, 0x42, 0x6F, 0x74, 0x74, 0x6F, 0x6D, 0x42,  // lBottomB
                    /* 0890 */  0x6F, 0x72, 0x64, 0x65, 0x72, 0x3E, 0x30, 0x3C,  // order>0<
                    /* 0898 */  0x2F, 0x56, 0x65, 0x72, 0x74, 0x69, 0x63, 0x61,  // /Vertica
                    /* 08A0 */  0x6C, 0x42, 0x6F, 0x74, 0x74, 0x6F, 0x6D, 0x42,  // lBottomB
                    /* 08A8 */  0x6F, 0x72, 0x64, 0x65, 0x72, 0x3E, 0x0A, 0x3C,  // order>.<
                    /* 08B0 */  0x2F, 0x47, 0x72, 0x6F, 0x75, 0x70, 0x3E, 0x0A,  // /Group>.
                    /* 08B8 */  0x3C, 0x47, 0x72, 0x6F, 0x75, 0x70, 0x20, 0x69,  // <Group i
                    /* 08C0 */  0x64, 0x3D, 0x27, 0x44, 0x69, 0x73, 0x70, 0x6C,  // d='Displ
                    /* 08C8 */  0x61, 0x79, 0x20, 0x49, 0x6E, 0x74, 0x65, 0x72,  // ay Inter
                    /* 08D0 */  0x66, 0x61, 0x63, 0x65, 0x27, 0x3E, 0x0A, 0x20,  // face'>. 
                    /* 08D8 */  0x20, 0x20, 0x20, 0x3C, 0x49, 0x6E, 0x74, 0x65,  //    <Inte
                    /* 08E0 */  0x72, 0x66, 0x61, 0x63, 0x65, 0x54, 0x79, 0x70,  // rfaceTyp
                    /* 08E8 */  0x65, 0x3E, 0x31, 0x30, 0x3C, 0x2F, 0x49, 0x6E,  // e>10</In
                    /* 08F0 */  0x74, 0x65, 0x72, 0x66, 0x61, 0x63, 0x65, 0x54,  // terfaceT
                    /* 08F8 */  0x79, 0x70, 0x65, 0x3E, 0x0A, 0x20, 0x20, 0x20,  // ype>.   
                    /* 0900 */  0x20, 0x3C, 0x49, 0x6E, 0x74, 0x65, 0x72, 0x66,  //  <Interf
                    /* 0908 */  0x61, 0x63, 0x65, 0x43, 0x6F, 0x6C, 0x6F, 0x72,  // aceColor
                    /* 0910 */  0x46, 0x6F, 0x72, 0x6D, 0x61, 0x74, 0x3E, 0x33,  // Format>3
                    /* 0918 */  0x3C, 0x2F, 0x49, 0x6E, 0x74, 0x65, 0x72, 0x66,  // </Interf
                    /* 0920 */  0x61, 0x63, 0x65, 0x43, 0x6F, 0x6C, 0x6F, 0x72,  // aceColor
                    /* 0928 */  0x46, 0x6F, 0x72, 0x6D, 0x61, 0x74, 0x3E, 0x0A,  // Format>.
                    /* 0930 */  0x3C, 0x2F, 0x47, 0x72, 0x6F, 0x75, 0x70, 0x3E,  // </Group>
                    /* 0938 */  0x0A, 0x3C, 0x47, 0x72, 0x6F, 0x75, 0x70, 0x20,  // .<Group 
                    /* 0940 */  0x69, 0x64, 0x3D, 0x27, 0x44, 0x69, 0x73, 0x70,  // id='Disp
                    /* 0948 */  0x6C, 0x61, 0x79, 0x50, 0x6F, 0x72, 0x74, 0x20,  // layPort 
                    /* 0950 */  0x49, 0x6E, 0x74, 0x65, 0x72, 0x66, 0x61, 0x63,  // Interfac
                    /* 0958 */  0x65, 0x27, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20,  // e'>.    
                    /* 0960 */  0x3C, 0x44, 0x50, 0x43, 0x6F, 0x6E, 0x74, 0x72,  // <DPContr
                    /* 0968 */  0x6F, 0x6C, 0x6C, 0x65, 0x72, 0x49, 0x64, 0x3E,  // ollerId>
                    /* 0970 */  0x30, 0x3C, 0x2F, 0x44, 0x50, 0x43, 0x6F, 0x6E,  // 0</DPCon
                    /* 0978 */  0x74, 0x72, 0x6F, 0x6C, 0x6C, 0x65, 0x72, 0x49,  // trollerI
                    /* 0980 */  0x64, 0x3E, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C,  // d>.    <
                    /* 0988 */  0x44, 0x50, 0x4C, 0x69, 0x6E, 0x6B, 0x52, 0x61,  // DPLinkRa
                    /* 0990 */  0x74, 0x65, 0x3E, 0x30, 0x78, 0x31, 0x34, 0x3C,  // te>0x14<
                    /* 0998 */  0x2F, 0x44, 0x50, 0x4C, 0x69, 0x6E, 0x6B, 0x52,  // /DPLinkR
                    /* 09A0 */  0x61, 0x74, 0x65, 0x3E, 0x0A, 0x20, 0x20, 0x20,  // ate>.   
                    /* 09A8 */  0x20, 0x3C, 0x44, 0x50, 0x4C, 0x61, 0x6E, 0x65,  //  <DPLane
                    /* 09B0 */  0x73, 0x3E, 0x34, 0x3C, 0x2F, 0x44, 0x50, 0x4C,  // s>4</DPL
                    /* 09B8 */  0x61, 0x6E, 0x65, 0x73, 0x3E, 0x0A, 0x20, 0x20,  // anes>.  
                    /* 09C0 */  0x20, 0x20, 0x3C, 0x44, 0x50, 0x41, 0x75, 0x78,  //   <DPAux
                    /* 09C8 */  0x43, 0x68, 0x61, 0x6E, 0x6E, 0x65, 0x6C, 0x3E,  // Channel>
                    /* 09D0 */  0x30, 0x3C, 0x2F, 0x44, 0x50, 0x41, 0x75, 0x78,  // 0</DPAux
                    /* 09D8 */  0x43, 0x68, 0x61, 0x6E, 0x6E, 0x65, 0x6C, 0x3E,  // Channel>
                    /* 09E0 */  0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x44, 0x50,  // .    <DP
                    /* 09E8 */  0x41, 0x6C, 0x74, 0x4D, 0x6F, 0x64, 0x65, 0x3E,  // AltMode>
                    /* 09F0 */  0x54, 0x72, 0x75, 0x65, 0x3C, 0x2F, 0x44, 0x50,  // True</DP
                    /* 09F8 */  0x41, 0x6C, 0x74, 0x4D, 0x6F, 0x64, 0x65, 0x3E,  // AltMode>
                    /* 0A00 */  0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x44, 0x50,  // .    <DP
                    /* 0A08 */  0x43, 0x6F, 0x6E, 0x6E, 0x65, 0x63, 0x74, 0x6F,  // Connecto
                    /* 0A10 */  0x72, 0x3E, 0x55, 0x53, 0x42, 0x2D, 0x43, 0x3C,  // r>USB-C<
                    /* 0A18 */  0x2F, 0x44, 0x50, 0x43, 0x6F, 0x6E, 0x6E, 0x65,  // /DPConne
                    /* 0A20 */  0x63, 0x74, 0x6F, 0x72, 0x3E, 0x0A, 0x3C, 0x2F,  // ctor>.</
                    /* 0A28 */  0x47, 0x72, 0x6F, 0x75, 0x70, 0x3E, 0x0A, 0x3C,  // Group>.<
                    /* 0A30 */  0x47, 0x72, 0x6F, 0x75, 0x70, 0x20, 0x69, 0x64,  // Group id
                    /* 0A38 */  0x3D, 0x27, 0x43, 0x6F, 0x6E, 0x6E, 0x65, 0x63,  // ='Connec
                    /* 0A40 */  0x74, 0x69, 0x6F, 0x6E, 0x20, 0x43, 0x6F, 0x6E,  // tion Con
                    /* 0A48 */  0x66, 0x69, 0x67, 0x75, 0x72, 0x61, 0x74, 0x69,  // figurati
                    /* 0A50 */  0x6F, 0x6E, 0x27, 0x3E, 0x0A, 0x3C, 0x2F, 0x47,  // on'>.</G
                    /* 0A58 */  0x72, 0x6F, 0x75, 0x70, 0x3E, 0x0A, 0x3C, 0x44,  // roup>.<D
                    /* 0A60 */  0x69, 0x73, 0x70, 0x6C, 0x61, 0x79, 0x50, 0x72,  // isplayPr
                    /* 0A68 */  0x69, 0x6D, 0x61, 0x72, 0x79, 0x46, 0x6C, 0x61,  // imaryFla
                    /* 0A70 */  0x67, 0x73, 0x3E, 0x31, 0x36, 0x37, 0x37, 0x37,  // gs>16777
                    /* 0A78 */  0x32, 0x31, 0x36, 0x3C, 0x2F, 0x44, 0x69, 0x73,  // 216</Dis
                    /* 0A80 */  0x70, 0x6C, 0x61, 0x79, 0x50, 0x72, 0x69, 0x6D,  // playPrim
                    /* 0A88 */  0x61, 0x72, 0x79, 0x46, 0x6C, 0x61, 0x67, 0x73,  // aryFlags
                    /* 0A90 */  0x3E, 0x0A, 0x3C, 0x47, 0x72, 0x6F, 0x75, 0x70,  // >.<Group
                    /* 0A98 */  0x20, 0x69, 0x64, 0x3D, 0x27, 0x42, 0x61, 0x63,  //  id='Bac
                    /* 0AA0 */  0x6B, 0x6C, 0x69, 0x67, 0x68, 0x74, 0x20, 0x43,  // klight C
                    /* 0AA8 */  0x6F, 0x6E, 0x66, 0x69, 0x67, 0x75, 0x72, 0x61,  // onfigura
                    /* 0AB0 */  0x74, 0x69, 0x6F, 0x6E, 0x27, 0x3E, 0x0A, 0x20,  // tion'>. 
                    /* 0AB8 */  0x20, 0x20, 0x20, 0x3C, 0x42, 0x61, 0x63, 0x6B,  //    <Back
                    /* 0AC0 */  0x6C, 0x69, 0x67, 0x68, 0x74, 0x54, 0x79, 0x70,  // lightTyp
                    /* 0AC8 */  0x65, 0x3E, 0x30, 0x3C, 0x2F, 0x42, 0x61, 0x63,  // e>0</Bac
                    /* 0AD0 */  0x6B, 0x6C, 0x69, 0x67, 0x68, 0x74, 0x54, 0x79,  // klightTy
                    /* 0AD8 */  0x70, 0x65, 0x3E, 0x0A, 0x3C, 0x2F, 0x47, 0x72,  // pe>.</Gr
                    /* 0AE0 */  0x6F, 0x75, 0x70, 0x3E, 0x00, 0x00, 0x00, 0x00,  // oup>....
                    /* 0AE8 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0AF0 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0AF8 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0B00 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0B08 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0B10 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0B18 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0B20 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0B28 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0B30 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0B38 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0B40 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0B48 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0B50 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0B58 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0B60 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0B68 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0B70 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0B78 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0B80 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0B88 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0B90 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0B98 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0BA0 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0BA8 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0BB0 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0BB8 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0BC0 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0BC8 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0BD0 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0BD8 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0BE0 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0BE8 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0BF0 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0BF8 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0C00 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0C08 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0C10 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0C18 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0C20 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0C28 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0C30 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0C38 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0C40 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0C48 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0C50 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0C58 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0C60 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0C68 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0C70 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0C78 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0C80 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0C88 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0C90 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0C98 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0CA0 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0CA8 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0CB0 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0CB8 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0CC0 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0CC8 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0CD0 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0CD8 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0CE0 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0CE8 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0CF0 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0CF8 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0D00 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0D08 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0D10 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0D18 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0D20 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0D28 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0D30 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0D38 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0D40 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0D48 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                    /* 0D50 */  0x00, 0x00, 0x00, 0x00                           // ....
                })
                While (One)
                {
                    Name (_T_0, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                    _T_0 = ToInteger (Arg1)
                    If ((_T_0 == 0x6F00))
                    {
                        Local2 = PCFG /* \_SB_.GPU0._ROM.PCFG */
                    }
                    Else
                    {
                        Local2 = PCFG /* \_SB_.GPU0._ROM.PCFG */
                    }

                    Break
                }

                If ((Arg0 >= SizeOf (Local2)))
                {
                    Return (Buffer (One)
                    {
                         0x00                                             // .
                    })
                }
                Else
                {
                    Local0 = Arg0
                }

                If ((Arg1 > 0x1000))
                {
                    Local1 = 0x1000
                }
                Else
                {
                    Local1 = Arg1
                }

                If (((Local0 + Local1) > SizeOf (Local2)))
                {
                    Local1 = (SizeOf (Local2) - Local0)
                }

                CreateField (Local2, (0x08 * Local0), (0x08 * Local1), RBUF)
                Return (RBUF) /* \_SB_.GPU0._ROM.RBUF */
            }

            Method (PGRT, 2, NotSerialized)
            {
                Name (RBUF, Buffer (One)
                {
                     0x00                                             // .
                })
                Return (RBUF) /* \_SB_.GPU0.PGRT.RBUF */
            }

            Method (PBRT, 2, NotSerialized)
            {
                Name (RBUF, Buffer (One)
                {
                     0x00                                             // .
                })
                Return (RBUF) /* \_SB_.GPU0.PBRT.RBUF */
            }

            Method (ROE1, 3, NotSerialized)
            {
                Name (PCFG, Buffer (One)
                {
                     0x00                                             // .
                })
                Local2 = PCFG /* \_SB_.GPU0.ROE1.PCFG */
                If ((Arg0 >= SizeOf (Local2)))
                {
                    Return (Buffer (One)
                    {
                         0x00                                             // .
                    })
                }
                Else
                {
                    Local0 = Arg0
                }

                If ((Arg1 > 0x1000))
                {
                    Local1 = 0x1000
                }
                Else
                {
                    Local1 = Arg1
                }

                If (((Local0 + Local1) > SizeOf (Local2)))
                {
                    Local1 = (SizeOf (Local2) - Local0)
                }

                CreateField (Local2, (0x08 * Local0), (0x08 * Local1), RBUF)
                Return (RBUF) /* \_SB_.GPU0.ROE1.RBUF */
            }

            Method (ROE2, 3, NotSerialized)
            {
                Name (PCFG, Buffer (One)
                {
                     0x00                                             // .
                })
                Local2 = PCFG /* \_SB_.GPU0.ROE2.PCFG */
                If ((Arg0 >= SizeOf (Local2)))
                {
                    Return (Buffer (One)
                    {
                         0x00                                             // .
                    })
                }
                Else
                {
                    Local0 = Arg0
                }

                If ((Arg1 > 0x1000))
                {
                    Local1 = 0x1000
                }
                Else
                {
                    Local1 = Arg1
                }

                If (((Local0 + Local1) > SizeOf (Local2)))
                {
                    Local1 = (SizeOf (Local2) - Local0)
                }

                CreateField (Local2, (0x08 * Local0), (0x08 * Local1), RBUF)
                Return (RBUF) /* \_SB_.GPU0.ROE2.RBUF */
            }

            Method (ROE3, 3, NotSerialized)
            {
                Name (PCFG, Buffer (One)
                {
                     0x00                                             // .
                })
                Local2 = PCFG /* \_SB_.GPU0.ROE3.PCFG */
                If ((Arg0 >= SizeOf (Local2)))
                {
                    Return (Buffer (One)
                    {
                         0x00                                             // .
                    })
                }
                Else
                {
                    Local0 = Arg0
                }

                If ((Arg1 > 0x1000))
                {
                    Local1 = 0x1000
                }
                Else
                {
                    Local1 = Arg1
                }

                If (((Local0 + Local1) > SizeOf (Local2)))
                {
                    Local1 = (SizeOf (Local2) - Local0)
                }

                CreateField (Local2, (0x08 * Local0), (0x08 * Local1), RBUF)
                Return (RBUF) /* \_SB_.GPU0.ROE3.RBUF */
            }

            Method (ROE4, 3, NotSerialized)
            {
                Name (PCFG, Buffer (One)
                {
                     0x00                                             // .
                })
                Local2 = PCFG /* \_SB_.GPU0.ROE4.PCFG */
                If ((Arg0 >= SizeOf (Local2)))
                {
                    Return (Buffer (One)
                    {
                         0x00                                             // .
                    })
                }
                Else
                {
                    Local0 = Arg0
                }

                If ((Arg1 > 0x1000))
                {
                    Local1 = 0x1000
                }
                Else
                {
                    Local1 = Arg1
                }

                If (((Local0 + Local1) > SizeOf (Local2)))
                {
                    Local1 = (SizeOf (Local2) - Local0)
                }

                CreateField (Local2, (0x08 * Local0), (0x08 * Local1), RBUF)
                Return (RBUF) /* \_SB_.GPU0.ROE4.RBUF */
            }

            Method (ROE5, 3, NotSerialized)
            {
                Name (PCFG, Buffer (One)
                {
                     0x00                                             // .
                })
                Local2 = PCFG /* \_SB_.GPU0.ROE5.PCFG */
                If ((Arg0 >= SizeOf (Local2)))
                {
                    Return (Buffer (One)
                    {
                         0x00                                             // .
                    })
                }
                Else
                {
                    Local0 = Arg0
                }

                If ((Arg1 > 0x1000))
                {
                    Local1 = 0x1000
                }
                Else
                {
                    Local1 = Arg1
                }

                If (((Local0 + Local1) > SizeOf (Local2)))
                {
                    Local1 = (SizeOf (Local2) - Local0)
                }

                CreateField (Local2, (0x08 * Local0), (0x08 * Local1), RBUF)
                Return (RBUF) /* \_SB_.GPU0.ROE5.RBUF */
            }

            Method (ROE6, 3, NotSerialized)
            {
                Name (PCFG, Buffer (One)
                {
                     0x00                                             // .
                })
                Local2 = PCFG /* \_SB_.GPU0.ROE6.PCFG */
                If ((Arg0 >= SizeOf (Local2)))
                {
                    Return (Buffer (One)
                    {
                         0x00                                             // .
                    })
                }
                Else
                {
                    Local0 = Arg0
                }

                If ((Arg1 > 0x1000))
                {
                    Local1 = 0x1000
                }
                Else
                {
                    Local1 = Arg1
                }

                If (((Local0 + Local1) > SizeOf (Local2)))
                {
                    Local1 = (SizeOf (Local2) - Local0)
                }

                CreateField (Local2, (0x08 * Local0), (0x08 * Local1), RBUF)
                Return (RBUF) /* \_SB_.GPU0.ROE6.RBUF */
            }

            Name (_DOD, Package (0x01)  // _DOD: Display Output Devices
            {
                0x00024321
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }

            Method (_HRV, 0, NotSerialized)  // _HRV: Hardware Revision
            {
                Name (RESU, Zero)
                Name (TIER, Zero)
                Name (DREV, Zero)
                Name (FAMI, Zero)
                Name (PROD, Zero)
                Name (DDRT, Zero)
                TIER = (\_SB.SIDT & 0x0F)
                DREV = ((\_SB.SJTG >> 0x1C) & 0x0F)
                DREV <<= 0x04
                PROD = ((\_SB.SJTG >> 0x0C) & 0xFFFF)
                If (((PROD == 0x0193) | (PROD == 0x01EB)))
                {
                    FAMI = (One << 0x08)
                }

                If ((PROD == 0x0194))
                {
                    FAMI = (0x02 << 0x08)
                }

                If (((PROD == 0x01E3) | (PROD == 0x020A)))
                {
                    FAMI = (0x03 << 0x08)
                }

                If ((PROD == 0x0215))
                {
                    FAMI = (0x04 << 0x08)
                }

                If (((PROD == 0x0197) | (PROD == 0x0198)))
                {
                    FAMI = (0x05 << 0x08)
                }

                If (((PROD == 0x020F) | (PROD == 0x020E)))
                {
                    FAMI = (0x06 << 0x08)
                }

                If ((\_SB.SDDR == 0x05))
                {
                    DDRT = (One << 0x0B)
                }

                RESU |= TIER /* \_SB_.GPU0._HRV.RESU */
                RESU |= DREV /* \_SB_.GPU0._HRV.RESU */
                RESU |= FAMI /* \_SB_.GPU0._HRV.RESU */
                RESU |= DDRT /* \_SB_.GPU0._HRV.RESU */
                Return (RESU) /* \_SB_.GPU0._HRV.RESU */
            }

            Device (AVS0)
            {
                Name (_ADR, 0x00024321)  // _ADR: Address
                Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
                {
                    Name (RBUF, Buffer (0x02)
                    {
                         0x79, 0x00                                       // y.
                    })
                    Return (RBUF) /* \_SB_.GPU0.AVS0._CRS.RBUF */
                }

                Name (_DEP, Package (0x03)  // _DEP: Dependencies
                {
                    \_SB.MMU0, , 
                    \_SB.IMM0, , 
                    \_SB.VFE0, 
                })
            }

            Method (CHDV, 0, NotSerialized)
            {
                Name (CHIF, Package (0x02)
                {
                    One, 
                    Package (0x07)
                    {
                        "CHILDDEV", 
                        Zero, 
                        0x00024321, 
                        "QCOM_AVStream_6490", 
                        Zero, 
                        "Qualcomm Camera AVStream Mini Driver", 
                        Package (0x04)
                        {
                            "COMPATIBLEIDS", 
                            0x02, 
                            "VEN_QCOM&DEV__AVSTREAM", 
                            "QCOM_AVSTREAM"
                        }
                    }
                })
                Return (CHIF) /* \_SB_.GPU0.CHDV.CHIF */
            }

            Method (_LID, 0, NotSerialized)  // _LID: Lid Status
            {
                Return (One)
            }
        }

        Device (SCM0)
        {
            Name (_HID, "QCOM04DD")  // _HID: Hardware ID
            Name (_DEP, Package (0x01)  // _DEP: Dependencies
            {
                \_SB.PEP0, 
            })
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, Zero)  // _UID: Unique ID
        }

        Device (TLOG)
        {
            Name (_HID, "QCOM0AE3")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, Zero)  // _UID: Unique ID
        }

        Device (TREE)
        {
            Name (_HID, "QCOM04DE")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, Zero)  // _UID: Unique ID
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x31)
                {
                    /* 0000 */  0x8C, 0x20, 0x00, 0x01, 0x00, 0x01, 0x00, 0x03,  // . ......
                    /* 0008 */  0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00,  // ........
                    /* 0010 */  0x00, 0x19, 0x00, 0x23, 0x00, 0x00, 0x00, 0x68,  // ...#...h
                    /* 0018 */  0x00, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x47, 0x49,  // .\_SB.GI
                    /* 0020 */  0x4F, 0x30, 0x00, 0x86, 0x09, 0x00, 0x01, 0xEF,  // O0......
                    /* 0028 */  0xBE, 0xAD, 0xDE, 0xAD, 0xDE, 0xEF, 0xBE, 0x79,  // .......y
                    /* 0030 */  0x00                                             // .
                })
                CreateDWordField (RBUF, 0x27, TGCA)
                CreateDWordField (RBUF, 0x2B, TGCL)
                TGCA = \_SB.TCMA
                TGCL = \_SB.TCML
                Return (RBUF) /* \_SB_.TREE._CRS.RBUF */
            }
        }

        Device (SPMI)
        {
            Name (_HID, "QCOM0A0B")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_CID, "PNP0CA2")  // _CID: Compatible ID
            Name (_UID, One)  // _UID: Unique ID
            Name (_CCA, Zero)  // _CCA: Cache Coherency Attribute
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x0E)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0x40, 0x0C,  // ......@.
                    /* 0008 */  0x00, 0x00, 0x80, 0x02, 0x79, 0x00               // ....y.
                })
                Return (RBUF) /* \_SB_.SPMI._CRS.RBUF */
            }

            Method (CONF, 0, NotSerialized)
            {
                Name (XBUF, Buffer (0x1A)
                {
                    /* 0000 */  0x00, 0x01, 0x01, 0x01, 0xFF, 0x00, 0x02, 0x00,  // ........
                    /* 0008 */  0x0A, 0x07, 0x04, 0x07, 0x01, 0xFF, 0x10, 0x01,  // ........
                    /* 0010 */  0x00, 0x01, 0x0C, 0x40, 0x00, 0x00, 0x02, 0x80,  // ...@....
                    /* 0018 */  0x00, 0x00                                       // ..
                })
                Return (XBUF) /* \_SB_.SPMI.CONF.XBUF */
            }
        }

        Device (GIO0)
        {
            Name (_HID, "QCOM0A0C")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Alias (\_SB.PSUB, _SUB)
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x68)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0x10, 0x0F,  // ........
                    /* 0008 */  0x00, 0x00, 0x30, 0x00, 0x89, 0x06, 0x00, 0x09,  // ..0.....
                    /* 0010 */  0x01, 0xF0, 0x00, 0x00, 0x00, 0x89, 0x06, 0x00,  // ........
                    /* 0018 */  0x09, 0x01, 0xF0, 0x00, 0x00, 0x00, 0x89, 0x06,  // ........
                    /* 0020 */  0x00, 0x09, 0x01, 0xF0, 0x00, 0x00, 0x00, 0x89,  // ........
                    /* 0028 */  0x06, 0x00, 0x09, 0x01, 0x2E, 0x02, 0x00, 0x00,  // ........
                    /* 0030 */  0x89, 0x06, 0x00, 0x09, 0x01, 0x28, 0x02, 0x00,  // .....(..
                    /* 0038 */  0x00, 0x89, 0x06, 0x00, 0x01, 0x01, 0x85, 0x02,  // ........
                    /* 0040 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x0B, 0x01, 0x33,  // .......3
                    /* 0048 */  0x02, 0x00, 0x00, 0x89, 0x06, 0x00, 0x0B, 0x01,  // ........
                    /* 0050 */  0x2B, 0x02, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03,  // +.......
                    /* 0058 */  0x01, 0x81, 0x02, 0x00, 0x00, 0x89, 0x06, 0x00,  // ........
                    /* 0060 */  0x03, 0x01, 0x88, 0x02, 0x00, 0x00, 0x79, 0x00   // ......y.
                })
                Return (RBUF) /* \_SB_.GIO0._CRS.RBUF */
            }

            Method (OFNI, 0, NotSerialized)
            {
                Name (RBUF, Buffer (0x02)
                {
                     0xAF, 0x00                                       // ..
                })
                Return (RBUF) /* \_SB_.GIO0.OFNI.RBUF */
            }

            Name (GABL, Zero)
            Method (_REG, 2, NotSerialized)  // _REG: Region Availability
            {
                If ((Arg0 == 0x08))
                {
                    GABL = Arg1
                }
            }

            Method (_AEI, 0, NotSerialized)  // _AEI: ACPI Event Interrupts
            {
                If ((\_SB.SKUV == Zero))
                {
                    Name (RBF0, Buffer (0x25)
                    {
                        /* 0000 */  0x8C, 0x20, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,  // . ......
                        /* 0008 */  0x00, 0x02, 0x00, 0x00, 0xF4, 0x01, 0x17, 0x00,  // ........
                        /* 0010 */  0x00, 0x19, 0x00, 0x23, 0x00, 0x00, 0x00, 0x03,  // ...#....
                        /* 0018 */  0x00, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x47, 0x49,  // .\_SB.GI
                        /* 0020 */  0x4F, 0x30, 0x00, 0x79, 0x00                     // O0.y.
                    })
                    Return (RBF0) /* \_SB_.GIO0._AEI.RBF0 */
                }
            }

            Method (_EBD, 0, NotSerialized)  // _Exx: Edge-Triggered GPE, xx=0x00-0xFF
            {
                Notify (\_SB.GPU0, 0x92) // Device-Specific
            }

            Method (_EVT, 1, NotSerialized)  // _EVT: Event
            {
                Debug = "RCVD EC INT"
            }

            Method (_DSM, 4, NotSerialized)  // _DSM: Device-Specific Method
            {
                If ((Arg0 == ToUUID ("4f248f40-d5e2-499f-834c-27758ea1cd3f") /* GPIO Controller */))
                {
                    While (One)
                    {
                        Name (_T_0, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                        _T_0 = Arg2
                        If ((_T_0 == Zero))
                        {
                            Return (Buffer (One)
                            {
                                 0x03                                             // .
                            })
                        }
                        ElseIf ((_T_0 == One))
                        {
                            Return (Package (0x01)
                            {
                                0x0100
                            })
                        }
                        Else
                        {
                            BreakPoint
                        }

                        Break
                    }
                }
                Else
                {
                    Return (Buffer (One)
                    {
                         0x00                                             // .
                    })
                }
            }
        }

        Device (IPCC)
        {
            Name (_HID, "QCOM06C2")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Alias (\_SB.PSUB, _SUB)
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x26)
                {
                    /* 0000 */  0x89, 0x06, 0x00, 0x03, 0x01, 0x05, 0x01, 0x00,  // ........
                    /* 0008 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x06, 0x01,  // ........
                    /* 0010 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x07,  // ........
                    /* 0018 */  0x01, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01,  // ........
                    /* 0020 */  0xEA, 0x02, 0x00, 0x00, 0x79, 0x00               // ....y.
                })
                Return (RBUF) /* \_SB_.IPCC._CRS.RBUF */
            }
        }

        Device (QPPX)
        {
            Name (_HID, "QCOM0A96")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_CCA, Zero)  // _CCA: Cache Coherency Attribute
            Alias (\_SB.PSUB, _SUB)
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }

            Method (_CRS, 0, Serialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x4A)
                {
                    /* 0000 */  0x8C, 0x21, 0x00, 0x01, 0x01, 0x01, 0x00, 0x08,  // .!......
                    /* 0008 */  0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00,  // ........
                    /* 0010 */  0x00, 0x19, 0x00, 0x23, 0x00, 0x01, 0x00, 0x57,  // ...#...W
                    /* 0018 */  0x00, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x47, 0x49,  // .\_SB.GI
                    /* 0020 */  0x4F, 0x30, 0x00, 0x01, 0x8C, 0x21, 0x00, 0x01,  // O0...!..
                    /* 0028 */  0x01, 0x01, 0x00, 0x08, 0x00, 0x03, 0x00, 0x00,  // ........
                    /* 0030 */  0x00, 0x00, 0x17, 0x00, 0x00, 0x19, 0x00, 0x23,  // .......#
                    /* 0038 */  0x00, 0x01, 0x00, 0x02, 0x00, 0x5C, 0x5F, 0x53,  // .....\_S
                    /* 0040 */  0x42, 0x2E, 0x47, 0x49, 0x4F, 0x30, 0x00, 0x01,  // B.GIO0..
                    /* 0048 */  0x79, 0x00                                       // y.
                })
                Return (RBUF) /* \_SB_.QPPX._CRS.RBUF */
            }

            Method (_QPG, 0, Serialized)
            {
                Return (Package (0x02)
                {
                    One, 
                    One
                })
            }
        }

        Device (PCI0)
        {
            Name (_DEP, Package (0x02)  // _DEP: Dependencies
            {
                \_SB.PEP0, , 
                \_SB.QPPX, 
            })
            Name (_HID, EisaId ("PNP0A08") /* PCI Express Bus */)  // _HID: Hardware ID
            Name (_CID, EisaId ("PNP0A03") /* PCI Bus */)  // _CID: Compatible ID
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_SEG, Zero)  // _SEG: PCI Segment
            Name (_BBN, Zero)  // _BBN: BIOS Bus Number
            Name (_PRT, Package (0x04)  // _PRT: PCI Routing Table
            {
                Package (0x04)
                {
                    0xFFFF, 
                    Zero, 
                    Zero, 
                    0xB5
                }, 

                Package (0x04)
                {
                    0xFFFF, 
                    One, 
                    Zero, 
                    0xB6
                }, 

                Package (0x04)
                {
                    0xFFFF, 
                    0x02, 
                    Zero, 
                    0xB7
                }, 

                Package (0x04)
                {
                    0xFFFF, 
                    0x03, 
                    Zero, 
                    0xB8
                }
            })
            Method (_CCA, 0, NotSerialized)  // _CCA: Cache Coherency Attribute
            {
                Return (One)
            }

            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If ((PRP0 == One))
                {
                    Return (0x0F)
                }
                Else
                {
                    Return (Zero)
                }
            }

            Method (_PSC, 0, NotSerialized)  // _PSC: Power State Current
            {
                Return (Zero)
            }

            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x2C)
                {
                    /* 0000 */  0x87, 0x17, 0x00, 0x00, 0x0C, 0x01, 0x00, 0x00,  // ........
                    /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x70, 0x61, 0xFF, 0xFF,  // ....pa..
                    /* 0010 */  0xFF, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // .c......
                    /* 0018 */  0x90, 0x02, 0x88, 0x0D, 0x00, 0x02, 0x0C, 0x00,  // ........
                    /* 0020 */  0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00,  // ........
                    /* 0028 */  0x10, 0x00, 0x79, 0x00                           // ..y.
                })
                Return (RBUF) /* \_SB_.PCI0._CRS.RBUF */
            }

            Name (SUPP, Zero)
            Name (CTRL, Zero)
            Method (_DSW, 3, NotSerialized)  // _DSW: Device Sleep Wake
            {
            }

            Method (_OSC, 4, NotSerialized)  // _OSC: Operating System Capabilities
            {
                If ((Arg0 == ToUUID ("33db4d5b-1ff7-401c-9657-7441c03dd766") /* PCI Host Bridge Device */))
                {
                    CreateDWordField (Arg3, Zero, CDW1)
                    CreateDWordField (Arg3, 0x04, CDW2)
                    CreateDWordField (Arg3, 0x08, CDW3)
                    SUPP = CDW2 /* \_SB_.PCI0._OSC.CDW2 */
                    CTRL = CDW3 /* \_SB_.PCI0._OSC.CDW3 */
                    If (((SUPP & 0x16) != 0x16))
                    {
                        CTRL &= 0x1E
                    }

                    CTRL &= 0x15
                    If ((Arg1 != One))
                    {
                        CDW1 |= 0x08
                    }

                    If ((CDW3 != CTRL))
                    {
                        CDW1 |= 0x10
                    }

                    CDW3 = CTRL /* \_SB_.PCI0.CTRL */
                    Return (Arg3)
                }
                Else
                {
                    CDW1 |= 0x04
                    Return (Arg3)
                }
            }

            Method (_DSM, 4, NotSerialized)  // _DSM: Device-Specific Method
            {
                If ((Arg0 == ToUUID ("e5c937d0-3553-4d7a-9117-ea4d19c3434d") /* Device Labeling Interface */))
                {
                    While (One)
                    {
                        Name (_T_0, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                        _T_0 = ToInteger (Arg2)
                        If ((_T_0 == Zero))
                        {
                            Return (Buffer (0x02)
                            {
                                 0xFF, 0x03                                       // ..
                            })
                        }
                        ElseIf ((_T_0 == One))
                        {
                            Return (Package (0x02)
                            {
                                Package (One)
                                {
                                    One
                                }, 

                                Package (0x03)
                                {
                                    Zero, 
                                    One, 
                                    One
                                }
                            })
                        }
                        ElseIf ((_T_0 == 0x02))
                        {
                            Return (Package (One)
                            {
                                Package (0x04)
                                {
                                    One, 
                                    0x03, 
                                    Zero, 
                                    0x07
                                }
                            })
                        }
                        ElseIf ((_T_0 == 0x03))
                        {
                            Return (Package (One)
                            {
                                Zero
                            })
                        }
                        ElseIf ((_T_0 == 0x04))
                        {
                            Return (Package (0x02)
                            {
                                Package (One)
                                {
                                    Zero
                                }, 

                                Package (0x04)
                                {
                                    One, 
                                    0x03, 
                                    Zero, 
                                    0x07
                                }
                            })
                        }
                        ElseIf ((_T_0 == 0x05))
                        {
                            Return (Package (One)
                            {
                                One
                            })
                        }
                        ElseIf ((_T_0 == 0x06))
                        {
                            Return (Package (0x04)
                            {
                                Package (One)
                                {
                                    Zero
                                }, 

                                Package (One)
                                {
                                    Zero
                                }, 

                                Package (One)
                                {
                                    Zero
                                }, 

                                Package (One)
                                {
                                    Zero
                                }
                            })
                        }
                        ElseIf ((_T_0 == 0x07))
                        {
                            Return (Package (One)
                            {
                                One
                            })
                        }
                        ElseIf ((_T_0 == 0x08))
                        {
                            Return (Package (One)
                            {
                                One
                            })
                        }
                        ElseIf ((_T_0 == 0x09))
                        {
                            Return (Package (0x05)
                            {
                                0xFFFFFFFF, 
                                0xFFFFFFFF, 
                                0xFFFFFFFF, 
                                Zero, 
                                0xFFFFFFFF
                            })
                        }
                        Else
                        {
                        }

                        Break
                    }
                }
            }

            Name (_S0W, 0x04)  // _S0W: S0 Device Wake State
            Name (_PR0, Package (0x01)  // _PR0: Power Resources for D0
            {
                \_SB.P0RR, 
            })
            Name (_PR3, Package (0x01)  // _PR3: Power Resources for D3hot
            {
                \_SB.P0RR, 
            })
            Device (RP1)
            {
                Method (_ADR, 0, Serialized)  // _ADR: Address
                {
                    Return (Zero)
                }

                Name (_PR0, Package (0x01)  // _PR0: Power Resources for D0
                {
                    \_SB.R0RR, 
                })
                Name (_PR3, Package (0x01)  // _PR3: Power Resources for D3hot
                {
                    \_SB.R0RR, 
                })
                Name (_PRR, Package (0x01)  // _PRR: Power Resource for Reset
                {
                    \_SB.R0RR, 
                })
                Name (_S0W, 0x04)  // _S0W: S0 Device Wake State
                Name (_DSD, Package (0x02)  // _DSD: Device-Specific Data
                {
                    ToUUID ("6211e2c0-58a3-4af3-90e1-927a4e0c55a4") /* Unknown UUID */, 
                    Package (0x01)
                    {
                        Package (0x02)
                        {
                            "HotPlugSupportInD3", 
                            One
                        }
                    }
                })
                Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
                {
                    Name (RBUF, Buffer (0x25)
                    {
                        /* 0000 */  0x8C, 0x20, 0x00, 0x01, 0x00, 0x01, 0x00, 0x13,  // . ......
                        /* 0008 */  0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00,  // ........
                        /* 0010 */  0x00, 0x19, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00,  // ...#....
                        /* 0018 */  0x02, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x47, 0x49,  // .\_SB.GI
                        /* 0020 */  0x4F, 0x30, 0x00, 0x79, 0x00                     // O0.y.
                    })
                    Return (RBUF) /* \_SB_.PCI0.RP1_._CRS.RBUF */
                }

                Method (_DSM, 4, NotSerialized)  // _DSM: Device-Specific Method
                {
                    If ((Arg0 == ToUUID ("e5c937d0-3553-4d7a-9117-ea4d19c3434d") /* Device Labeling Interface */))
                    {
                        While (One)
                        {
                            Name (_T_0, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_0 = ToInteger (Arg2)
                            If ((_T_0 == Zero))
                            {
                                Return (Buffer (0x02)
                                {
                                     0x01, 0x03                                       // ..
                                })
                            }
                            ElseIf ((_T_0 == 0x08))
                            {
                                Return (Package (One)
                                {
                                    One
                                })
                            }
                            ElseIf ((_T_0 == 0x09))
                            {
                                Return (Package (0x05)
                                {
                                    0xFFFFFFFF, 
                                    0xFFFFFFFF, 
                                    0xFFFFFFFF, 
                                    Zero, 
                                    0xFFFFFFFF
                                })
                            }
                            Else
                            {
                            }

                            Break
                        }
                    }
                }
            }
        }

        PowerResource (P0RR, 0x05, 0x0000)
        {
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (Zero)
            }

            Method (_ON, 0, NotSerialized)  // _ON_: Power On
            {
            }

            Method (_OFF, 0, NotSerialized)  // _OFF: Power Off
            {
            }
        }

        PowerResource (R0RR, 0x05, 0x0000)
        {
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (Zero)
            }

            Method (_ON, 0, NotSerialized)  // _ON_: Power On
            {
            }

            Method (_OFF, 0, NotSerialized)  // _OFF: Power Off
            {
            }

            Method (_RST, 0, NotSerialized)  // _RST: Device Reset
            {
            }
        }

        Device (PCI1)
        {
            Name (_DEP, Package (0x02)  // _DEP: Dependencies
            {
                \_SB.PEP0, , 
                \_SB.QPPX, 
            })
            Name (_HID, EisaId ("PNP0A08") /* PCI Express Bus */)  // _HID: Hardware ID
            Name (_CID, EisaId ("PNP0A03") /* PCI Bus */)  // _CID: Compatible ID
            Name (_UID, One)  // _UID: Unique ID
            Name (_SEG, One)  // _SEG: PCI Segment
            Name (_BBN, Zero)  // _BBN: BIOS Bus Number
            Name (_PRT, Package (0x04)  // _PRT: PCI Routing Table
            {
                Package (0x04)
                {
                    0xFFFF, 
                    Zero, 
                    Zero, 
                    0x01D2
                }, 

                Package (0x04)
                {
                    0xFFFF, 
                    One, 
                    Zero, 
                    0x01D3
                }, 

                Package (0x04)
                {
                    0xFFFF, 
                    0x02, 
                    Zero, 
                    0x01D6
                }, 

                Package (0x04)
                {
                    0xFFFF, 
                    0x03, 
                    Zero, 
                    0x01D7
                }
            })
            Method (_CCA, 0, NotSerialized)  // _CCA: Cache Coherency Attribute
            {
                Return (One)
            }

            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If ((PRP1 == One))
                {
                    Return (0x0F)
                }
                Else
                {
                    Return (Zero)
                }
            }

            Method (_PSC, 0, NotSerialized)  // _PSC: Power State Current
            {
                Return (Zero)
            }

            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x2C)
                {
                    /* 0000 */  0x87, 0x17, 0x00, 0x00, 0x0C, 0x01, 0x00, 0x00,  // ........
                    /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x10, 0x50, 0xFF, 0xFF,  // .....P..
                    /* 0010 */  0xFF, 0x5F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ._......
                    /* 0018 */  0xF0, 0x0F, 0x88, 0x0D, 0x00, 0x02, 0x0C, 0x00,  // ........
                    /* 0020 */  0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,  // ........
                    /* 0028 */  0x00, 0x01, 0x79, 0x00                           // ..y.
                })
                Return (RBUF) /* \_SB_.PCI1._CRS.RBUF */
            }

            Name (SUPP, Zero)
            Name (CTRL, Zero)
            Method (_DSW, 3, NotSerialized)  // _DSW: Device Sleep Wake
            {
            }

            Method (_OSC, 4, NotSerialized)  // _OSC: Operating System Capabilities
            {
                If ((Arg0 == ToUUID ("33db4d5b-1ff7-401c-9657-7441c03dd766") /* PCI Host Bridge Device */))
                {
                    CreateDWordField (Arg3, Zero, CDW1)
                    CreateDWordField (Arg3, 0x04, CDW2)
                    CreateDWordField (Arg3, 0x08, CDW3)
                    SUPP = CDW2 /* \_SB_.PCI1._OSC.CDW2 */
                    CTRL = CDW3 /* \_SB_.PCI1._OSC.CDW3 */
                    If (((SUPP & 0x16) != 0x16))
                    {
                        CTRL &= 0x1E
                    }

                    CTRL &= 0x15
                    If ((Arg1 != One))
                    {
                        CDW1 |= 0x08
                    }

                    If ((CDW3 != CTRL))
                    {
                        CDW1 |= 0x10
                    }

                    CDW3 = CTRL /* \_SB_.PCI1.CTRL */
                    Return (Arg3)
                }
                Else
                {
                    CDW1 |= 0x04
                    Return (Arg3)
                }
            }

            Method (_DSM, 4, NotSerialized)  // _DSM: Device-Specific Method
            {
                If ((Arg0 == ToUUID ("e5c937d0-3553-4d7a-9117-ea4d19c3434d") /* Device Labeling Interface */))
                {
                    While (One)
                    {
                        Name (_T_0, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                        _T_0 = ToInteger (Arg2)
                        If ((_T_0 == Zero))
                        {
                            Return (Buffer (0x02)
                            {
                                 0xFF, 0x03                                       // ..
                            })
                        }
                        ElseIf ((_T_0 == One))
                        {
                            Return (Package (0x02)
                            {
                                Package (One)
                                {
                                    One
                                }, 

                                Package (0x03)
                                {
                                    Zero, 
                                    One, 
                                    One
                                }
                            })
                        }
                        ElseIf ((_T_0 == 0x02))
                        {
                            Return (Package (One)
                            {
                                Package (0x04)
                                {
                                    One, 
                                    0x03, 
                                    Zero, 
                                    0x07
                                }
                            })
                        }
                        ElseIf ((_T_0 == 0x03))
                        {
                            Return (Package (One)
                            {
                                Zero
                            })
                        }
                        ElseIf ((_T_0 == 0x04))
                        {
                            Return (Package (0x02)
                            {
                                Package (One)
                                {
                                    Zero
                                }, 

                                Package (0x04)
                                {
                                    One, 
                                    0x03, 
                                    Zero, 
                                    0x07
                                }
                            })
                        }
                        ElseIf ((_T_0 == 0x05))
                        {
                            Return (Package (One)
                            {
                                One
                            })
                        }
                        ElseIf ((_T_0 == 0x06))
                        {
                            Return (Package (0x04)
                            {
                                Package (One)
                                {
                                    Zero
                                }, 

                                Package (One)
                                {
                                    Zero
                                }, 

                                Package (One)
                                {
                                    Zero
                                }, 

                                Package (One)
                                {
                                    Zero
                                }
                            })
                        }
                        ElseIf ((_T_0 == 0x07))
                        {
                            Return (Package (One)
                            {
                                0x02
                            })
                        }
                        ElseIf ((_T_0 == 0x08))
                        {
                            Return (Package (One)
                            {
                                One
                            })
                        }
                        ElseIf ((_T_0 == 0x09))
                        {
                            Return (Package (0x05)
                            {
                                0xFFFFFFFF, 
                                0xFFFFFFFF, 
                                0xFFFFFFFF, 
                                Zero, 
                                0xFFFFFFFF
                            })
                        }
                        Else
                        {
                        }

                        Break
                    }
                }
            }

            Name (_S0W, 0x04)  // _S0W: S0 Device Wake State
            Name (_PR0, Package (0x01)  // _PR0: Power Resources for D0
            {
                \_SB.P1RR, 
            })
            Name (_PR3, Package (0x01)  // _PR3: Power Resources for D3hot
            {
                \_SB.P1RR, 
            })
            Device (RP1)
            {
                Method (_ADR, 0, Serialized)  // _ADR: Address
                {
                    Return (Zero)
                }

                Name (_PR0, Package (0x01)  // _PR0: Power Resources for D0
                {
                    \_SB.R1RR, 
                })
                Name (_PR3, Package (0x01)  // _PR3: Power Resources for D3hot
                {
                    \_SB.R1RR, 
                })
                Name (_PRR, Package (0x01)  // _PRR: Power Resource for Reset
                {
                    \_SB.R1RR, 
                })
                Name (_S0W, 0x04)  // _S0W: S0 Device Wake State
                Name (_DSD, Package (0x02)  // _DSD: Device-Specific Data
                {
                    ToUUID ("6211e2c0-58a3-4af3-90e1-927a4e0c55a4") /* Unknown UUID */, 
                    Package (0x01)
                    {
                        Package (0x02)
                        {
                            "HotPlugSupportInD3", 
                            One
                        }
                    }
                })
                Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
                {
                    Name (RBUF, Buffer (0x25)
                    {
                        /* 0000 */  0x8C, 0x20, 0x00, 0x01, 0x00, 0x01, 0x00, 0x13,  // . ......
                        /* 0008 */  0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00,  // ........
                        /* 0010 */  0x00, 0x19, 0x00, 0x23, 0x00, 0x00, 0x00, 0x03,  // ...#....
                        /* 0018 */  0x00, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x47, 0x49,  // .\_SB.GI
                        /* 0020 */  0x4F, 0x30, 0x00, 0x79, 0x00                     // O0.y.
                    })
                    Return (RBUF) /* \_SB_.PCI1.RP1_._CRS.RBUF */
                }

                Method (_DSM, 4, NotSerialized)  // _DSM: Device-Specific Method
                {
                    If ((Arg0 == ToUUID ("e5c937d0-3553-4d7a-9117-ea4d19c3434d") /* Device Labeling Interface */))
                    {
                        While (One)
                        {
                            Name (_T_0, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_0 = ToInteger (Arg2)
                            If ((_T_0 == Zero))
                            {
                                Return (Buffer (0x02)
                                {
                                     0x01, 0x03                                       // ..
                                })
                            }
                            ElseIf ((_T_0 == 0x08))
                            {
                                Return (Package (One)
                                {
                                    One
                                })
                            }
                            ElseIf ((_T_0 == 0x09))
                            {
                                Return (Package (0x05)
                                {
                                    0xFFFFFFFF, 
                                    0xFFFFFFFF, 
                                    0xFFFFFFFF, 
                                    Zero, 
                                    0xFFFFFFFF
                                })
                            }
                            Else
                            {
                            }

                            Break
                        }
                    }
                }
            }
        }

        PowerResource (P1RR, 0x05, 0x0000)
        {
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (Zero)
            }

            Method (_ON, 0, NotSerialized)  // _ON_: Power On
            {
            }

            Method (_OFF, 0, NotSerialized)  // _OFF: Power Off
            {
            }
        }

        PowerResource (R1RR, 0x05, 0x0000)
        {
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (Zero)
            }

            Method (_ON, 0, NotSerialized)  // _ON_: Power On
            {
            }

            Method (_OFF, 0, NotSerialized)  // _OFF: Power Off
            {
            }

            Method (_RST, 0, NotSerialized)  // _RST: Device Reset
            {
            }
        }

        Device (IPC0)
        {
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.GLNK, 
            })
            Name (_HID, "QCOM0A0D")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
        }

        Device (GLNK)
        {
            Name (_DEP, Package (0x02)  // _DEP: Dependencies
            {
                \_SB.IPCC, , 
                \_SB.RPEN, 
            })
            Name (_HID, "QCOM0A84")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, Zero)  // _UID: Unique ID
        }

        Device (ARPC)
        {
            Name (_DEP, Package (0x04)  // _DEP: Dependencies
            {
                \_SB.MMU0, , 
                \_SB.GLNK, , 
                \_SB.SCM0, , 
                \_SB.IMM0, 
            })
            Name (_HID, "QCOM0A5C")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
        }

        Device (ARPD)
        {
            Name (_DEP, Package (0x02)  // _DEP: Dependencies
            {
                \_SB.ADSP, , 
                \_SB.ARPC, 
            })
            Name (_HID, "QCOM0A82")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
        }

        Device (AGOD)
        {
            Name (_DEP, Package (0x02)  // _DEP: Dependencies
            {
                \_SB.ADSP, , 
                \_SB.ARPC, 
            })
            Name (_HID, "QCOM0AE5")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
        }

        Device (RFS0)
        {
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.IPC0, 
            })
            Name (_HID, "QCOM0A15")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x26)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x88, 0x88, 0x88, 0x88,  // ........
                    /* 0008 */  0x99, 0x99, 0x99, 0x99, 0x86, 0x09, 0x00, 0x01,  // ........
                    /* 0010 */  0x11, 0x11, 0x11, 0x11, 0x22, 0x22, 0x22, 0x22,  // ....""""
                    /* 0018 */  0x86, 0x09, 0x00, 0x01, 0x33, 0x33, 0x33, 0x33,  // ....3333
                    /* 0020 */  0x44, 0x44, 0x44, 0x44, 0x79, 0x00               // DDDDy.
                })
                CreateDWordField (RBUF, 0x04, RMTA)
                CreateDWordField (RBUF, 0x08, RMTL)
                CreateDWordField (RBUF, 0x10, RFMA)
                CreateDWordField (RBUF, 0x14, RFML)
                CreateDWordField (RBUF, 0x1C, RFAA)
                CreateDWordField (RBUF, 0x20, RFAL)
                RMTA = \_SB.RMTB
                RMTL = \_SB.RMTX
                RFMA = \_SB.RFMB
                RFML = \_SB.RFMS
                RFAA = \_SB.RFAB
                RFAL = \_SB.RFAS
                Return (RBUF) /* \_SB_.RFS0._CRS.RBUF */
            }
        }

        Scope (\_SB.RFS0)
        {
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0B)
            }
        }

        Device (IPA)
        {
            Name (_DEP, Package (0x07)  // _DEP: Dependencies
            {
                \_SB.PEP0, , 
                \_SB.RPEN, , 
                \_SB.TREE, , 
                \_SB.MMU0, , 
                \_SB.GLNK, , 
                \_SB.IPC0, , 
                \_SB.IMM0, 
            })
            Name (HHID, "QCOM0A6A")
            Name (PHID, "QCOM0AE6")
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, Zero)  // _UID: Unique ID
            Method (_HID, 0, NotSerialized)  // _HID: Hardware ID
            {
                If ((\_SB.UAON == Zero))
                {
                    Return (\_SB.IPA.HHID)
                }
                Else
                {
                    Return (\_SB.IPA.PHID)
                }
            }

            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Return (Buffer (0x2C)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0xE4, 0x01,  // ........
                    /* 0008 */  0x00, 0x00, 0x06, 0x00, 0x89, 0x06, 0x00, 0x03,  // ........
                    /* 0010 */  0x01, 0xAE, 0x02, 0x00, 0x00, 0x86, 0x09, 0x00,  // ........
                    /* 0018 */  0x01, 0x00, 0x00, 0xE0, 0x01, 0x00, 0x00, 0x03,  // ........
                    /* 0020 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0xD0, 0x01,  // ........
                    /* 0028 */  0x00, 0x00, 0x79, 0x00                           // ..y.
                })
            }

            Device (NTAD)
            {
                Name (_ADR, One)  // _ADR: Address
            }
        }

        Scope (\_SB.IPA)
        {
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Device (QDIG)
        {
            Name (_DEP, Package (0x02)  // _DEP: Dependencies
            {
                \_SB.GLNK, , 
                \_SB.IPC0, 
            })
            Name (_HID, "QCOM0A13")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
        }

        Device (SSM)
        {
            Name (_DEP, Package (0x02)  // _DEP: Dependencies
            {
                \_SB.GLNK, , 
                \_SB.TREE, 
            })
            Name (_HID, "QCOM0A14")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (Zero)
            }
        }

        Device (SYSM)
        {
            Name (_HID, "ACPI0010" /* Processor Container Device */)  // _HID: Hardware ID
            Name (_UID, 0x00100000)  // _UID: Unique ID
            Name (_LPI, Package (0x04)  // _LPI: Low Power Idle States
            {
                Zero, 
                0x01000000, 
                One, 
                Package (0x0A)
                {
                    0x251C, 
                    0x1770, 
                    One, 
                    0x20, 
                    Zero, 
                    Zero, 
                    0xB300, 
                    Buffer (0x11)
                    {
                        /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                        /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                        /* 0010 */  0x00                                             // .
                    }, 

                    Buffer (0x11)
                    {
                        /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                        /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                        /* 0010 */  0x00                                             // .
                    }, 

                    "platform.DRIPS"
                }
            })
            Device (CLUS)
            {
                Name (_HID, "ACPI0010" /* Processor Container Device */)  // _HID: Hardware ID
                Name (_UID, 0x10)  // _UID: Unique ID
                Name (_LPI, Package (0x05)  // _LPI: Low Power Idle States
                {
                    Zero, 
                    0x01000000, 
                    0x02, 
                    Package (0x0A)
                    {
                        0x170C, 
                        0x0BB8, 
                        Zero, 
                        Zero, 
                        Zero, 
                        Zero, 
                        0x20, 
                        Buffer (0x11)
                        {
                            /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                            /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                            /* 0010 */  0x00                                             // .
                        }, 

                        Buffer (0x11)
                        {
                            /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                            /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                            /* 0010 */  0x00                                             // .
                        }, 

                        "L3Cluster.D2"
                    }, 

                    Package (0x0A)
                    {
                        0x1770, 
                        0x0CE4, 
                        One, 
                        Zero, 
                        Zero, 
                        One, 
                        0x40, 
                        Buffer (0x11)
                        {
                            /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                            /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                            /* 0010 */  0x00                                             // .
                        }, 

                        Buffer (0x11)
                        {
                            /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                            /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                            /* 0010 */  0x00                                             // .
                        }, 

                        "L3Cluster.D4"
                    }
                })
                Device (CPU0)
                {
                    Name (_HID, "ACPI0007" /* Processor Device */)  // _HID: Hardware ID
                    Name (_UID, Zero)  // _UID: Unique ID
                    Method (_STA, 0, NotSerialized)  // _STA: Status
                    {
                        Return (0x0F)
                    }

                    Name (_LPI, Package (0x07)  // _LPI: Low Power Idle States
                    {
                        Zero, 
                        Zero, 
                        0x04, 
                        Package (0x0A)
                        {
                            Zero, 
                            Zero, 
                            One, 
                            Zero, 
                            Zero, 
                            Zero, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0xFF,  // .... ...
                                /* 0008 */  0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoSilver0.C1"
                        }, 

                        Package (0x0A)
                        {
                            0x0190, 
                            0x64, 
                            Zero, 
                            Zero, 
                            Zero, 
                            One, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x02,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoSilver0.C2"
                        }, 

                        Package (0x0A)
                        {
                            0x06EE, 
                            0x0385, 
                            One, 
                            One, 
                            Zero, 
                            One, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x03,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x79,  // ..@....y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoSilver0.C3"
                        }, 

                        Package (0x0A)
                        {
                            0x0FA1, 
                            0x0393, 
                            One, 
                            One, 
                            Zero, 
                            0x02, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x04,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x79,  // ..@....y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoSilver0.C4"
                        }
                    })
                }

                Device (CPU1)
                {
                    Name (_HID, "ACPI0007" /* Processor Device */)  // _HID: Hardware ID
                    Name (_UID, One)  // _UID: Unique ID
                    Method (_STA, 0, NotSerialized)  // _STA: Status
                    {
                        Return (0x0F)
                    }

                    Name (_LPI, Package (0x07)  // _LPI: Low Power Idle States
                    {
                        Zero, 
                        Zero, 
                        0x04, 
                        Package (0x0A)
                        {
                            Zero, 
                            Zero, 
                            One, 
                            Zero, 
                            Zero, 
                            Zero, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0xFF,  // .... ...
                                /* 0008 */  0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoSilver1.C1"
                        }, 

                        Package (0x0A)
                        {
                            0x0190, 
                            0x64, 
                            Zero, 
                            Zero, 
                            Zero, 
                            One, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x02,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoSilver1.C2"
                        }, 

                        Package (0x0A)
                        {
                            0x06EE, 
                            0x0385, 
                            One, 
                            One, 
                            Zero, 
                            One, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x03,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x79,  // ..@....y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoSilver1.C3"
                        }, 

                        Package (0x0A)
                        {
                            0x0FA1, 
                            0x0393, 
                            One, 
                            One, 
                            Zero, 
                            0x02, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x04,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x79,  // ..@....y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoSilver1.C4"
                        }
                    })
                }

                Device (CPU2)
                {
                    Name (_HID, "ACPI0007" /* Processor Device */)  // _HID: Hardware ID
                    Name (_UID, 0x02)  // _UID: Unique ID
                    Method (_STA, 0, NotSerialized)  // _STA: Status
                    {
                        Return (0x0F)
                    }

                    Name (_LPI, Package (0x07)  // _LPI: Low Power Idle States
                    {
                        Zero, 
                        Zero, 
                        0x04, 
                        Package (0x0A)
                        {
                            Zero, 
                            Zero, 
                            One, 
                            Zero, 
                            Zero, 
                            Zero, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0xFF,  // .... ...
                                /* 0008 */  0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoSilver2.C1"
                        }, 

                        Package (0x0A)
                        {
                            0x0190, 
                            0x64, 
                            Zero, 
                            Zero, 
                            Zero, 
                            One, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x02,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoSilver2.C2"
                        }, 

                        Package (0x0A)
                        {
                            0x06EE, 
                            0x0385, 
                            One, 
                            One, 
                            Zero, 
                            One, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x03,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x79,  // ..@....y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoSilver2.C3"
                        }, 

                        Package (0x0A)
                        {
                            0x0FA1, 
                            0x0393, 
                            One, 
                            One, 
                            Zero, 
                            0x02, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x04,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x79,  // ..@....y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoSilver2.C4"
                        }
                    })
                }

                Device (CPU3)
                {
                    Name (_HID, "ACPI0007" /* Processor Device */)  // _HID: Hardware ID
                    Name (_UID, 0x03)  // _UID: Unique ID
                    Method (_STA, 0, NotSerialized)  // _STA: Status
                    {
                        Return (0x0F)
                    }

                    Name (_LPI, Package (0x07)  // _LPI: Low Power Idle States
                    {
                        Zero, 
                        Zero, 
                        0x04, 
                        Package (0x0A)
                        {
                            Zero, 
                            Zero, 
                            One, 
                            Zero, 
                            Zero, 
                            Zero, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0xFF,  // .... ...
                                /* 0008 */  0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoSilver3.C1"
                        }, 

                        Package (0x0A)
                        {
                            0x0190, 
                            0x64, 
                            Zero, 
                            Zero, 
                            Zero, 
                            One, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x02,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoSilver3.C2"
                        }, 

                        Package (0x0A)
                        {
                            0x06EE, 
                            0x0385, 
                            One, 
                            One, 
                            Zero, 
                            One, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x03,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x79,  // ..@....y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoSilver3.C3"
                        }, 

                        Package (0x0A)
                        {
                            0x0FA1, 
                            0x0393, 
                            One, 
                            One, 
                            Zero, 
                            0x02, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x04,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x79,  // ..@....y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoSilver3.C4"
                        }
                    })
                }

                Device (CPU4)
                {
                    Name (_HID, "ACPI0007" /* Processor Device */)  // _HID: Hardware ID
                    Name (_UID, 0x04)  // _UID: Unique ID
                    Method (_STA, 0, NotSerialized)  // _STA: Status
                    {
                        Return (0x0F)
                    }

                    Name (_LPI, Package (0x07)  // _LPI: Low Power Idle States
                    {
                        Zero, 
                        Zero, 
                        0x04, 
                        Package (0x0A)
                        {
                            Zero, 
                            Zero, 
                            One, 
                            Zero, 
                            Zero, 
                            Zero, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0xFF,  // .... ...
                                /* 0008 */  0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoGold0.C1"
                        }, 

                        Package (0x0A)
                        {
                            0x0190, 
                            0x64, 
                            Zero, 
                            Zero, 
                            Zero, 
                            One, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x02,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoGold0.C2"
                        }, 

                        Package (0x0A)
                        {
                            0x0F0A, 
                            0x035C, 
                            One, 
                            One, 
                            Zero, 
                            One, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x03,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x79,  // ..@....y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoGold0.C3"
                        }, 

                        Package (0x0A)
                        {
                            0x0F6E, 
                            0x038E, 
                            One, 
                            One, 
                            Zero, 
                            0x02, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x04,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x79,  // ..@....y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoGold0.C4"
                        }
                    })
                }

                Device (CPU5)
                {
                    Name (_HID, "ACPI0007" /* Processor Device */)  // _HID: Hardware ID
                    Name (_UID, 0x05)  // _UID: Unique ID
                    Method (_STA, 0, NotSerialized)  // _STA: Status
                    {
                        Return (0x0F)
                    }

                    Name (_LPI, Package (0x07)  // _LPI: Low Power Idle States
                    {
                        Zero, 
                        Zero, 
                        0x04, 
                        Package (0x0A)
                        {
                            Zero, 
                            Zero, 
                            One, 
                            Zero, 
                            Zero, 
                            Zero, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0xFF,  // .... ...
                                /* 0008 */  0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoGold1.C1"
                        }, 

                        Package (0x0A)
                        {
                            0x0190, 
                            0x64, 
                            Zero, 
                            Zero, 
                            Zero, 
                            One, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x02,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoGold1.C2"
                        }, 

                        Package (0x0A)
                        {
                            0x0F0A, 
                            0x035C, 
                            One, 
                            One, 
                            Zero, 
                            One, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x03,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x79,  // ..@....y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoGold1.C3"
                        }, 

                        Package (0x0A)
                        {
                            0x0F6E, 
                            0x038E, 
                            One, 
                            One, 
                            Zero, 
                            0x02, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x04,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x79,  // ..@....y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoGold1.C4"
                        }
                    })
                }

                Device (CPU6)
                {
                    Name (_HID, "ACPI0007" /* Processor Device */)  // _HID: Hardware ID
                    Name (_UID, 0x06)  // _UID: Unique ID
                    Method (_STA, 0, NotSerialized)  // _STA: Status
                    {
                        If ((\_SB.SJTG == 0x102150E1))
                        {
                            Return (Zero)
                        }
                        Else
                        {
                            Return (0x0F)
                        }
                    }

                    Name (_LPI, Package (0x07)  // _LPI: Low Power Idle States
                    {
                        Zero, 
                        Zero, 
                        0x04, 
                        Package (0x0A)
                        {
                            Zero, 
                            Zero, 
                            One, 
                            Zero, 
                            Zero, 
                            Zero, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0xFF,  // .... ...
                                /* 0008 */  0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoGold2.C1"
                        }, 

                        Package (0x0A)
                        {
                            0x0190, 
                            0x64, 
                            Zero, 
                            Zero, 
                            Zero, 
                            One, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x02,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoGold2.C2"
                        }, 

                        Package (0x0A)
                        {
                            0x0F0A, 
                            0x035C, 
                            One, 
                            One, 
                            Zero, 
                            One, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x03,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x79,  // ..@....y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoGold2.C3"
                        }, 

                        Package (0x0A)
                        {
                            0x0F6E, 
                            0x038E, 
                            One, 
                            One, 
                            Zero, 
                            0x02, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x04,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x79,  // ..@....y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoGold2.C4"
                        }
                    })
                }

                Device (CPU7)
                {
                    Name (_HID, "ACPI0007" /* Processor Device */)  // _HID: Hardware ID
                    Name (_UID, 0x07)  // _UID: Unique ID
                    Method (_STA, 0, NotSerialized)  // _STA: Status
                    {
                        If ((\_SB.SJTG == 0x102150E1))
                        {
                            Return (Zero)
                        }
                        Else
                        {
                            Return (0x0F)
                        }
                    }

                    Name (_LPI, Package (0x07)  // _LPI: Low Power Idle States
                    {
                        Zero, 
                        Zero, 
                        0x04, 
                        Package (0x0A)
                        {
                            Zero, 
                            Zero, 
                            One, 
                            Zero, 
                            Zero, 
                            Zero, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0xFF,  // .... ...
                                /* 0008 */  0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoPrime0.C1"
                        }, 

                        Package (0x0A)
                        {
                            0x0190, 
                            0x64, 
                            Zero, 
                            Zero, 
                            Zero, 
                            One, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x02,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoPrime0.C2"
                        }, 

                        Package (0x0A)
                        {
                            0x0F96, 
                            0x03E8, 
                            One, 
                            One, 
                            Zero, 
                            One, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x03,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x79,  // ..@....y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoPrime0.C3"
                        }, 

                        Package (0x0A)
                        {
                            0x118A, 
                            0x05DC, 
                            One, 
                            One, 
                            Zero, 
                            0x02, 
                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x7F, 0x20, 0x00, 0x03, 0x04,  // .... ...
                                /* 0008 */  0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x79,  // ..@....y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            Buffer (0x11)
                            {
                                /* 0000 */  0x82, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // ........
                                /* 0008 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79,  // .......y
                                /* 0010 */  0x00                                             // .
                            }, 

                            "KryoPrime0.C4"
                        }
                    })
                }
            }
        }

        Device (QDCI)
        {
            Name (_DEP, Package (0x02)  // _DEP: Dependencies
            {
                \_SB.GLNK, , 
                \_SB.IPC0, 
            })
            Name (_HID, "QCOM0A12")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
        }

        Device (GPS)
        {
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.GLNK, 
            })
            Name (_HID, "QCOM0A6C")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, Zero)  // _UID: Unique ID
        }

        Scope (\_SB.GPS)
        {
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Device (QGP0)
        {
            Name (_HID, "QCOM0A88")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_CCA, Zero)  // _CCA: Cache Coherency Attribute
            Method (_CRS, 0, Serialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x20)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x40, 0x90, 0x00,  // .....@..
                    /* 0008 */  0x00, 0x00, 0x05, 0x00, 0x89, 0x06, 0x00, 0x01,  // ........
                    /* 0010 */  0x01, 0x14, 0x01, 0x00, 0x00, 0x89, 0x06, 0x00,  // ........
                    /* 0018 */  0x01, 0x01, 0x15, 0x01, 0x00, 0x00, 0x79, 0x00   // ......y.
                })
                Return (RBUF) /* \_SB_.QGP0._CRS.RBUF */
            }
        }

        Device (QGP1)
        {
            Name (_HID, "QCOM0A88")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, One)  // _UID: Unique ID
            Name (_CCA, Zero)  // _CCA: Cache Coherency Attribute
            Method (_CRS, 0, Serialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x20)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x40, 0xA0, 0x00,  // .....@..
                    /* 0008 */  0x00, 0x00, 0x05, 0x00, 0x89, 0x06, 0x00, 0x01,  // ........
                    /* 0010 */  0x01, 0x37, 0x01, 0x00, 0x00, 0x89, 0x06, 0x00,  // .7......
                    /* 0018 */  0x01, 0x01, 0x38, 0x01, 0x00, 0x00, 0x79, 0x00   // ..8...y.
                })
                Return (RBUF) /* \_SB_.QGP1._CRS.RBUF */
            }
        }

        Device (SOCP)
        {
            Name (_HID, "QCOM06DD")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Alias (\_SB.STOR, STOR)
        }

        Device (QDSS)
        {
            Name (_DEP, Package (0x02)  // _DEP: Dependencies
            {
                \_SB.PEP0, , 
                \_SB.MMU0, 
            })
            Name (_HID, "QCOM0A56")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, Zero)  // _UID: Unique ID
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0B)
            }

            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Return (Buffer (0x38)
                {
                    /* 0000 */  0x89, 0x06, 0x00, 0x03, 0x01, 0x2E, 0x01, 0x00,  // ........
                    /* 0008 */  0x00, 0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0x00,  // ........
                    /* 0010 */  0x06, 0x00, 0xA0, 0x04, 0x00, 0x86, 0x09, 0x00,  // ........
                    /* 0018 */  0x01, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00,  // ........
                    /* 0020 */  0x01, 0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0x00,  // ........
                    /* 0028 */  0x07, 0x00, 0x00, 0xA0, 0x00, 0x89, 0x06, 0x00,  // ........
                    /* 0030 */  0x03, 0x01, 0x43, 0x00, 0x00, 0x00, 0x79, 0x00   // ..C...y.
                })
            }
        }

        Device (QCDB)
        {
            Name (_HID, "QCOM06DE")  // _HID: Hardware ID
            Method (_SUB, 0, NotSerialized)  // _SUB: Subsystem ID
            {
                Return ("AGN00000")
            }

            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }
        }

        Device (RMNT)
        {
            Name (_HID, "QCOM0A95")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
        }

        Device (MBRG)
        {
            Name (_HID, "QCOM0A07")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
        }

        Device (RMAT)
        {
            Name (_HID, "QCOM0A08")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
        }

        Device (DPLB)
        {
            Name (_HID, "QCOM0A70")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
        }

        Device (CCID)
        {
            Name (_HID, "QCOM0AA2")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
        }

        Device (DSBY)
        {
            Name (_HID, "QCOM06CD")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
        }

        Scope (\_SB.RMNT)
        {
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Scope (\_SB.MBRG)
        {
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Scope (\_SB.RMAT)
        {
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Scope (\_SB.DPLB)
        {
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Scope (\_SB.CCID)
        {
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Scope (\_SB.DSBY)
        {
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (Zero)
            }
        }

        Device (SSVC)
        {
            Name (_DEP, Package (0x02)  // _DEP: Dependencies
            {
                \_SB.IPC0, , 
                \_SB.QDIG, 
            })
            Name (_HID, "QCOM06DB")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_CID, "ACPIQCOM06DB")  // _CID: Compatible ID
            Name (_UID, Zero)  // _UID: Unique ID
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }
        }

        Name (HWNL, One)
        Device (HWN0)
        {
            Name (_HID, "QCOM0A68")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Alias (\_SB.PSUB, _SUB)
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If ((\_SB.HWNL == Zero))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (Zero)
                }
            }

            Method (HWNL, 0, NotSerialized)
            {
                Name (CFG0, Package (0x10)
                {
                    0x02, 
                    0x03, 
                    0x019B, 
                    0x14, 
                    Zero, 
                    Zero, 
                    One, 
                    One, 
                    0x02, 
                    0x02, 
                    One, 
                    One, 
                    One, 
                    0x03, 
                    0x06, 
                    One
                })
                Return (CFG0) /* \_SB_.HWN0.HWNL.CFG0 */
            }
        }

        Device (CAMP)
        {
            Name (_DEP, Package (0x04)  // _DEP: Dependencies
            {
                \_SB.PEP0, , 
                \_SB.PMIC, , 
                \_SB.ARPC, , 
                \_SB.NSP0, 
            })
            Name (_HID, "QCOM0A32")  // _HID: Hardware ID
            Name (_UID, 0x1B)  // _UID: Unique ID
            Method (_SUB, 0, NotSerialized)  // _SUB: Subsystem ID
            {
                If (((((\_SB.SKUV == 0x1F) || (\_SB.SKUV == 0x20)) || (\_SB.SKUV == 
                    0x21)) || (\_SB.SKUV == 0x22)))
                {
                    Return ("IOT06490")
                }
                Else
                {
                    Return ("IOT16490")
                }
            }

            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If ((((\_SB.PLST == 0x02) || (\_SB.PLST == 0x03)) || (\_SB.PLST == 0x07)))
                {
                    Return (0x0F)
                }
                Else
                {
                    Return (Zero)
                }
            }

            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x4D)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0xC4, 0x0A,  // ........
                    /* 0008 */  0x00, 0x10, 0x00, 0x00, 0x86, 0x09, 0x00, 0x01,  // ........
                    /* 0010 */  0x00, 0xF0, 0xC9, 0x0A, 0x00, 0x80, 0x00, 0x00,  // ........
                    /* 0018 */  0x86, 0x09, 0x00, 0x01, 0x00, 0xA0, 0xC4, 0x0A,  // ........
                    /* 0020 */  0x00, 0x10, 0x00, 0x00, 0x86, 0x09, 0x00, 0x01,  // ........
                    /* 0028 */  0x00, 0xB0, 0xC4, 0x0A, 0x00, 0x10, 0x00, 0x00,  // ........
                    /* 0030 */  0x89, 0x06, 0x00, 0x03, 0x01, 0xEC, 0x01, 0x00,  // ........
                    /* 0038 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0x2F, 0x01,  // ....../.
                    /* 0040 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0xEB,  // ........
                    /* 0048 */  0x01, 0x00, 0x00, 0x79, 0x00                     // ...y.
                })
                Return (RBUF) /* \_SB_.CAMP._CRS.RBUF */
            }
        }

        Device (CAMS)
        {
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.MPCS, 
            })
            Name (_HID, "QCOM0A26")  // _HID: Hardware ID
            Name (_UID, 0x15)  // _UID: Unique ID
            Method (_SUB, 0, NotSerialized)  // _SUB: Subsystem ID
            {
                If (((((\_SB.SKUV == 0x1F) || (\_SB.SKUV == 0x20)) || (\_SB.SKUV == 
                    0x21)) || (\_SB.SKUV == 0x22)))
                {
                    Return ("IOT06490")
                }
                Else
                {
                    Return ("IOT16490")
                }
            }

            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((((\_SB.SKUV == 0x1F) || (\_SB.SKUV == 0x20)) || (\_SB.SKUV == 
                    0x21)) || (\_SB.SKUV == 0x22)))
                {
                    Return (0x0F)
                }
                Else
                {
                    Return (Zero)
                }
            }
        }

        Device (CAMF)
        {
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.MPCS, 
            })
            Name (_HID, "QCOM0A06")  // _HID: Hardware ID
            Name (_UID, 0x1A)  // _UID: Unique ID
            Method (_SUB, 0, NotSerialized)  // _SUB: Subsystem ID
            {
                If (((((\_SB.SKUV == 0x1F) || (\_SB.SKUV == 0x20)) || (\_SB.SKUV == 
                    0x21)) || (\_SB.SKUV == 0x22)))
                {
                    Return ("IOT06490")
                }
                Else
                {
                    Return ("IOT16490")
                }
            }

            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }
        }

        Device (CAMI)
        {
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.MPCS, 
            })
            Name (_HID, "QCOM0A99")  // _HID: Hardware ID
            Name (_UID, 0x1C)  // _UID: Unique ID
            Method (_SUB, 0, NotSerialized)  // _SUB: Subsystem ID
            {
                If (((((\_SB.SKUV == 0x1F) || (\_SB.SKUV == 0x20)) || (\_SB.SKUV == 
                    0x21)) || (\_SB.SKUV == 0x22)))
                {
                    Return ("IOT06490")
                }
                Else
                {
                    Return ("IOT16490")
                }
            }

            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((((\_SB.SKUV == 0x1F) || (\_SB.SKUV == 0x20)) || (\_SB.SKUV == 
                    0x21)) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Device (CAMT)
        {
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.MPCS, 
            })
            Name (_HID, "QCOM0ACE")  // _HID: Hardware ID
            Name (_UID, 0x1D)  // _UID: Unique ID
            Alias (\_SB.PSUB, _SUB)
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((((\_SB.SKUV == 0x1F) || (\_SB.SKUV == 0x20)) || (\_SB.SKUV == 
                    0x21)) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (Zero)
                }
            }
        }

        Device (CAMU)
        {
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.MPCS, 
            })
            Name (_HID, "QCOM0ACF")  // _HID: Hardware ID
            Name (_UID, 0x1E)  // _UID: Unique ID
            Alias (\_SB.PSUB, _SUB)
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((((\_SB.SKUV == 0x1F) || (\_SB.SKUV == 0x20)) || (\_SB.SKUV == 
                    0x21)) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (Zero)
                }
            }
        }

        Device (FLSH)
        {
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.CAMP, 
            })
            Name (_HID, "QCOM0A27")  // _HID: Hardware ID
            Name (_UID, 0x19)  // _UID: Unique ID
            Method (_SUB, 0, NotSerialized)  // _SUB: Subsystem ID
            {
                If (((((\_SB.SKUV == 0x1F) || (\_SB.SKUV == 0x20)) || (\_SB.SKUV == 
                    0x21)) || (\_SB.SKUV == 0x22)))
                {
                    Return ("IOT06490")
                }
                Else
                {
                    Return ("IOT16490")
                }
            }

            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (Zero)
            }

            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x02)
                {
                     0x79, 0x00                                       // y.
                })
                Return (RBUF) /* \_SB_.FLSH._CRS.RBUF */
            }
        }

        Device (MPCS)
        {
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.CAMP, 
            })
            Name (_HID, "QCOM0A98")  // _HID: Hardware ID
            Name (_UID, 0x18)  // _UID: Unique ID
            Alias (\_SB.PSUB, _SUB)
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }

            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x6B)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0xCE, 0x0A,  // ........
                    /* 0008 */  0x00, 0x20, 0x00, 0x00, 0x86, 0x09, 0x00, 0x01,  // . ......
                    /* 0010 */  0x00, 0x20, 0xCE, 0x0A, 0x00, 0x20, 0x00, 0x00,  // . ... ..
                    /* 0018 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x40, 0xCE, 0x0A,  // .....@..
                    /* 0020 */  0x00, 0x20, 0x00, 0x00, 0x86, 0x09, 0x00, 0x01,  // . ......
                    /* 0028 */  0x00, 0x60, 0xCE, 0x0A, 0x00, 0x20, 0x00, 0x00,  // .`... ..
                    /* 0030 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x80, 0xCE, 0x0A,  // ........
                    /* 0038 */  0x00, 0x20, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03,  // . ......
                    /* 0040 */  0x01, 0xFD, 0x01, 0x00, 0x00, 0x89, 0x06, 0x00,  // ........
                    /* 0048 */  0x03, 0x01, 0xFE, 0x01, 0x00, 0x00, 0x89, 0x06,  // ........
                    /* 0050 */  0x00, 0x03, 0x01, 0xFF, 0x01, 0x00, 0x00, 0x89,  // ........
                    /* 0058 */  0x06, 0x00, 0x03, 0x01, 0xE0, 0x01, 0x00, 0x00,  // ........
                    /* 0060 */  0x89, 0x06, 0x00, 0x03, 0x01, 0x9A, 0x00, 0x00,  // ........
                    /* 0068 */  0x00, 0x79, 0x00                                 // .y.
                })
                Return (RBUF) /* \_SB_.MPCS._CRS.RBUF */
            }
        }

        Device (JPGE)
        {
            Name (_DEP, Package (0x02)  // _DEP: Dependencies
            {
                \_SB.CAMP, , 
                \_SB.MMU0, 
            })
            Name (_HID, "QCOM0A33")  // _HID: Hardware ID
            Name (_UID, 0x17)  // _UID: Unique ID
            Alias (\_SB.PSUB, _SUB)
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }

            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x2C)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0xE0, 0xC4, 0x0A,  // ........
                    /* 0008 */  0x00, 0x40, 0x00, 0x00, 0x86, 0x09, 0x00, 0x01,  // .@......
                    /* 0010 */  0x00, 0x20, 0xC5, 0x0A, 0x00, 0x40, 0x00, 0x00,  // . ...@..
                    /* 0018 */  0x89, 0x06, 0x00, 0x03, 0x01, 0xFA, 0x01, 0x00,  // ........
                    /* 0020 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0xFB, 0x01,  // ........
                    /* 0028 */  0x00, 0x00, 0x79, 0x00                           // ..y.
                })
                Return (RBUF) /* \_SB_.JPGE._CRS.RBUF */
            }
        }

        Device (VFE0)
        {
            Name (_DEP, Package (0x03)  // _DEP: Dependencies
            {
                \_SB.MMU0, , 
                \_SB.PEP0, , 
                \_SB.CAMP, 
            })
            Name (_HID, "QCOM0A25")  // _HID: Hardware ID
            Name (_UID, 0x16)  // _UID: Unique ID
            Alias (\_SB.PSUB, _SUB)
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }

            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x0116)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0xC0, 0x0A,  // ........
                    /* 0008 */  0x00, 0x00, 0x02, 0x00, 0x86, 0x09, 0x00, 0x01,  // ........
                    /* 0010 */  0x00, 0x80, 0xC4, 0x0A, 0x00, 0x02, 0x00, 0x00,  // ........
                    /* 0018 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x82, 0xC4, 0x0A,  // ........
                    /* 0020 */  0x00, 0x02, 0x00, 0x00, 0x86, 0x09, 0x00, 0x01,  // ........
                    /* 0028 */  0x00, 0x84, 0xC4, 0x0A, 0x00, 0x02, 0x00, 0x00,  // ........
                    /* 0030 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x86, 0xC4, 0x0A,  // ........
                    /* 0038 */  0x00, 0x02, 0x00, 0x00, 0x86, 0x09, 0x00, 0x01,  // ........
                    /* 0040 */  0x00, 0xB0, 0xC6, 0x0A, 0x00, 0x10, 0x00, 0x00,  // ........
                    /* 0048 */  0x86, 0x09, 0x00, 0x00, 0x00, 0xF0, 0xC6, 0x0A,  // ........
                    /* 0050 */  0x00, 0x80, 0x00, 0x00, 0x86, 0x09, 0x00, 0x00,  // ........
                    /* 0058 */  0x00, 0x70, 0xC8, 0x0A, 0x00, 0xA0, 0x00, 0x00,  // .p......
                    /* 0060 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x40, 0xCC, 0x0A,  // .....@..
                    /* 0068 */  0x00, 0x50, 0x00, 0x00, 0x86, 0x09, 0x00, 0x01,  // .P......
                    /* 0070 */  0x00, 0xB0, 0xCC, 0x0A, 0x00, 0x50, 0x00, 0x00,  // .....P..
                    /* 0078 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0x30, 0x84,  // ......0.
                    /* 0080 */  0x00, 0x00, 0x50, 0x00, 0x89, 0x06, 0x00, 0x03,  // ..P.....
                    /* 0088 */  0x01, 0xED, 0x01, 0x00, 0x00, 0x89, 0x06, 0x00,  // ........
                    /* 0090 */  0x03, 0x01, 0xEE, 0x01, 0x00, 0x00, 0x89, 0x06,  // ........
                    /* 0098 */  0x00, 0x03, 0x01, 0xE3, 0x01, 0x00, 0x00, 0x89,  // ........
                    /* 00A0 */  0x06, 0x00, 0x03, 0x01, 0x99, 0x01, 0x00, 0x00,  // ........
                    /* 00A8 */  0x89, 0x06, 0x00, 0x03, 0x01, 0xEF, 0x01, 0x00,  // ........
                    /* 00B0 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0xF1, 0x01,  // ........
                    /* 00B8 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0xF3,  // ........
                    /* 00C0 */  0x01, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01,  // ........
                    /* 00C8 */  0xA1, 0x02, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03,  // ........
                    /* 00D0 */  0x01, 0xF5, 0x01, 0x00, 0x00, 0x89, 0x06, 0x00,  // ........
                    /* 00D8 */  0x03, 0x01, 0x88, 0x01, 0x00, 0x00, 0x89, 0x06,  // ........
                    /* 00E0 */  0x00, 0x03, 0x01, 0xFC, 0x01, 0x00, 0x00, 0x89,  // ........
                    /* 00E8 */  0x06, 0x00, 0x03, 0x01, 0xF0, 0x01, 0x00, 0x00,  // ........
                    /* 00F0 */  0x89, 0x06, 0x00, 0x03, 0x01, 0xF2, 0x01, 0x00,  // ........
                    /* 00F8 */  0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0xA0, 0x02,  // ........
                    /* 0100 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01, 0xF4,  // ........
                    /* 0108 */  0x01, 0x00, 0x00, 0x89, 0x06, 0x00, 0x03, 0x01,  // ........
                    /* 0110 */  0x87, 0x01, 0x00, 0x00, 0x79, 0x00               // ....y.
                })
                Return (RBUF) /* \_SB_.VFE0._CRS.RBUF */
            }
        }

        Device (SEN2)
        {
            Name (_DEP, Package (0x03)  // _DEP: Dependencies
            {
                \_SB.IPC0, , 
                \_SB.ADSP, , 
                \_SB.ARPC, 
            })
            Name (_HID, "QCOM0693")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_CID, "QCOM0667")  // _CID: Compatible ID
            Name (_PLD, Package (0x01)  // _PLD: Physical Location of Device
            {
                ToPLD (
                    PLD_Revision           = 0x2,
                    PLD_IgnoreColor        = 0x1,
                    PLD_Red                = 0x0,
                    PLD_Green              = 0x0,
                    PLD_Blue               = 0x0,
                    PLD_Width              = 0x0,
                    PLD_Height             = 0x0,
                    PLD_UserVisible        = 0x0,
                    PLD_Dock               = 0x0,
                    PLD_Lid                = 0x1,
                    PLD_Panel              = "TOP",
                    PLD_VerticalPosition   = "UPPER",
                    PLD_HorizontalPosition = "LEFT",
                    PLD_Shape              = "UNKNOWN",
                    PLD_GroupOrientation   = 0x0,
                    PLD_GroupToken         = 0x0,
                    PLD_GroupPosition      = 0x0,
                    PLD_Bay                = 0x0,
                    PLD_Ejectable          = 0x0,
                    PLD_EjectRequired      = 0x0,
                    PLD_CabinetNumber      = 0x0,
                    PLD_CardCageNumber     = 0x0,
                    PLD_Reference          = 0x0,
                    PLD_Rotation           = 0x0,
                    PLD_Order              = 0x0,
                    PLD_VerticalOffset     = 0xFFFF,
                    PLD_HorizontalOffset   = 0xFFFF)

            })
        }

        Name (HPDB, Zero)
        Name (HPDS, Buffer (One)
        {
             0x00                                             // .
        })
        Name (DPPN, 0x0D)
        Name (CCST, Buffer (One)
        {
             0x02                                             // .
        })
        Name (PORT, Buffer (One)
        {
             0x02                                             // .
        })
        Name (HIRQ, Buffer (One)
        {
             0x00                                             // .
        })
        Name (USBC, Buffer (One)
        {
             0x0B                                             // .
        })
        Name (MUXC, Buffer (One)
        {
             0x00                                             // .
        })
        Device (URS0)
        {
            Name (_HID, "QCOM0A8B")  // _HID: Hardware ID
            Name (_CID, Package (0x02)  // _CID: Compatible ID
            {
                "PNP0CA1", 
                "QCOMFFE1"
            })
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_CCA, Zero)  // _CCA: Cache Coherency Attribute
            Name (_DEP, Package (0x02)  // _DEP: Dependencies
            {
                \_SB.PEP0, , 
                \_SB.UCS0, 
            })
            Name (_CRS, Buffer (0x0E)  // _CRS: Current Resource Settings
            {
                /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0x60, 0x0A,  // ......`.
                /* 0008 */  0xFF, 0xFF, 0x0F, 0x00, 0x79, 0x00               // ....y.
            })
            Device (USB0)
            {
                Name (_ADR, Zero)  // _ADR: Address
                Name (_S0W, 0x03)  // _S0W: S0 Device Wake State
                Name (_CRS, Buffer (0x2F)  // _CRS: Current Resource Settings
                {
                    /* 0000 */  0x89, 0x06, 0x00, 0x09, 0x01, 0xA5, 0x00, 0x00,  // ........
                    /* 0008 */  0x00, 0x89, 0x06, 0x00, 0x19, 0x01, 0xA2, 0x00,  // ........
                    /* 0010 */  0x00, 0x00, 0x89, 0x06, 0x00, 0x19, 0x01, 0x11,  // ........
                    /* 0018 */  0x02, 0x00, 0x00, 0x89, 0x06, 0x00, 0x1B, 0x01,  // ........
                    /* 0020 */  0x0F, 0x02, 0x00, 0x00, 0x89, 0x06, 0x00, 0x1B,  // ........
                    /* 0028 */  0x01, 0x0E, 0x02, 0x00, 0x00, 0x79, 0x00         // .....y.
                })
                Device (RHUB)
                {
                    Name (_ADR, Zero)  // _ADR: Address
                    Device (PRT1)
                    {
                        Name (_ADR, One)  // _ADR: Address
                        Name (_UPC, Package (0x04)  // _UPC: USB Port Capabilities
                        {
                            One, 
                            0x09, 
                            Zero, 
                            Zero
                        })
                        Name (_PLD, Package (0x01)  // _PLD: Physical Location of Device
                        {
                            ToPLD (
                                PLD_Revision           = 0x2,
                                PLD_IgnoreColor        = 0x1,
                                PLD_Red                = 0x0,
                                PLD_Green              = 0x0,
                                PLD_Blue               = 0x0,
                                PLD_Width              = 0x0,
                                PLD_Height             = 0x0,
                                PLD_UserVisible        = 0x1,
                                PLD_Dock               = 0x0,
                                PLD_Lid                = 0x0,
                                PLD_Panel              = "BACK",
                                PLD_VerticalPosition   = "CENTER",
                                PLD_HorizontalPosition = "LEFT",
                                PLD_Shape              = "VERTICALRECTANGLE",
                                PLD_GroupOrientation   = 0x0,
                                PLD_GroupToken         = 0x0,
                                PLD_GroupPosition      = 0x0,
                                PLD_Bay                = 0x0,
                                PLD_Ejectable          = 0x0,
                                PLD_EjectRequired      = 0x0,
                                PLD_CabinetNumber      = 0x0,
                                PLD_CardCageNumber     = 0x0,
                                PLD_Reference          = 0x0,
                                PLD_Rotation           = 0x0,
                                PLD_Order              = 0x0,
                                PLD_VerticalOffset     = 0xFFFF,
                                PLD_HorizontalOffset   = 0xFFFF)

                        })
                    }
                }

                Method (_STA, 0, NotSerialized)  // _STA: Status
                {
                    Return (0x0F)
                }

                Method (CCVL, 0, NotSerialized)
                {
                    Return (\_SB.UCS0.ECC0)
                }

                Method (_DSM, 4, NotSerialized)  // _DSM: Device-Specific Method
                {
                    While (One)
                    {
                        Name (_T_0, Buffer (0x01)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                        {
                             0x00                                             // .
                        })
                        CopyObject (ToBuffer (Arg0), _T_0) /* \_SB_.URS0.USB0._DSM._T_0 */
                        If ((_T_0 == ToUUID ("ce2ee385-00e6-48cb-9f05-2edb927c4899") /* USB Controller */))
                        {
                            While (One)
                            {
                                Name (_T_1, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                                _T_1 = ToInteger (Arg2)
                                If ((_T_1 == Zero))
                                {
                                    While (One)
                                    {
                                        Name (_T_2, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                                        _T_2 = ToInteger (Arg1)
                                        If ((_T_2 == Zero))
                                        {
                                            Return (Buffer (One)
                                            {
                                                 0x1D                                             // .
                                            })
                                            Break
                                        }
                                        Else
                                        {
                                            Return (Buffer (One)
                                            {
                                                 0x01                                             // .
                                            })
                                            Break
                                        }

                                        Break
                                    }

                                    Return (Buffer (One)
                                    {
                                         0x00                                             // .
                                    })
                                    Break
                                }
                                ElseIf ((_T_1 == 0x02))
                                {
                                    Return (Zero)
                                    Break
                                }
                                ElseIf ((_T_1 == 0x03))
                                {
                                    Return (Zero)
                                    Break
                                }
                                ElseIf ((_T_1 == 0x04))
                                {
                                    Return (0x02)
                                    Break
                                }
                                Else
                                {
                                    Return (Buffer (One)
                                    {
                                         0x00                                             // .
                                    })
                                    Break
                                }

                                Break
                            }
                        }
                        Else
                        {
                            Return (Buffer (One)
                            {
                                 0x00                                             // .
                            })
                            Break
                        }

                        Break
                    }
                }

                Method (PHYC, 0, NotSerialized)
                {
                    Name (CFG0, Package (0x00){})
                    Return (CFG0) /* \_SB_.URS0.USB0.PHYC.CFG0 */
                }
            }

            Device (UFN0)
            {
                Name (_ADR, One)  // _ADR: Address
                Name (_S0W, 0x03)  // _S0W: S0 Device Wake State
                Device (RHUB)
                {
                    Name (_ADR, Zero)  // _ADR: Address
                    Device (PRT1)
                    {
                        Name (_ADR, One)  // _ADR: Address
                        Name (_UPC, Package (0x04)  // _UPC: USB Port Capabilities
                        {
                            One, 
                            0x09, 
                            Zero, 
                            Zero
                        })
                        Name (_PLD, Package (0x01)  // _PLD: Physical Location of Device
                        {
                            ToPLD (
                                PLD_Revision           = 0x2,
                                PLD_IgnoreColor        = 0x1,
                                PLD_Red                = 0x0,
                                PLD_Green              = 0x0,
                                PLD_Blue               = 0x0,
                                PLD_Width              = 0x0,
                                PLD_Height             = 0x0,
                                PLD_UserVisible        = 0x1,
                                PLD_Dock               = 0x0,
                                PLD_Lid                = 0x0,
                                PLD_Panel              = "BACK",
                                PLD_VerticalPosition   = "CENTER",
                                PLD_HorizontalPosition = "LEFT",
                                PLD_Shape              = "VERTICALRECTANGLE",
                                PLD_GroupOrientation   = 0x0,
                                PLD_GroupToken         = 0x0,
                                PLD_GroupPosition      = 0x0,
                                PLD_Bay                = 0x0,
                                PLD_Ejectable          = 0x0,
                                PLD_EjectRequired      = 0x0,
                                PLD_CabinetNumber      = 0x0,
                                PLD_CardCageNumber     = 0x0,
                                PLD_Reference          = 0x0,
                                PLD_Rotation           = 0x0,
                                PLD_Order              = 0x0,
                                PLD_VerticalOffset     = 0xFFFF,
                                PLD_HorizontalOffset   = 0xFFFF)

                        })
                    }
                }

                Name (_CRS, Buffer (0x14)  // _CRS: Current Resource Settings
                {
                    /* 0000 */  0x89, 0x06, 0x00, 0x09, 0x01, 0xA5, 0x00, 0x00,  // ........
                    /* 0008 */  0x00, 0x89, 0x06, 0x00, 0x19, 0x01, 0xA2, 0x00,  // ........
                    /* 0010 */  0x00, 0x00, 0x79, 0x00                           // ..y.
                })
                Method (CCVL, 0, NotSerialized)
                {
                    Return (\_SB.UCS0.ECC0)
                }

                Method (_DSM, 4, NotSerialized)  // _DSM: Device-Specific Method
                {
                    While (One)
                    {
                        Name (_T_0, Buffer (0x01)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                        {
                             0x00                                             // .
                        })
                        CopyObject (ToBuffer (Arg0), _T_0) /* \_SB_.URS0.UFN0._DSM._T_0 */
                        If ((_T_0 == ToUUID ("fe56cfeb-49d5-4378-a8a2-2978dbe54ad2") /* Unknown UUID */))
                        {
                            While (One)
                            {
                                Name (_T_1, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                                _T_1 = ToInteger (Arg2)
                                If ((_T_1 == Zero))
                                {
                                    While (One)
                                    {
                                        Name (_T_2, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                                        _T_2 = ToInteger (Arg1)
                                        If ((_T_2 == Zero))
                                        {
                                            Return (Buffer (One)
                                            {
                                                 0x03                                             // .
                                            })
                                            Break
                                        }
                                        Else
                                        {
                                            Return (Buffer (One)
                                            {
                                                 0x01                                             // .
                                            })
                                            Break
                                        }

                                        Break
                                    }

                                    Return (Buffer (One)
                                    {
                                         0x00                                             // .
                                    })
                                    Break
                                }
                                ElseIf ((_T_1 == One))
                                {
                                    Return (0x20)
                                    Break
                                }
                                Else
                                {
                                    Return (Buffer (One)
                                    {
                                         0x00                                             // .
                                    })
                                    Break
                                }

                                Break
                            }
                        }
                        ElseIf ((_T_0 == ToUUID ("18de299f-9476-4fc9-b43b-8aeb713ed751") /* Unknown UUID */))
                        {
                            While (One)
                            {
                                Name (_T_3, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                                _T_3 = ToInteger (Arg2)
                                If ((_T_3 == Zero))
                                {
                                    While (One)
                                    {
                                        Name (_T_4, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                                        _T_4 = ToInteger (Arg1)
                                        If ((_T_4 == Zero))
                                        {
                                            Return (Buffer (One)
                                            {
                                                 0x03                                             // .
                                            })
                                            Break
                                        }
                                        Else
                                        {
                                            Return (Buffer (One)
                                            {
                                                 0x01                                             // .
                                            })
                                            Break
                                        }

                                        Break
                                    }

                                    Return (Buffer (One)
                                    {
                                         0x00                                             // .
                                    })
                                    Break
                                }
                                ElseIf ((_T_3 == One))
                                {
                                    Return (0x39)
                                    Break
                                }
                                Else
                                {
                                    Return (Buffer (One)
                                    {
                                         0x00                                             // .
                                    })
                                    Break
                                }

                                Break
                            }
                        }
                        Else
                        {
                            Return (Buffer (One)
                            {
                                 0x00                                             // .
                            })
                            Break
                        }

                        Break
                    }
                }

                Method (PHYC, 0, NotSerialized)
                {
                    Name (CFG0, Package (0x00){})
                    Return (CFG0) /* \_SB_.URS0.UFN0.PHYC.CFG0 */
                }
            }
        }

        Device (USB1)
        {
            Name (_HID, "QCOM0AA1")  // _HID: Hardware ID
            Name (_CID, "ACPI\\PNP0D15")  // _CID: Compatible ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, One)  // _UID: Unique ID
            Name (_CCA, Zero)  // _CCA: Cache Coherency Attribute
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.PEP0, 
            })
            Name (_S0W, 0x03)  // _S0W: S0 Device Wake State
            Name (_ADR, Zero)  // _ADR: Address
            Name (_CRS, Buffer (0x32)  // _CRS: Current Resource Settings
            {
                /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0xC0, 0x08,  // ........
                /* 0008 */  0xFF, 0xFF, 0x0F, 0x00, 0x89, 0x06, 0x00, 0x09,  // ........
                /* 0010 */  0x01, 0x12, 0x01, 0x00, 0x00, 0x89, 0x06, 0x00,  // ........
                /* 0018 */  0x19, 0x01, 0x11, 0x01, 0x00, 0x00, 0x89, 0x06,  // ........
                /* 0020 */  0x00, 0x1B, 0x01, 0x0D, 0x02, 0x00, 0x00, 0x89,  // ........
                /* 0028 */  0x06, 0x00, 0x1B, 0x01, 0x0C, 0x02, 0x00, 0x00,  // ........
                /* 0030 */  0x79, 0x00                                       // y.
            })
            Device (RHUB)
            {
                Name (_ADR, Zero)  // _ADR: Address
                Device (PRT1)
                {
                    Name (_ADR, One)  // _ADR: Address
                    Name (_UPC, Package (0x04)  // _UPC: USB Port Capabilities
                    {
                        One, 
                        0x06, 
                        Zero, 
                        Zero
                    })
                    Name (_PLD, Package (0x01)  // _PLD: Physical Location of Device
                    {
                        ToPLD (
                            PLD_Revision           = 0x2,
                            PLD_IgnoreColor        = 0x1,
                            PLD_Red                = 0x0,
                            PLD_Green              = 0x0,
                            PLD_Blue               = 0x0,
                            PLD_Width              = 0x0,
                            PLD_Height             = 0x0,
                            PLD_UserVisible        = 0x1,
                            PLD_Dock               = 0x0,
                            PLD_Lid                = 0x0,
                            PLD_Panel              = "BACK",
                            PLD_VerticalPosition   = "CENTER",
                            PLD_HorizontalPosition = "LEFT",
                            PLD_Shape              = "VERTICALRECTANGLE",
                            PLD_GroupOrientation   = 0x0,
                            PLD_GroupToken         = 0x0,
                            PLD_GroupPosition      = 0x1,
                            PLD_Bay                = 0x0,
                            PLD_Ejectable          = 0x0,
                            PLD_EjectRequired      = 0x0,
                            PLD_CabinetNumber      = 0x0,
                            PLD_CardCageNumber     = 0x0,
                            PLD_Reference          = 0x0,
                            PLD_Rotation           = 0x0,
                            PLD_Order              = 0x0,
                            PLD_VerticalOffset     = 0xFFFF,
                            PLD_HorizontalOffset   = 0xFFFF)

                    })
                }
            }

            Method (_DSM, 4, NotSerialized)  // _DSM: Device-Specific Method
            {
                While (One)
                {
                    Name (_T_0, Buffer (0x01)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                    {
                         0x00                                             // .
                    })
                    CopyObject (ToBuffer (Arg0), _T_0) /* \_SB_.USB1._DSM._T_0 */
                    If ((_T_0 == ToUUID ("ce2ee385-00e6-48cb-9f05-2edb927c4899") /* USB Controller */))
                    {
                        While (One)
                        {
                            Name (_T_1, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_1 = ToInteger (Arg2)
                            If ((_T_1 == Zero))
                            {
                                While (One)
                                {
                                    Name (_T_2, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                                    _T_2 = ToInteger (Arg1)
                                    If ((_T_2 == Zero))
                                    {
                                        Return (Buffer (One)
                                        {
                                             0x1D                                             // .
                                        })
                                        Break
                                    }
                                    Else
                                    {
                                        Return (Buffer (One)
                                        {
                                             0x01                                             // .
                                        })
                                        Break
                                    }

                                    Break
                                }

                                Return (Buffer (One)
                                {
                                     0x00                                             // .
                                })
                                Break
                            }
                            ElseIf ((_T_1 == 0x02))
                            {
                                Return (Zero)
                                Break
                            }
                            ElseIf ((_T_1 == 0x03))
                            {
                                Return (Zero)
                                Break
                            }
                            ElseIf ((_T_1 == 0x04))
                            {
                                Return (0x02)
                                Break
                            }
                            Else
                            {
                                Return (Buffer (One)
                                {
                                     0x00                                             // .
                                })
                                Break
                            }

                            Break
                        }
                    }
                    Else
                    {
                        Return (Buffer (One)
                        {
                             0x00                                             // .
                        })
                        Break
                    }

                    Break
                }
            }

            Method (PHYC, 0, NotSerialized)
            {
                Name (CFG0, Package (0x00){})
                Return (CFG0) /* \_SB_.USB1.PHYC.CFG0 */
            }
        }

        Device (UCSI)
        {
            Name (_HID, EisaId ("USBC000"))  // _HID: Hardware ID
            Name (_CID, EisaId ("PNP0CA0"))  // _CID: Compatible ID
            Name (_UID, 0x02)  // _UID: Unique ID
            Name (_DDN, "USB Type-C")  // _DDN: DOS Device Name
            Name (_DEP, Package (0x02)  // _DEP: Dependencies
            {
                \_SB.PMGK, , 
                \_SB.UCS0, 
            })
            Device (UCN0)
            {
                Name (_ADR, Zero)  // _ADR: Address
                Name (_PLD, Package (0x01)  // _PLD: Physical Location of Device
                {
                    ToPLD (
                        PLD_Revision           = 0x2,
                        PLD_IgnoreColor        = 0x1,
                        PLD_Red                = 0x0,
                        PLD_Green              = 0x0,
                        PLD_Blue               = 0x0,
                        PLD_Width              = 0x0,
                        PLD_Height             = 0x0,
                        PLD_UserVisible        = 0x1,
                        PLD_Dock               = 0x0,
                        PLD_Lid                = 0x0,
                        PLD_Panel              = "BACK",
                        PLD_VerticalPosition   = "CENTER",
                        PLD_HorizontalPosition = "LEFT",
                        PLD_Shape              = "VERTICALRECTANGLE",
                        PLD_GroupOrientation   = 0x0,
                        PLD_GroupToken         = 0x0,
                        PLD_GroupPosition      = 0x0,
                        PLD_Bay                = 0x0,
                        PLD_Ejectable          = 0x0,
                        PLD_EjectRequired      = 0x0,
                        PLD_CabinetNumber      = 0x0,
                        PLD_CardCageNumber     = 0x0,
                        PLD_Reference          = 0x0,
                        PLD_Rotation           = 0x0,
                        PLD_Order              = 0x0,
                        PLD_VerticalOffset     = 0xFFFF,
                        PLD_HorizontalOffset   = 0xFFFF)

                })
                Name (_UPC, Package (0x04)  // _UPC: USB Port Capabilities
                {
                    One, 
                    0x09, 
                    Zero, 
                    Zero
                })
            }

            Name (_CRS, Buffer (0x0E)  // _CRS: Current Resource Settings
            {
                /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA0,  // ........
                /* 0008 */  0x30, 0x00, 0x00, 0x00, 0x79, 0x00               // 0...y.
            })
            OperationRegion (USBC, SystemMemory, 0xA0000000, 0x30)
            Field (USBC, ByteAcc, Lock, Preserve)
            {
                VERS,   16, 
                RESV,   16, 
                CCI,    32, 
                CTRL,   64, 
                MSGI,   128, 
                MSGO,   128
            }

            Name (BUFF, Buffer (0x32){})
            CreateField (BUFF, Zero, 0x08, BSTA)
            CreateField (BUFF, 0x08, 0x08, BSIZ)
            CreateField (BUFF, 0x10, 0x10, BVER)
            CreateField (BUFF, 0x30, 0x20, BCCI)
            CreateField (BUFF, 0x50, 0x40, BCTL)
            CreateField (BUFF, 0x90, 0x80, BMGI)
            CreateField (BUFF, 0x0110, 0x80, BMGO)
            Method (OPMW, 0, NotSerialized)
            {
                BCTL = CTRL /* \_SB_.UCSI.CTRL */
                BMGO = MSGO /* \_SB_.UCSI.MSGO */
                \_SB.PMGK.UCSI = BUFF /* \_SB_.UCSI.BUFF */
                Return (Zero)
            }

            Method (OPMR, 0, NotSerialized)
            {
                BUFF = \_SB.PMGK.UCSI
                VERS = BVER /* \_SB_.UCSI.BVER */
                CCI = BCCI /* \_SB_.UCSI.BCCI */
                MSGI = BMGI /* \_SB_.UCSI.BMGI */
                Return (Zero)
            }

            Method (_DSM, 4, Serialized)  // _DSM: Device-Specific Method
            {
                If ((Arg0 == ToUUID ("6f8398c2-7ca4-11e4-ad36-631042b5008f") /* Unknown UUID */))
                {
                    While (One)
                    {
                        Name (_T_0, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                        _T_0 = ToInteger (Arg2)
                        If ((_T_0 == Zero))
                        {
                            Return (Buffer (One)
                            {
                                 0x0F                                             // .
                            })
                        }
                        ElseIf ((_T_0 == One))
                        {
                            Return (OPMW ())
                        }
                        ElseIf ((_T_0 == 0x02))
                        {
                            Return (OPMR ())
                        }
                        ElseIf ((_T_0 == 0x03))
                        {
                            Return (Zero)
                        }
                        ElseIf ((_T_0 == 0x04))
                        {
                            Return (One)
                        }

                        Break
                    }
                }
            }
        }

        Device (UCS0)
        {
            Name (_HID, "QCOM0AA4")  // _HID: Hardware ID
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.PEP0, 
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (Zero)
            }

            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x0E)
                {
                    /* 0000 */  0x86, 0x09, 0x00, 0x01, 0x40, 0x00, 0x00, 0xA0,  // ....@...
                    /* 0008 */  0x20, 0x00, 0x00, 0x00, 0x79, 0x00               //  ...y.
                })
                Return (RBUF) /* \_SB_.UCS0._CRS.RBUF */
            }

            OperationRegion (USBC, SystemMemory, 0xA0000040, 0x20)
            Field (USBC, ByteAcc, Lock, Preserve)
            {
                INFO,   8, 
                UPDT,   8, 
                CCX0,   8, 
                MUX0,   8, 
                RES0,   8, 
                VID0,   16, 
                SID0,   16, 
                SVS0,   8
            }

            Name (PORT, Buffer (0x20){})
            CreateByteField (PORT, Zero, EINF)
            CreateByteField (PORT, One, EUPD)
            CreateByteField (PORT, 0x02, ECC0)
            CreateByteField (PORT, 0x03, EMX0)
            CreateWordField (PORT, 0x05, EVI0)
            CreateWordField (PORT, 0x07, ESI0)
            CreateQWordField (PORT, 0x09, ESV0)
            Method (USBW, 0, NotSerialized)
            {
                EUPD = UPDT /* \_SB_.UCS0.UPDT */
                Notify (\_SB.PMGK, 0xF0) // Hardware-Specific
                Return (Zero)
            }

            Method (USBR, 0, NotSerialized)
            {
                INFO = One
                UPDT = EUPD /* \_SB_.UCS0.EUPD */
                CCX0 = ECC0 /* \_SB_.UCS0.ECC0 */
                MUX0 = EMX0 /* \_SB_.UCS0.EMX0 */
                RES0 = Zero
                VID0 = EVI0 /* \_SB_.UCS0.EVI0 */
                SID0 = ESI0 /* \_SB_.UCS0.ESI0 */
                SVS0 = ESV0 /* \_SB_.UCS0.ESV0 */
                Notify (UCS0, 0xA0) // Device-Specific
                Return (Zero)
            }

            Method (MUXV, 0, NotSerialized)
            {
                \_SB.MUXC = EMX0 /* \_SB_.UCS0.EMX0 */
                Return (\_SB.MUXC)
            }

            Method (CCVL, 0, NotSerialized)
            {
                \_SB.CCST = ECC0 /* \_SB_.UCS0.ECC0 */
                Return (\_SB.CCST)
            }

            Method (DPVL, 0, NotSerialized)
            {
                \_SB.DPPN = (ESV0 & 0x3F)
                Return (\_SB.DPPN)
            }

            Method (HPDM, 0, NotSerialized)
            {
                \_SB.HPDS = ((ESV0 & 0x40) >> 0x06)
                Return (\_SB.HPDS)
            }

            Method (HPDI, 0, NotSerialized)
            {
                \_SB.HIRQ = ((ESV0 & 0x80) >> 0x07)
                Return (\_SB.HIRQ)
            }
        }

        Device (AGR0)
        {
            Name (_HID, "ACPI000C" /* Processor Aggregator Device */)  // _HID: Hardware ID
            Name (_PUR, Package (0x02)  // _PUR: Processor Utilization Request
            {
                One, 
                Zero
            })
            Method (_OST, 3, NotSerialized)  // _OST: OSPM Status Indication
            {
                \_SB.PEP0.ROST = Arg2
            }
        }

        ThermalZone (TZ0)
        {
            Name (_HID, "QCOM0A58")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.PEP0, 
                })
            }

            Name (TTSP, One)
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ0.TTSP)
            }
        }

        ThermalZone (TZ1)
        {
            Name (_HID, "QCOM0A58")  // _HID: Hardware ID
            Name (_UID, One)  // _UID: Unique ID
            Name (_TZD, Package (0x01)  // _TZD: Thermal Zone Devices
            {
                \_SB.PEP0, 
            })
            Name (TPSV, 0x0EC4)
            Method (_PSV, 0, NotSerialized)  // _PSV: Passive Temperature
            {
                Return (\_SB.TZ1.TPSV)
            }

            Name (_MTL, 0x14)  // _MTL: Minimum Throttle Limit
            Name (TTC1, Zero)
            Method (_TC1, 0, NotSerialized)  // _TC1: Thermal Constant 1
            {
                Return (\_SB.TZ1.TTC1)
            }

            Name (TTC2, One)
            Method (_TC2, 0, NotSerialized)  // _TC2: Thermal Constant 2
            {
                Return (\_SB.TZ1.TTC2)
            }

            Name (TTSP, One)
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ1.TTSP)
            }

            Name (_TZP, Zero)  // _TZP: Thermal Zone Polling
            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.PEP0, 
                })
            }
        }

        ThermalZone (TZ2)
        {
            Name (_HID, "QCOM0A59")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.PEP0, 
                })
            }

            Name (TTSP, One)
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ2.TTSP)
            }
        }

        ThermalZone (TZ3)
        {
            Name (_HID, "QCOM0A59")  // _HID: Hardware ID
            Name (_UID, One)  // _UID: Unique ID
            Name (_TZD, Package (0x01)  // _TZD: Thermal Zone Devices
            {
                \_SB.PEP0, 
            })
            Name (TPSV, 0x0EC4)
            Method (_PSV, 0, NotSerialized)  // _PSV: Passive Temperature
            {
                Return (\_SB.TZ3.TPSV)
            }

            Name (_MTL, 0x14)  // _MTL: Minimum Throttle Limit
            Name (TTC1, Zero)
            Method (_TC1, 0, NotSerialized)  // _TC1: Thermal Constant 1
            {
                Return (\_SB.TZ3.TTC1)
            }

            Name (TTC2, One)
            Method (_TC2, 0, NotSerialized)  // _TC2: Thermal Constant 2
            {
                Return (\_SB.TZ3.TTC2)
            }

            Name (TTSP, One)
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ3.TTSP)
            }

            Name (_TZP, Zero)  // _TZP: Thermal Zone Polling
            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.PEP0, 
                })
            }
        }

        ThermalZone (TZ4)
        {
            Name (_HID, "QCOM0AD4")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.PEP0, 
                })
            }

            Name (TTSP, One)
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ4.TTSP)
            }
        }

        ThermalZone (TZ5)
        {
            Name (_HID, "QCOM0AD4")  // _HID: Hardware ID
            Name (_UID, One)  // _UID: Unique ID
            Name (_TZD, Package (0x01)  // _TZD: Thermal Zone Devices
            {
                \_SB.PEP0, 
            })
            Name (TPSV, 0x0EC4)
            Method (_PSV, 0, NotSerialized)  // _PSV: Passive Temperature
            {
                Return (\_SB.TZ5.TPSV)
            }

            Name (_MTL, 0x14)  // _MTL: Minimum Throttle Limit
            Name (TTC1, Zero)
            Method (_TC1, 0, NotSerialized)  // _TC1: Thermal Constant 1
            {
                Return (\_SB.TZ5.TTC1)
            }

            Name (TTC2, One)
            Method (_TC2, 0, NotSerialized)  // _TC2: Thermal Constant 2
            {
                Return (\_SB.TZ5.TTC2)
            }

            Name (TTSP, One)
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ5.TTSP)
            }

            Name (_TZP, Zero)  // _TZP: Thermal Zone Polling
            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.PEP0, 
                })
            }
        }

        ThermalZone (TZ6)
        {
            Name (_HID, "QCOM0A91")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_TZD, Package (0x01)  // _TZD: Thermal Zone Devices
            {
                \_SB.GPU0, 
            })
            Name (PROD, Zero)
            Name (PSV1, 0x0EC4)
            Name (PSV2, 0x0E60)
            Method (_PSV, 0, NotSerialized)  // _PSV: Passive Temperature
            {
                PROD = ((\_SB.SJTG >> 0x0C) & 0xFFFF)
                If ((((PROD == 0x0197) & (\_SB.SIDT == 0x03)) | ((
                    PROD == 0x0198) & (\_SB.SIDT == 0x04))))
                {
                    Return (\_SB.TZ6.PSV1)
                }
                Else
                {
                    Return (\_SB.TZ6.PSV2)
                }
            }

            Name (TTC1, One)
            Method (_TC1, 0, NotSerialized)  // _TC1: Thermal Constant 1
            {
                Return (\_SB.TZ6.TTC1)
            }

            Name (TTC2, 0x02)
            Method (_TC2, 0, NotSerialized)  // _TC2: Thermal Constant 2
            {
                Return (\_SB.TZ6.TTC2)
            }

            Name (TTSP, 0x02)
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ6.TTSP)
            }

            Name (_TZP, Zero)  // _TZP: Thermal Zone Polling
            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.PEP0, 
                })
            }
        }

        ThermalZone (TZ7)
        {
            Name (_HID, "QCOM0A51")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.PEP0, 
                })
            }

            Name (TTSP, 0x32)
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ7.TTSP)
            }
        }

        ThermalZone (TZ9)
        {
            Name (_HID, "QCOM0A4C")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.PEP0, 
                })
            }

            Name (TTSP, 0x32)
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ9.TTSP)
            }
        }

        ThermalZone (TZ10)
        {
            Name (_HID, "QCOM0A92")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_TZD, Package (0x01)  // _TZD: Thermal Zone Devices
            {
                \_SB.MJCT, 
            })
            Name (PROD, Zero)
            Name (PSV1, 0x0EC4)
            Name (PSV2, 0x0E60)
            Method (_PSV, 0, NotSerialized)  // _PSV: Passive Temperature
            {
                PROD = ((\_SB.SJTG >> 0x0C) & 0xFFFF)
                If ((((PROD == 0x0197) & (\_SB.SIDT == 0x03)) | ((
                    PROD == 0x0198) & (\_SB.SIDT == 0x04))))
                {
                    Return (\_SB.TZ10.PSV1)
                }
                Else
                {
                    Return (\_SB.TZ10.PSV2)
                }
            }

            Name (TTC1, One)
            Method (_TC1, 0, NotSerialized)  // _TC1: Thermal Constant 1
            {
                Return (\_SB.TZ10.TTC1)
            }

            Name (TTC2, 0x02)
            Method (_TC2, 0, NotSerialized)  // _TC2: Thermal Constant 2
            {
                Return (\_SB.TZ10.TTC2)
            }

            Name (TTSP, 0x0A)
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ10.TTSP)
            }

            Name (_TZP, Zero)  // _TZP: Thermal Zone Polling
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }

            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.PEP0, 
                })
            }
        }

        ThermalZone (TZ11)
        {
            Name (_HID, "QCOM0ABF")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_TZD, Package (0x01)  // _TZD: Thermal Zone Devices
            {
                \_SB.CSW0, 
            })
            Name (TPSV, 0x0EC4)
            Method (_PSV, 0, NotSerialized)  // _PSV: Passive Temperature
            {
                Return (\_SB.TZ11.TPSV)
            }

            Name (TTC1, Zero)
            Method (_TC1, 0, NotSerialized)  // _TC1: Thermal Constant 1
            {
                Return (\_SB.TZ11.TTC1)
            }

            Name (TTC2, One)
            Method (_TC2, 0, NotSerialized)  // _TC2: Thermal Constant 2
            {
                Return (\_SB.TZ11.TTC2)
            }

            Name (TTSP, 0x32)
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ11.TTSP)
            }

            Name (_TZP, Zero)  // _TZP: Thermal Zone Polling
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If ((\_SB.SJTG == 0x102150E1))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }

            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.PEP0, 
                })
            }
        }

        ThermalZone (TZ12)
        {
            Name (_HID, "QCOM0A4B")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.PEP0, 
                })
            }

            Name (TTSP, 0x32)
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ12.TTSP)
            }
        }

        ThermalZone (TZ13)
        {
            Name (_HID, "QCOM0A57")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_TZD, Package (0x04)  // _TZD: Thermal Zone Devices
            {
                \_SB.WLTM, , 
                \_SB.CSW0, , 
                \_SB.GPU0, , 
                \_SB.MBCL, 
            })
            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x02)
                {
                    \_SB.PEP0, , 
                    \_SB.BCL1, 
                })
            }
        }

        ThermalZone (TZ15)
        {
            Name (_HID, "QCOM0AC8")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x25)
                {
                    /* 0000 */  0x8C, 0x20, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,  // . ......
                    /* 0008 */  0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00,  // ........
                    /* 0010 */  0x00, 0x19, 0x00, 0x23, 0x00, 0x00, 0x00, 0xC0,  // ...#....
                    /* 0018 */  0x00, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x50, 0x4D,  // .\_SB.PM
                    /* 0020 */  0x30, 0x31, 0x00, 0x79, 0x00                     // 01.y.
                })
                Return (RBUF) /* \_SB_.TZ15._CRS.RBUF */
            }

            Name (_TZD, Package (0x01)  // _TZD: Thermal Zone Devices
            {
                \_SB.PEP0, 
            })
            Name (PROD, Zero)
            Name (PSV1, 0x0EC4)
            Name (PSV2, 0x0E60)
            Method (_PSV, 0, NotSerialized)  // _PSV: Passive Temperature
            {
                PROD = ((\_SB.SJTG >> 0x0C) & 0xFFFF)
                If ((((PROD == 0x0197) & (\_SB.SIDT == 0x03)) | ((
                    PROD == 0x0198) & (\_SB.SIDT == 0x04))))
                {
                    Return (\_SB.TZ15.PSV1)
                }
                Else
                {
                    Return (\_SB.TZ15.PSV2)
                }
            }

            Name (TCRT, 0x0F28)
            Method (_CRT, 0, NotSerialized)  // _CRT: Critical Temperature
            {
                Return (\_SB.TZ15.TCRT)
            }

            Name (_MTL, 0x14)  // _MTL: Minimum Throttle Limit
            Name (TTC1, Zero)
            Method (_TC1, 0, NotSerialized)  // _TC1: Thermal Constant 1
            {
                Return (\_SB.TZ15.TTC1)
            }

            Name (TTC2, 0x14)
            Method (_TC2, 0, NotSerialized)  // _TC2: Thermal Constant 2
            {
                Return (\_SB.TZ15.TTC2)
            }

            Name (_TSP, One)  // _TSP: Thermal Sampling Period
            Name (_TZP, Zero)  // _TZP: Thermal Zone Polling
            Method (_DSM, 4, NotSerialized)  // _DSM: Device-Specific Method
            {
                While (One)
                {
                    Name (_T_0, Buffer (0x01)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                    {
                         0x00                                             // .
                    })
                    CopyObject (ToBuffer (Arg0), _T_0) /* \_SB_.TZ15._DSM._T_0 */
                    If ((_T_0 == ToUUID ("c2d42c4b-e25e-471c-8a4e-290aac3a29a3") /* Unknown UUID */))
                    {
                        While (One)
                        {
                            Name (_T_1, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_1 = Arg2
                            If ((_T_1 == Zero))
                            {
                                While (One)
                                {
                                    Name (_T_2, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                                    _T_2 = Arg1
                                    If ((_T_2 == Zero))
                                    {
                                        Return (Buffer (One)
                                        {
                                             0x03                                             // .
                                        })
                                    }

                                    Break
                                }

                                Return (Buffer (One)
                                {
                                     0x00                                             // .
                                })
                            }
                            ElseIf ((_T_1 == One))
                            {
                                \_SB.TZ15.TPSV = DerefOf (Arg3 [Zero])
                                \_SB.TZ15.TCRT = DerefOf (Arg3 [One])
                                \_SB.TZ15.TTC2 = DerefOf (Arg3 [0x02])
                                \_SB.TZ15.TTC1 = Zero
                                Notify (\_SB.TZ15, 0x81) // Thermal Trip Point Change
                                Return (\_SB.TZ15.TPSV) /* External reference */
                            }
                            Else
                            {
                                Return (Zero)
                            }

                            Break
                        }
                    }
                    Else
                    {
                        Return (Zero)
                    }

                    Break
                }
            }

            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x02)
                {
                    \_SB.PEP0, , 
                    \_SB.PMIC, 
                })
            }
        }

        ThermalZone (TZ16)
        {
            Name (_HID, "QCOM0AC9")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x25)
                {
                    /* 0000 */  0x8C, 0x20, 0x00, 0x01, 0x00, 0x01, 0x00, 0x09,  // . ......
                    /* 0008 */  0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00,  // ........
                    /* 0010 */  0x00, 0x19, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00,  // ...#....
                    /* 0018 */  0x01, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x50, 0x4D,  // .\_SB.PM
                    /* 0020 */  0x30, 0x31, 0x00, 0x79, 0x00                     // 01.y.
                })
                Return (RBUF) /* \_SB_.TZ16._CRS.RBUF */
            }

            Name (_TZD, Package (0x02)  // _TZD: Thermal Zone Devices
            {
                \_SB.WLTM, , 
                \_SB.MJCT, 
            })
            Name (PROD, Zero)
            Name (PSV1, 0x0EC4)
            Name (PSV2, 0x0E60)
            Method (_PSV, 0, NotSerialized)  // _PSV: Passive Temperature
            {
                PROD = ((\_SB.SJTG >> 0x0C) & 0xFFFF)
                If ((((PROD == 0x0197) & (\_SB.SIDT == 0x03)) | ((
                    PROD == 0x0198) & (\_SB.SIDT == 0x04))))
                {
                    Return (\_SB.TZ16.PSV1)
                }
                Else
                {
                    Return (\_SB.TZ16.PSV2)
                }
            }

            Name (TCRT, 0x0F28)
            Method (_CRT, 0, NotSerialized)  // _CRT: Critical Temperature
            {
                Return (\_SB.TZ16.TCRT)
            }

            Name (TTC1, Zero)
            Method (_TC1, 0, NotSerialized)  // _TC1: Thermal Constant 1
            {
                Return (\_SB.TZ16.TTC1)
            }

            Name (TTC2, 0x14)
            Method (_TC2, 0, NotSerialized)  // _TC2: Thermal Constant 2
            {
                Return (\_SB.TZ16.TTC2)
            }

            Name (_TSP, One)  // _TSP: Thermal Sampling Period
            Name (_TZP, Zero)  // _TZP: Thermal Zone Polling
            Method (_DSM, 4, NotSerialized)  // _DSM: Device-Specific Method
            {
                While (One)
                {
                    Name (_T_0, Buffer (0x01)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                    {
                         0x00                                             // .
                    })
                    CopyObject (ToBuffer (Arg0), _T_0) /* \_SB_.TZ16._DSM._T_0 */
                    If ((_T_0 == ToUUID ("c2d42c4b-e25e-471c-8a4e-290aac3a29a3") /* Unknown UUID */))
                    {
                        While (One)
                        {
                            Name (_T_1, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_1 = Arg2
                            If ((_T_1 == Zero))
                            {
                                While (One)
                                {
                                    Name (_T_2, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                                    _T_2 = Arg1
                                    If ((_T_2 == Zero))
                                    {
                                        Return (Buffer (One)
                                        {
                                             0x03                                             // .
                                        })
                                    }

                                    Break
                                }

                                Return (Buffer (One)
                                {
                                     0x00                                             // .
                                })
                            }
                            ElseIf ((_T_1 == One))
                            {
                                \_SB.TZ16.TPSV = DerefOf (Arg3 [Zero])
                                \_SB.TZ16.TCRT = DerefOf (Arg3 [One])
                                \_SB.TZ16.TTC2 = DerefOf (Arg3 [0x02])
                                \_SB.TZ16.TTC1 = Zero
                                Notify (\_SB.TZ16, 0x81) // Thermal Trip Point Change
                                Return (\_SB.TZ16.TPSV) /* External reference */
                            }
                            Else
                            {
                                Return (Zero)
                            }

                            Break
                        }
                    }
                    Else
                    {
                        Return (Zero)
                    }

                    Break
                }
            }

            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x02)
                {
                    \_SB.PEP0, , 
                    \_SB.PMIC, 
                })
            }
        }

        ThermalZone (TZ18)
        {
            Name (_HID, "QCOM0ACB")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x25)
                {
                    /* 0000 */  0x8C, 0x20, 0x00, 0x01, 0x00, 0x01, 0x00, 0x09,  // . ......
                    /* 0008 */  0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00,  // ........
                    /* 0010 */  0x00, 0x19, 0x00, 0x23, 0x00, 0x00, 0x00, 0x80,  // ...#....
                    /* 0018 */  0x01, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x50, 0x4D,  // .\_SB.PM
                    /* 0020 */  0x30, 0x31, 0x00, 0x79, 0x00                     // 01.y.
                })
                Return (RBUF) /* \_SB_.TZ18._CRS.RBUF */
            }

            Name (_TZD, Package (0x03)  // _TZD: Thermal Zone Devices
            {
                \_SB.MBS0, , 
                \_SB.MBS1, , 
                \_SB.MBS2, 
            })
            Name (PROD, Zero)
            Name (PSV1, 0x0EC4)
            Name (PSV2, 0x0E60)
            Method (_PSV, 0, NotSerialized)  // _PSV: Passive Temperature
            {
                PROD = ((\_SB.SJTG >> 0x0C) & 0xFFFF)
                If ((((PROD == 0x0197) & (\_SB.SIDT == 0x03)) | ((
                    PROD == 0x0198) & (\_SB.SIDT == 0x04))))
                {
                    Return (\_SB.TZ18.PSV1)
                }
                Else
                {
                    Return (\_SB.TZ18.PSV2)
                }
            }

            Name (TCRT, 0x0F28)
            Method (_CRT, 0, NotSerialized)  // _CRT: Critical Temperature
            {
                Return (\_SB.TZ18.TCRT)
            }

            Name (TTC1, Zero)
            Method (_TC1, 0, NotSerialized)  // _TC1: Thermal Constant 1
            {
                Return (\_SB.TZ18.TTC1)
            }

            Name (TTC2, 0x14)
            Method (_TC2, 0, NotSerialized)  // _TC2: Thermal Constant 2
            {
                Return (\_SB.TZ18.TTC2)
            }

            Name (_TSP, One)  // _TSP: Thermal Sampling Period
            Name (_TZP, Zero)  // _TZP: Thermal Zone Polling
            Method (_DSM, 4, NotSerialized)  // _DSM: Device-Specific Method
            {
                While (One)
                {
                    Name (_T_0, Buffer (0x01)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                    {
                         0x00                                             // .
                    })
                    CopyObject (ToBuffer (Arg0), _T_0) /* \_SB_.TZ18._DSM._T_0 */
                    If ((_T_0 == ToUUID ("c2d42c4b-e25e-471c-8a4e-290aac3a29a3") /* Unknown UUID */))
                    {
                        While (One)
                        {
                            Name (_T_1, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                            _T_1 = Arg2
                            If ((_T_1 == Zero))
                            {
                                While (One)
                                {
                                    Name (_T_2, 0x00)  // _T_x: Emitted by ASL Compiler, x=0-9, A-Z
                                    _T_2 = Arg1
                                    If ((_T_2 == Zero))
                                    {
                                        Return (Buffer (One)
                                        {
                                             0x03                                             // .
                                        })
                                    }

                                    Break
                                }

                                Return (Buffer (One)
                                {
                                     0x00                                             // .
                                })
                            }
                            ElseIf ((_T_1 == One))
                            {
                                \_SB.TZ18.TPSV = DerefOf (Arg3 [Zero])
                                \_SB.TZ18.TCRT = DerefOf (Arg3 [One])
                                \_SB.TZ18.TTC2 = DerefOf (Arg3 [0x02])
                                \_SB.TZ18.TTC1 = Zero
                                Notify (\_SB.TZ18, 0x81) // Thermal Trip Point Change
                                Return (\_SB.TZ18.TPSV) /* External reference */
                            }
                            Else
                            {
                                Return (Zero)
                            }

                            Break
                        }
                    }
                    Else
                    {
                        Return (Zero)
                    }

                    Break
                }
            }

            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }

            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x02)
                {
                    \_SB.PEP0, , 
                    \_SB.PMIC, 
                })
            }
        }

        ThermalZone (TZ99)
        {
            Name (_HID, "QCOM0A5A")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_TZD, Package (0x0D)  // _TZD: Thermal Zone Devices
            {
                \_SB.SYSM.CLUS.CPU0, , 
                \_SB.SYSM.CLUS.CPU1, , 
                \_SB.SYSM.CLUS.CPU2, , 
                \_SB.SYSM.CLUS.CPU3, , 
                \_SB.SYSM.CLUS.CPU4, , 
                \_SB.SYSM.CLUS.CPU5, , 
                \_SB.SYSM.CLUS.CPU6, , 
                \_SB.SYSM.CLUS.CPU7, , 
                \_SB.PEP0, , 
                \_SB.WLTM, , 
                \_SB.CSW0, , 
                \_SB.GPU0, , 
                \_SB.MJCT, 
            })
            Name (TPSV, 0x0EC4)
            Method (_PSV, 0, NotSerialized)  // _PSV: Passive Temperature
            {
                Return (\_SB.TZ99.TPSV)
            }

            Name (TCRT, 0x0F28)
            Method (_CRT, 0, NotSerialized)  // _CRT: Critical Temperature
            {
                Return (\_SB.TZ99.TCRT)
            }

            Name (_MTL, 0x14)  // _MTL: Minimum Throttle Limit
            Name (TTC1, 0x04)
            Method (_TC1, 0, NotSerialized)  // _TC1: Thermal Constant 1
            {
                Return (\_SB.TZ99.TTC1)
            }

            Name (TTC2, 0x03)
            Method (_TC2, 0, NotSerialized)  // _TC2: Thermal Constant 2
            {
                Return (\_SB.TZ99.TTC2)
            }

            Name (TTSP, 0x0A)
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ99.TTSP)
            }

            Name (_TZP, Zero)  // _TZP: Thermal Zone Polling
            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.PEP0, 
                })
            }
        }

        Device (MPA)
        {
            Name (_HID, "QCOM04B4")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.IPC0, 
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Device (MPA1)
        {
            Name (_HID, "QCOM04B5")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.IPC0, 
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Device (MBJ0)
        {
            Name (_HID, "QCOM04B6")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.IPC0, 
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Device (MBJ1)
        {
            Name (_HID, "QCOM04B7")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.IPC0, 
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Device (MBJ2)
        {
            Name (_HID, "QCOM04B8")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.IPC0, 
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Device (MBJ3)
        {
            Name (_HID, "QCOM04B9")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.IPC0, 
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Device (MBS0)
        {
            Name (_HID, "QCOM04BA")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.IPC0, 
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Device (MBS1)
        {
            Name (_HID, "QCOM04BB")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.IPC0, 
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Device (MBS2)
        {
            Name (_HID, "QCOM04BC")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.IPC0, 
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Device (MSKN)
        {
            Name (_HID, "QCOM04BE")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.IPC0, 
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Device (MJCT)
        {
            Name (_HID, "QCOM04BF")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.IPC0, 
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Device (MBCL)
        {
            Name (_HID, "QCOM06D4")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.IPC0, 
            })
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If (((\_SB.SKUV == 0x20) || (\_SB.SKUV == 0x22)))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        ThermalZone (TZ51)
        {
            Name (_HID, "QCOM04C0")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_TZD, Package (0x01)  // _TZD: Thermal Zone Devices
            {
                \_SB.MPA, 
            })
            Name (TPSV, 0x0E60)
            Method (_PSV, 0, NotSerialized)  // _PSV: Passive Temperature
            {
                Return (\_SB.TZ51.TPSV)
            }

            Name (TCRT, 0x0F5A)
            Method (_CRT, 0, NotSerialized)  // _CRT: Critical Temperature
            {
                Return (\_SB.TZ51.TCRT)
            }

            Name (TTC1, One)
            Method (_TC1, 0, NotSerialized)  // _TC1: Thermal Constant 1
            {
                Return (\_SB.TZ51.TTC1)
            }

            Name (TTC2, 0x02)
            Method (_TC2, 0, NotSerialized)  // _TC2: Thermal Constant 2
            {
                Return (\_SB.TZ51.TTC2)
            }

            Name (TTSP, 0x0A)
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ51.TTSP)
            }

            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.MPA, 
                })
            }
        }

        ThermalZone (TZ52)
        {
            Name (_HID, "QCOM04C1")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_TZD, Package (0x01)  // _TZD: Thermal Zone Devices
            {
                \_SB.MPA1, 
            })
            Name (TPSV, 0x0E60)
            Method (_PSV, 0, NotSerialized)  // _PSV: Passive Temperature
            {
                Return (\_SB.TZ52.TPSV)
            }

            Name (TCRT, 0x0F5A)
            Method (_CRT, 0, NotSerialized)  // _CRT: Critical Temperature
            {
                Return (\_SB.TZ52.TCRT)
            }

            Name (TTC1, One)
            Method (_TC1, 0, NotSerialized)  // _TC1: Thermal Constant 1
            {
                Return (\_SB.TZ52.TTC1)
            }

            Name (TTC2, 0x02)
            Method (_TC2, 0, NotSerialized)  // _TC2: Thermal Constant 2
            {
                Return (\_SB.TZ52.TTC2)
            }

            Name (TTSP, 0x0A)
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ52.TTSP)
            }

            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.MPA1, 
                })
            }
        }

        ThermalZone (TZ53)
        {
            Name (_HID, "QCOM04C2")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_TZD, Package (0x01)  // _TZD: Thermal Zone Devices
            {
                \_SB.MBJ0, 
            })
            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.MBJ0, 
                })
            }
        }

        ThermalZone (TZ54)
        {
            Name (_HID, "QCOM04C3")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_TZD, Package (0x01)  // _TZD: Thermal Zone Devices
            {
                \_SB.MBJ1, 
            })
            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.MBJ1, 
                })
            }
        }

        ThermalZone (TZ55)
        {
            Name (_HID, "QCOM04C4")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_TZD, Package (0x01)  // _TZD: Thermal Zone Devices
            {
                \_SB.MBJ2, 
            })
            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.MBJ2, 
                })
            }
        }

        ThermalZone (TZ56)
        {
            Name (_HID, "QCOM04C5")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_TZD, Package (0x01)  // _TZD: Thermal Zone Devices
            {
                \_SB.MBJ3, 
            })
            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.MBJ3, 
                })
            }
        }

        ThermalZone (TZ57)
        {
            Name (_HID, "QCOM04C6")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_TZD, Package (0x01)  // _TZD: Thermal Zone Devices
            {
                \_SB.MBS0, 
            })
            Name (TPSV, 0x0E60)
            Method (_PSV, 0, NotSerialized)  // _PSV: Passive Temperature
            {
                Return (\_SB.TZ57.TPSV)
            }

            Name (TCRT, 0x0F5A)
            Method (_CRT, 0, NotSerialized)  // _CRT: Critical Temperature
            {
                Return (\_SB.TZ57.TCRT)
            }

            Name (TTC1, One)
            Method (_TC1, 0, NotSerialized)  // _TC1: Thermal Constant 1
            {
                Return (\_SB.TZ57.TTC1)
            }

            Name (TTC2, 0x02)
            Method (_TC2, 0, NotSerialized)  // _TC2: Thermal Constant 2
            {
                Return (\_SB.TZ57.TTC2)
            }

            Name (TTSP, 0x0A)
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ57.TTSP)
            }

            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.MBS0, 
                })
            }
        }

        ThermalZone (TZ58)
        {
            Name (_HID, "QCOM04C7")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_TZD, Package (0x01)  // _TZD: Thermal Zone Devices
            {
                \_SB.MBS1, 
            })
            Name (TPSV, 0x0E60)
            Method (_PSV, 0, NotSerialized)  // _PSV: Passive Temperature
            {
                Return (\_SB.TZ58.TPSV)
            }

            Name (TCRT, 0x0F5A)
            Method (_CRT, 0, NotSerialized)  // _CRT: Critical Temperature
            {
                Return (\_SB.TZ58.TCRT)
            }

            Name (TTC1, One)
            Method (_TC1, 0, NotSerialized)  // _TC1: Thermal Constant 1
            {
                Return (\_SB.TZ58.TTC1)
            }

            Name (TTC2, 0x02)
            Method (_TC2, 0, NotSerialized)  // _TC2: Thermal Constant 2
            {
                Return (\_SB.TZ58.TTC2)
            }

            Name (TTSP, 0x0A)
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ58.TTSP)
            }

            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.MBS1, 
                })
            }
        }

        ThermalZone (TZ59)
        {
            Name (_HID, "QCOM04C8")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Name (_TZD, Package (0x01)  // _TZD: Thermal Zone Devices
            {
                \_SB.MBS2, 
            })
            Name (TPSV, 0x0E60)
            Method (_PSV, 0, NotSerialized)  // _PSV: Passive Temperature
            {
                Return (\_SB.TZ59.TPSV)
            }

            Name (TCRT, 0x0F5A)
            Method (_CRT, 0, NotSerialized)  // _CRT: Critical Temperature
            {
                Return (\_SB.TZ59.TCRT)
            }

            Name (TTC1, One)
            Method (_TC1, 0, NotSerialized)  // _TC1: Thermal Constant 1
            {
                Return (\_SB.TZ59.TTC1)
            }

            Name (TTC2, 0x02)
            Method (_TC2, 0, NotSerialized)  // _TC2: Thermal Constant 2
            {
                Return (\_SB.TZ59.TTC2)
            }

            Name (TTSP, 0x0A)
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ59.TTSP)
            }

            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x01)
                {
                    \_SB.MBS2, 
                })
            }
        }

        ThermalZone (TZ31)
        {
            Name (_HID, "QCOM0A5F")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Name (TTC1, One)
            Method (_TC1, 0, NotSerialized)  // _TC1: Thermal Constant 1
            {
                Return (\_SB.TZ31.TTC1)
            }

            Name (TTC2, 0x05)
            Method (_TC2, 0, NotSerialized)  // _TC2: Thermal Constant 2
            {
                Return (\_SB.TZ31.TTC2)
            }

            Name (TTSP, 0x1E)
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ31.TTSP)
            }

            Name (_TZP, Zero)  // _TZP: Thermal Zone Polling
            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x02)
                {
                    \_SB.PEP0, , 
                    \_SB.ADC1, 
                })
            }
        }

        ThermalZone (TZ32)
        {
            Name (_HID, "QCOM0A61")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x02)
                {
                    \_SB.PEP0, , 
                    \_SB.ADC1, 
                })
            }

            Name (_TZD, Package (0x05)  // _TZD: Thermal Zone Devices
            {
                \_SB.SYSM.CLUS.CPU4, , 
                \_SB.SYSM.CLUS.CPU5, , 
                \_SB.SYSM.CLUS.CPU6, , 
                \_SB.SYSM.CLUS.CPU7, , 
                \_SB.GPU0, 
            })
            Name (TPSV, 0x0E2E)
            Method (_PSV, 0, NotSerialized)  // _PSV: Passive Temperature
            {
                Return (\_SB.TZ32.TPSV)
            }

            Name (TCRT, 0x0EF6)
            Method (_CRT, 0, NotSerialized)  // _CRT: Critical Temperature
            {
                Return (\_SB.TZ32.TCRT)
            }

            Name (_MTL, 0x14)  // _MTL: Minimum Throttle Limit
            Name (TTC1, Zero)
            Method (_TC1, 0, NotSerialized)  // _TC1: Thermal Constant 1
            {
                Return (\_SB.TZ32.TTC1)
            }

            Name (TTC2, 0x14)
            Method (_TC2, 0, NotSerialized)  // _TC2: Thermal Constant 2
            {
                Return (\_SB.TZ32.TTC2)
            }

            Name (TTSP, 0x28)
            Name (_TZP, Zero)  // _TZP: Thermal Zone Polling
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ32.TTSP)
            }
        }

        ThermalZone (TZ33)
        {
            Name (_HID, "QCOM0A63")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Method (_DEP, 0, NotSerialized)  // _DEP: Dependencies
            {
                Return (Package (0x02)
                {
                    \_SB.PEP0, , 
                    \_SB.ADC1, 
                })
            }

            Name (_MTL, 0x14)  // _MTL: Minimum Throttle Limit
            Name (TTC1, Zero)
            Method (_TC1, 0, NotSerialized)  // _TC1: Thermal Constant 1
            {
                Return (\_SB.TZ33.TTC1)
            }

            Name (TTC2, 0x14)
            Method (_TC2, 0, NotSerialized)  // _TC2: Thermal Constant 2
            {
                Return (\_SB.TZ33.TTC2)
            }

            Name (TTSP, 0x1E)
            Name (_TZP, Zero)  // _TZP: Thermal Zone Polling
            Method (_TSP, 0, NotSerialized)  // _TSP: Thermal Sampling Period
            {
                Return (\_SB.TZ33.TTSP)
            }
        }

        Name (HWNH, Zero)
        Device (HWN1)
        {
            Name (_HID, "QCOM0A69")  // _HID: Hardware ID
            Name (_UID, One)  // _UID: Unique ID
            Alias (\_SB.PSUB, _SUB)
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If ((\_SB.HWNH == Zero))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (Zero)
                }
            }

            Name (_DEP, Package (One)  // _DEP: Dependencies
            {
                \_SB.PMIC, 
            })
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x25)
                {
                    /* 0000 */  0x8C, 0x20, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,  // . ......
                    /* 0008 */  0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00,  // ........
                    /* 0010 */  0x00, 0x19, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00,  // ...#....
                    /* 0018 */  0x0E, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x50, 0x4D,  // .\_SB.PM
                    /* 0020 */  0x30, 0x32, 0x00, 0x79, 0x00                     // 02.y.
                })
                Return (RBUF) /* \_SB_.HWN1._CRS.RBUF */
            }

            Method (HAPI, 0, NotSerialized)
            {
                Name (CFG0, Package (0x03)
                {
                    One, 
                    One, 
                    One
                })
                Return (CFG0) /* \_SB_.HWN1.HAPI.CFG0 */
            }

            Method (HAPC, 0, NotSerialized)
            {
                Name (CFG0, Package (0x16)
                {
                    Zero, 
                    0x0984, 
                    Zero, 
                    One, 
                    One, 
                    One, 
                    One, 
                    Zero, 
                    0x04, 
                    One, 
                    0x03, 
                    0x14, 
                    One, 
                    0x03, 
                    Zero, 
                    Zero, 
                    0x06, 
                    Zero, 
                    Zero, 
                    0x0535, 
                    0x03, 
                    One
                })
                Return (CFG0) /* \_SB_.HWN1.HAPC.CFG0 */
            }
        }

        Device (BTNS)
        {
            Name (_HID, "ACPI0011" /* Generic Buttons Device */)  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_UID, Zero)  // _UID: Unique ID
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (RBUF, Buffer (0x6B)
                {
                    /* 0000 */  0x8C, 0x20, 0x00, 0x01, 0x00, 0x01, 0x00, 0x15,  // . ......
                    /* 0008 */  0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00,  // ........
                    /* 0010 */  0x00, 0x19, 0x00, 0x23, 0x00, 0x00, 0x00, 0x07,  // ...#....
                    /* 0018 */  0x00, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x50, 0x4D,  // .\_SB.PM
                    /* 0020 */  0x30, 0x31, 0x00, 0x8C, 0x20, 0x00, 0x01, 0x00,  // 01.. ...
                    /* 0028 */  0x01, 0x00, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00,  // ........
                    /* 0030 */  0x00, 0x17, 0x00, 0x00, 0x19, 0x00, 0x23, 0x00,  // ......#.
                    /* 0038 */  0x00, 0x00, 0xC6, 0x00, 0x5C, 0x5F, 0x53, 0x42,  // ....\_SB
                    /* 0040 */  0x2E, 0x50, 0x4D, 0x30, 0x31, 0x00, 0x8C, 0x20,  // .PM01.. 
                    /* 0048 */  0x00, 0x01, 0x00, 0x01, 0x00, 0x05, 0x00, 0x02,  // ........
                    /* 0050 */  0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x19,  // ........
                    /* 0058 */  0x00, 0x23, 0x00, 0x00, 0x00, 0x06, 0x00, 0x5C,  // .#.....\
                    /* 0060 */  0x5F, 0x53, 0x42, 0x2E, 0x50, 0x4D, 0x30, 0x31,  // _SB.PM01
                    /* 0068 */  0x00, 0x79, 0x00                                 // .y.
                })
                Return (RBUF) /* \_SB_.BTNS._CRS.RBUF */
            }

            Name (_DSD, Package (0x02)  // _DSD: Device-Specific Data
            {
                ToUUID ("fa6bd625-9ce8-470d-a2c7-b3ca36c4282e") /* Generic Buttons Device */, 
                Package (0x04)
                {
                    Package (0x05)
                    {
                        Zero, 
                        One, 
                        Zero, 
                        One, 
                        0x0D
                    }, 

                    Package (0x05)
                    {
                        One, 
                        Zero, 
                        One, 
                        One, 
                        0x81
                    }, 

                    Package (0x05)
                    {
                        One, 
                        One, 
                        One, 
                        0x0C, 
                        0xE9
                    }, 

                    Package (0x05)
                    {
                        One, 
                        0x02, 
                        One, 
                        0x0C, 
                        0xEA
                    }
                }
            })
        }

        Device (NRCX)
        {
            Name (_HID, "QCOM0AD6")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If ((\_SB.SJTG == 0x001980E1))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Device (PSAU)
        {
            Name (_HID, "QCOM0AE1")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                If ((\_SB.SJTG == 0x001980E1))
                {
                    Return (Zero)
                }
                Else
                {
                    Return (0x0F)
                }
            }
        }

        Device (BTH0)
        {
            Name (_HID, "QCOM0A6B")  // _HID: Hardware ID
            Alias (\_SB.PSUB, _SUB)
            Name (_DEP, Package (0x03)  // _DEP: Dependencies
            {
                \_SB.PEP0, , 
                \_SB.PMIC, , 
                \_SB.UAR8, 
            })
            Name (_PRW, Package (0x02)  // _PRW: Power Resources for Wake
            {
                Zero, 
                Zero
            })
            Name (_S4W, 0x02)  // _S4W: S4 Device Wake State
            Name (_S0W, 0x02)  // _S0W: S0 Device Wake State
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (PBUF, Buffer (0x45)
                {
                    /* 0000 */  0x8E, 0x1D, 0x00, 0x01, 0x00, 0x03, 0x02, 0x35,  // .......5
                    /* 0008 */  0x00, 0x01, 0x0A, 0x00, 0x00, 0xC2, 0x01, 0x00,  // ........
                    /* 0010 */  0x20, 0x00, 0x20, 0x00, 0x00, 0xC0, 0x5C, 0x5F,  //  . ...\_
                    /* 0018 */  0x53, 0x42, 0x2E, 0x55, 0x41, 0x52, 0x38, 0x00,  // SB.UAR8.
                    /* 0020 */  0x8C, 0x20, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00,  // . ......
                    /* 0028 */  0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00,  // ........
                    /* 0030 */  0x00, 0x19, 0x00, 0x23, 0x00, 0x00, 0x00, 0x55,  // ...#...U
                    /* 0038 */  0x00, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x47, 0x49,  // .\_SB.GI
                    /* 0040 */  0x4F, 0x30, 0x00, 0x79, 0x00                     // O0.y.
                })
                Return (PBUF) /* \_SB_.BTH0._CRS.PBUF */
            }

            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (0x0F)
            }
        }

        Device (ADC1)
        {
            Name (_DEP, Package (0x02)  // _DEP: Dependencies
            {
                \_SB.SPMI, , 
                \_SB.PMIC, 
            })
            Name (_HID, "QCOM0A11")  // _HID: Hardware ID
            Name (_UID, Zero)  // _UID: Unique ID
            Alias (\_SB.PSUB, _SUB)
            Method (_CRS, 0, NotSerialized)  // _CRS: Current Resource Settings
            {
                Name (INTB, Buffer (0x4A)
                {
                    /* 0000 */  0x8C, 0x21, 0x00, 0x01, 0x00, 0x01, 0x00, 0x11,  // .!......
                    /* 0008 */  0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00,  // ........
                    /* 0010 */  0x00, 0x19, 0x00, 0x23, 0x00, 0x01, 0x00, 0x20,  // ...#... 
                    /* 0018 */  0x00, 0x5C, 0x5F, 0x53, 0x42, 0x2E, 0x50, 0x4D,  // .\_SB.PM
                    /* 0020 */  0x30, 0x31, 0x00, 0x02, 0x8C, 0x21, 0x00, 0x01,  // 01...!..
                    /* 0028 */  0x00, 0x01, 0x00, 0x11, 0x00, 0x01, 0x00, 0x00,  // ........
                    /* 0030 */  0x00, 0x00, 0x17, 0x00, 0x00, 0x19, 0x00, 0x23,  // .......#
                    /* 0038 */  0x00, 0x01, 0x00, 0x28, 0x00, 0x5C, 0x5F, 0x53,  // ...(.\_S
                    /* 0040 */  0x42, 0x2E, 0x50, 0x4D, 0x30, 0x31, 0x00, 0x02,  // B.PM01..
                    /* 0048 */  0x79, 0x00                                       // y.
                })
                Name (NAM, Buffer (0x0A)
                {
                    "\\_SB.SPMI"
                })
                Name (VUSR, Buffer (0x0C)
                {
                    /* 0000 */  0x8E, 0x13, 0x00, 0x01, 0x00, 0xC1, 0x02, 0x00,  // ........
                    /* 0008 */  0x31, 0x01, 0x00, 0x00                           // 1...
                })
                Name (VBTM, Buffer (0x0C)
                {
                    /* 0000 */  0x8E, 0x13, 0x00, 0x01, 0x00, 0xC1, 0x02, 0x00,  // ........
                    /* 0008 */  0x34, 0x01, 0x00, 0x00                           // 4...
                })
                Concatenate (VUSR, NAM, Local1)
                Concatenate (VBTM, NAM, Local2)
                Concatenate (Local1, Local2, Local3)
                Concatenate (Local3, INTB, Local0)
                Return (Local0)
            }
        }
    }
}


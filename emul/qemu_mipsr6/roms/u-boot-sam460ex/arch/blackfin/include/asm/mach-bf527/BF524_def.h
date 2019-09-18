/* DO NOT EDIT THIS FILE
 * Automatically generated by generate-def-headers.xsl
 * DO NOT EDIT THIS FILE
 */

#ifndef __BFIN_DEF_ADSP_BF524_proc__
#define __BFIN_DEF_ADSP_BF524_proc__

#include "../mach-common/ADSP-EDN-core_def.h"

#include "ADSP-EDN-BF52x-extended_def.h"

#define PLL_CTL                        0xFFC00000 /* PLL Control Register */
#define PLL_DIV                        0xFFC00004 /* PLL Divide Register */
#define VR_CTL                         0xFFC00008 /* Voltage Regulator Control Register */
#define PLL_STAT                       0xFFC0000C /* PLL Status Register */
#define PLL_LOCKCNT                    0xFFC00010 /* PLL Lock Count Register */
#define CHIPID                         0xFFC00014
#define SWRST                          0xFFC00100 /* Software Reset Register */
#define SYSCR                          0xFFC00104 /* System Configuration register */
#define SRAM_BASE_ADDR                 0xFFE00000 /* SRAM Base Address (Read Only) */
#define DMEM_CONTROL                   0xFFE00004 /* Data memory control */
#define DCPLB_STATUS                   0xFFE00008 /* Data Cache Programmable Look-Aside Buffer Status */
#define DCPLB_FAULT_ADDR               0xFFE0000C /* Data Cache Programmable Look-Aside Buffer Fault Address */
#define DCPLB_ADDR0                    0xFFE00100 /* Data Cache Protection Lookaside Buffer 0 */
#define DCPLB_ADDR1                    0xFFE00104 /* Data Cache Protection Lookaside Buffer 1 */
#define DCPLB_ADDR2                    0xFFE00108 /* Data Cache Protection Lookaside Buffer 2 */
#define DCPLB_ADDR3                    0xFFE0010C /* Data Cache Protection Lookaside Buffer 3 */
#define DCPLB_ADDR4                    0xFFE00110 /* Data Cache Protection Lookaside Buffer 4 */
#define DCPLB_ADDR5                    0xFFE00114 /* Data Cache Protection Lookaside Buffer 5 */
#define DCPLB_ADDR6                    0xFFE00118 /* Data Cache Protection Lookaside Buffer 6 */
#define DCPLB_ADDR7                    0xFFE0011C /* Data Cache Protection Lookaside Buffer 7 */
#define DCPLB_ADDR8                    0xFFE00120 /* Data Cache Protection Lookaside Buffer 8 */
#define DCPLB_ADDR9                    0xFFE00124 /* Data Cache Protection Lookaside Buffer 9 */
#define DCPLB_ADDR10                   0xFFE00128 /* Data Cache Protection Lookaside Buffer 10 */
#define DCPLB_ADDR11                   0xFFE0012C /* Data Cache Protection Lookaside Buffer 11 */
#define DCPLB_ADDR12                   0xFFE00130 /* Data Cache Protection Lookaside Buffer 12 */
#define DCPLB_ADDR13                   0xFFE00134 /* Data Cache Protection Lookaside Buffer 13 */
#define DCPLB_ADDR14                   0xFFE00138 /* Data Cache Protection Lookaside Buffer 14 */
#define DCPLB_ADDR15                   0xFFE0013C /* Data Cache Protection Lookaside Buffer 15 */
#define DCPLB_DATA0                    0xFFE00200 /* Data Cache 0 Status */
#define DCPLB_DATA1                    0xFFE00204 /* Data Cache 1 Status */
#define DCPLB_DATA2                    0xFFE00208 /* Data Cache 2 Status */
#define DCPLB_DATA3                    0xFFE0020C /* Data Cache 3 Status */
#define DCPLB_DATA4                    0xFFE00210 /* Data Cache 4 Status */
#define DCPLB_DATA5                    0xFFE00214 /* Data Cache 5 Status */
#define DCPLB_DATA6                    0xFFE00218 /* Data Cache 6 Status */
#define DCPLB_DATA7                    0xFFE0021C /* Data Cache 7 Status */
#define DCPLB_DATA8                    0xFFE00220 /* Data Cache 8 Status */
#define DCPLB_DATA9                    0xFFE00224 /* Data Cache 9 Status */
#define DCPLB_DATA10                   0xFFE00228 /* Data Cache 10 Status */
#define DCPLB_DATA11                   0xFFE0022C /* Data Cache 11 Status */
#define DCPLB_DATA12                   0xFFE00230 /* Data Cache 12 Status */
#define DCPLB_DATA13                   0xFFE00234 /* Data Cache 13 Status */
#define DCPLB_DATA14                   0xFFE00238 /* Data Cache 14 Status */
#define DCPLB_DATA15                   0xFFE0023C /* Data Cache 15 Status */
#define DTEST_COMMAND                  0xFFE00300 /* Data Test Command Register */
#define DTEST_DATA0                    0xFFE00400 /* Data Test Data Register */
#define DTEST_DATA1                    0xFFE00404 /* Data Test Data Register */
#define IMEM_CONTROL                   0xFFE01004 /* Instruction Memory Control */
#define ICPLB_STATUS                   0xFFE01008 /* Instruction Cache Programmable Look-Aside Buffer Status */
#define ICPLB_FAULT_ADDR               0xFFE0100C /* Instruction Cache Programmable Look-Aside Buffer Fault Address */
#define ICPLB_ADDR0                    0xFFE01100 /* Instruction Cacheability Protection Lookaside Buffer 0 */
#define ICPLB_ADDR1                    0xFFE01104 /* Instruction Cacheability Protection Lookaside Buffer 1 */
#define ICPLB_ADDR2                    0xFFE01108 /* Instruction Cacheability Protection Lookaside Buffer 2 */
#define ICPLB_ADDR3                    0xFFE0110C /* Instruction Cacheability Protection Lookaside Buffer 3 */
#define ICPLB_ADDR4                    0xFFE01110 /* Instruction Cacheability Protection Lookaside Buffer 4 */
#define ICPLB_ADDR5                    0xFFE01114 /* Instruction Cacheability Protection Lookaside Buffer 5 */
#define ICPLB_ADDR6                    0xFFE01118 /* Instruction Cacheability Protection Lookaside Buffer 6 */
#define ICPLB_ADDR7                    0xFFE0111C /* Instruction Cacheability Protection Lookaside Buffer 7 */
#define ICPLB_ADDR8                    0xFFE01120 /* Instruction Cacheability Protection Lookaside Buffer 8 */
#define ICPLB_ADDR9                    0xFFE01124 /* Instruction Cacheability Protection Lookaside Buffer 9 */
#define ICPLB_ADDR10                   0xFFE01128 /* Instruction Cacheability Protection Lookaside Buffer 10 */
#define ICPLB_ADDR11                   0xFFE0112C /* Instruction Cacheability Protection Lookaside Buffer 11 */
#define ICPLB_ADDR12                   0xFFE01130 /* Instruction Cacheability Protection Lookaside Buffer 12 */
#define ICPLB_ADDR13                   0xFFE01134 /* Instruction Cacheability Protection Lookaside Buffer 13 */
#define ICPLB_ADDR14                   0xFFE01138 /* Instruction Cacheability Protection Lookaside Buffer 14 */
#define ICPLB_ADDR15                   0xFFE0113C /* Instruction Cacheability Protection Lookaside Buffer 15 */
#define ICPLB_DATA0                    0xFFE01200 /* Instruction Cache 0 Status */
#define ICPLB_DATA1                    0xFFE01204 /* Instruction Cache 1 Status */
#define ICPLB_DATA2                    0xFFE01208 /* Instruction Cache 2 Status */
#define ICPLB_DATA3                    0xFFE0120C /* Instruction Cache 3 Status */
#define ICPLB_DATA4                    0xFFE01210 /* Instruction Cache 4 Status */
#define ICPLB_DATA5                    0xFFE01214 /* Instruction Cache 5 Status */
#define ICPLB_DATA6                    0xFFE01218 /* Instruction Cache 6 Status */
#define ICPLB_DATA7                    0xFFE0121C /* Instruction Cache 7 Status */
#define ICPLB_DATA8                    0xFFE01220 /* Instruction Cache 8 Status */
#define ICPLB_DATA9                    0xFFE01224 /* Instruction Cache 9 Status */
#define ICPLB_DATA10                   0xFFE01228 /* Instruction Cache 10 Status */
#define ICPLB_DATA11                   0xFFE0122C /* Instruction Cache 11 Status */
#define ICPLB_DATA12                   0xFFE01230 /* Instruction Cache 12 Status */
#define ICPLB_DATA13                   0xFFE01234 /* Instruction Cache 13 Status */
#define ICPLB_DATA14                   0xFFE01238 /* Instruction Cache 14 Status */
#define ICPLB_DATA15                   0xFFE0123C /* Instruction Cache 15 Status */
#define ITEST_COMMAND                  0xFFE01300 /* Instruction Test Command Register */
#define ITEST_DATA0                    0xFFE01400 /* Instruction Test Data Register */
#define ITEST_DATA1                    0xFFE01404 /* Instruction Test Data Register */
#define EVT0                           0xFFE02000 /* Event Vector 0 ESR Address */
#define EVT1                           0xFFE02004 /* Event Vector 1 ESR Address */
#define EVT2                           0xFFE02008 /* Event Vector 2 ESR Address */
#define EVT3                           0xFFE0200C /* Event Vector 3 ESR Address */
#define EVT4                           0xFFE02010 /* Event Vector 4 ESR Address */
#define EVT5                           0xFFE02014 /* Event Vector 5 ESR Address */
#define EVT6                           0xFFE02018 /* Event Vector 6 ESR Address */
#define EVT7                           0xFFE0201C /* Event Vector 7 ESR Address */
#define EVT8                           0xFFE02020 /* Event Vector 8 ESR Address */
#define EVT9                           0xFFE02024 /* Event Vector 9 ESR Address */
#define EVT10                          0xFFE02028 /* Event Vector 10 ESR Address */
#define EVT11                          0xFFE0202C /* Event Vector 11 ESR Address */
#define EVT12                          0xFFE02030 /* Event Vector 12 ESR Address */
#define EVT13                          0xFFE02034 /* Event Vector 13 ESR Address */
#define EVT14                          0xFFE02038 /* Event Vector 14 ESR Address */
#define EVT15                          0xFFE0203C /* Event Vector 15 ESR Address */
#define ILAT                           0xFFE0210C /* Interrupt Latch Register */
#define IMASK                          0xFFE02104 /* Interrupt Mask Register */
#define IPEND                          0xFFE02108 /* Interrupt Pending Register */
#define IPRIO                          0xFFE02110 /* Interrupt Priority Register */
#define TCNTL                          0xFFE03000 /* Core Timer Control Register */
#define TPERIOD                        0xFFE03004 /* Core Timer Period Register */
#define TSCALE                         0xFFE03008 /* Core Timer Scale Register */
#define TCOUNT                         0xFFE0300C /* Core Timer Count Register */
#define USB_FADDR                      0xFFC03800 /* Function address register */
#define USB_POWER                      0xFFC03804 /* Power management register */
#define USB_INTRTX                     0xFFC03808 /* Interrupt register for endpoint 0 and Tx endpoint 1 to 7 */
#define USB_INTRRX                     0xFFC0380C /* Interrupt register for Rx endpoints 1 to 7 */
#define USB_INTRTXE                    0xFFC03810 /* Interrupt enable register for IntrTx */
#define USB_INTRRXE                    0xFFC03814 /* Interrupt enable register for IntrRx */
#define USB_INTRUSB                    0xFFC03818 /* Interrupt register for common USB interrupts */
#define USB_INTRUSBE                   0xFFC0381C /* Interrupt enable register for IntrUSB */
#define USB_FRAME                      0xFFC03820 /* USB frame number */
#define USB_INDEX                      0xFFC03824 /* Index register for selecting the indexed endpoint registers */
#define USB_TESTMODE                   0xFFC03828 /* Enabled USB 20 test modes */
#define USB_GLOBINTR                   0xFFC0382C /* Global Interrupt Mask register and Wakeup Exception Interrupt */
#define USB_GLOBAL_CTL                 0xFFC03830 /* Global Clock Control for the core */
#define USB_TX_MAX_PACKET              0xFFC03840 /* Maximum packet size for Host Tx endpoint */
#define USB_CSR0                       0xFFC03844 /* Control Status register for endpoint 0 and Control Status register for Host Tx endpoint */
#define USB_TXCSR                      0xFFC03844 /* Control Status register for endpoint 0 and Control Status register for Host Tx endpoint */
#define USB_RX_MAX_PACKET              0xFFC03848 /* Maximum packet size for Host Rx endpoint */
#define USB_RXCSR                      0xFFC0384C /* Control Status register for Host Rx endpoint */
#define USB_COUNT0                     0xFFC03850 /* Number of bytes received in endpoint 0 FIFO and Number of bytes received in Host Tx endpoint */
#define USB_RXCOUNT                    0xFFC03850 /* Number of bytes received in endpoint 0 FIFO and Number of bytes received in Host Tx endpoint */
#define USB_TXTYPE                     0xFFC03854 /* Sets the transaction protocol and peripheral endpoint number for the Host Tx endpoint */
#define USB_NAKLIMIT0                  0xFFC03858 /* Sets the NAK response timeout on Endpoint 0 and on Bulk transfers for Host Tx endpoint */
#define USB_TXINTERVAL                 0xFFC03858 /* Sets the NAK response timeout on Endpoint 0 and on Bulk transfers for Host Tx endpoint */
#define USB_RXTYPE                     0xFFC0385C /* Sets the transaction protocol and peripheral endpoint number for the Host Rx endpoint */
#define USB_RXINTERVAL                 0xFFC03860 /* Sets the polling interval for Interrupt and Isochronous transfers or the NAK response timeout on Bulk transfers */
#define USB_TXCOUNT                    0xFFC03868 /* Number of bytes to be written to the selected endpoint Tx FIFO */
#define USB_EP0_FIFO                   0xFFC03880 /* Endpoint 0 FIFO */
#define USB_EP1_FIFO                   0xFFC03888 /* Endpoint 1 FIFO */
#define USB_EP2_FIFO                   0xFFC03890 /* Endpoint 2 FIFO */
#define USB_EP3_FIFO                   0xFFC03898 /* Endpoint 3 FIFO */
#define USB_EP4_FIFO                   0xFFC038A0 /* Endpoint 4 FIFO */
#define USB_EP5_FIFO                   0xFFC038A8 /* Endpoint 5 FIFO */
#define USB_EP6_FIFO                   0xFFC038B0 /* Endpoint 6 FIFO */
#define USB_EP7_FIFO                   0xFFC038B8 /* Endpoint 7 FIFO */
#define USB_OTG_DEV_CTL                0xFFC03900 /* OTG Device Control Register */
#define USB_OTG_VBUS_IRQ               0xFFC03904 /* OTG VBUS Control Interrupts */
#define USB_OTG_VBUS_MASK              0xFFC03908 /* VBUS Control Interrupt Enable */
#define USB_LINKINFO                   0xFFC03948 /* Enables programming of some PHY-side delays */
#define USB_VPLEN                      0xFFC0394C /* Determines duration of VBUS pulse for VBUS charging */
#define USB_HS_EOF1                    0xFFC03950 /* Time buffer for High-Speed transactions */
#define USB_FS_EOF1                    0xFFC03954 /* Time buffer for Full-Speed transactions */
#define USB_LS_EOF1                    0xFFC03958 /* Time buffer for Low-Speed transactions */
#define USB_APHY_CNTRL                 0xFFC039E0 /* Register that increases visibility of Analog PHY */
#define USB_APHY_CALIB                 0xFFC039E4 /* Register used to set some calibration values */
#define USB_APHY_CNTRL2                0xFFC039E8 /* Register used to prevent re-enumeration once Moab goes into hibernate mode */
#define USB_PHY_TEST                   0xFFC039EC /* Used for reducing simulation time and simplifies FIFO testability */
#define USB_PLLOSC_CTRL                0xFFC039F0 /* Used to program different parameters for USB PLL and Oscillator */
#define USB_SRP_CLKDIV                 0xFFC039F4 /* Used to program clock divide value for the clock fed to the SRP detection logic */
#define USB_EP_NI0_TXMAXP              0xFFC03A00 /* Maximum packet size for Host Tx endpoint0 */
#define USB_EP_NI0_TXCSR               0xFFC03A04 /* Control Status register for endpoint 0 */
#define USB_EP_NI0_RXMAXP              0xFFC03A08 /* Maximum packet size for Host Rx endpoint0 */
#define USB_EP_NI0_RXCSR               0xFFC03A0C /* Control Status register for Host Rx endpoint0 */
#define USB_EP_NI0_RXCOUNT             0xFFC03A10 /* Number of bytes received in endpoint 0 FIFO */
#define USB_EP_NI0_TXTYPE              0xFFC03A14 /* Sets the transaction protocol and peripheral endpoint number for the Host Tx endpoint0 */
#define USB_EP_NI0_TXINTERVAL          0xFFC03A18 /* Sets the NAK response timeout on Endpoint 0 */
#define USB_EP_NI0_RXTYPE              0xFFC03A1C /* Sets the transaction protocol and peripheral endpoint number for the Host Rx endpoint0 */
#define USB_EP_NI0_RXINTERVAL          0xFFC03A20 /* Sets the polling interval for Interrupt/Isochronous transfers or the NAK response timeout on Bulk transfers for Host Rx endpoint0 */
#define USB_EP_NI0_TXCOUNT             0xFFC03A28 /* Number of bytes to be written to the endpoint0 Tx FIFO */
#define USB_EP_NI1_TXMAXP              0xFFC03A40 /* Maximum packet size for Host Tx endpoint1 */
#define USB_EP_NI1_TXCSR               0xFFC03A44 /* Control Status register for endpoint1 */
#define USB_EP_NI1_RXMAXP              0xFFC03A48 /* Maximum packet size for Host Rx endpoint1 */
#define USB_EP_NI1_RXCSR               0xFFC03A4C /* Control Status register for Host Rx endpoint1 */
#define USB_EP_NI1_RXCOUNT             0xFFC03A50 /* Number of bytes received in endpoint1 FIFO */
#define USB_EP_NI1_TXTYPE              0xFFC03A54 /* Sets the transaction protocol and peripheral endpoint number for the Host Tx endpoint1 */
#define USB_EP_NI1_TXINTERVAL          0xFFC03A58 /* Sets the NAK response timeout on Endpoint1 */
#define USB_EP_NI1_RXTYPE              0xFFC03A5C /* Sets the transaction protocol and peripheral endpoint number for the Host Rx endpoint1 */
#define USB_EP_NI1_RXINTERVAL          0xFFC03A60 /* Sets the polling interval for Interrupt/Isochronous transfers or the NAK response timeout on Bulk transfers for Host Rx endpoint1 */
#define USB_EP_NI1_TXCOUNT             0xFFC03A68 /* Number of bytes to be written to the+H102 endpoint1 Tx FIFO */
#define USB_EP_NI2_TXMAXP              0xFFC03A80 /* Maximum packet size for Host Tx endpoint2 */
#define USB_EP_NI2_TXCSR               0xFFC03A84 /* Control Status register for endpoint2 */
#define USB_EP_NI2_RXMAXP              0xFFC03A88 /* Maximum packet size for Host Rx endpoint2 */
#define USB_EP_NI2_RXCSR               0xFFC03A8C /* Control Status register for Host Rx endpoint2 */
#define USB_EP_NI2_RXCOUNT             0xFFC03A90 /* Number of bytes received in endpoint2 FIFO */
#define USB_EP_NI2_TXTYPE              0xFFC03A94 /* Sets the transaction protocol and peripheral endpoint number for the Host Tx endpoint2 */
#define USB_EP_NI2_TXINTERVAL          0xFFC03A98 /* Sets the NAK response timeout on Endpoint2 */
#define USB_EP_NI2_RXTYPE              0xFFC03A9C /* Sets the transaction protocol and peripheral endpoint number for the Host Rx endpoint2 */
#define USB_EP_NI2_RXINTERVAL          0xFFC03AA0 /* Sets the polling interval for Interrupt/Isochronous transfers or the NAK response timeout on Bulk transfers for Host Rx endpoint2 */
#define USB_EP_NI2_TXCOUNT             0xFFC03AA8 /* Number of bytes to be written to the endpoint2 Tx FIFO */
#define USB_EP_NI3_TXMAXP              0xFFC03AC0 /* Maximum packet size for Host Tx endpoint3 */
#define USB_EP_NI3_TXCSR               0xFFC03AC4 /* Control Status register for endpoint3 */
#define USB_EP_NI3_RXMAXP              0xFFC03AC8 /* Maximum packet size for Host Rx endpoint3 */
#define USB_EP_NI3_RXCSR               0xFFC03ACC /* Control Status register for Host Rx endpoint3 */
#define USB_EP_NI3_RXCOUNT             0xFFC03AD0 /* Number of bytes received in endpoint3 FIFO */
#define USB_EP_NI3_TXTYPE              0xFFC03AD4 /* Sets the transaction protocol and peripheral endpoint number for the Host Tx endpoint3 */
#define USB_EP_NI3_TXINTERVAL          0xFFC03AD8 /* Sets the NAK response timeout on Endpoint3 */
#define USB_EP_NI3_RXTYPE              0xFFC03ADC /* Sets the transaction protocol and peripheral endpoint number for the Host Rx endpoint3 */
#define USB_EP_NI3_RXINTERVAL          0xFFC03AE0 /* Sets the polling interval for Interrupt/Isochronous transfers or the NAK response timeout on Bulk transfers for Host Rx endpoint3 */
#define USB_EP_NI3_TXCOUNT             0xFFC03AE8 /* Number of bytes to be written to the H124endpoint3 Tx FIFO */
#define USB_EP_NI4_TXMAXP              0xFFC03B00 /* Maximum packet size for Host Tx endpoint4 */
#define USB_EP_NI4_TXCSR               0xFFC03B04 /* Control Status register for endpoint4 */
#define USB_EP_NI4_RXMAXP              0xFFC03B08 /* Maximum packet size for Host Rx endpoint4 */
#define USB_EP_NI4_RXCSR               0xFFC03B0C /* Control Status register for Host Rx endpoint4 */
#define USB_EP_NI4_RXCOUNT             0xFFC03B10 /* Number of bytes received in endpoint4 FIFO */
#define USB_EP_NI4_TXTYPE              0xFFC03B14 /* Sets the transaction protocol and peripheral endpoint number for the Host Tx endpoint4 */
#define USB_EP_NI4_TXINTERVAL          0xFFC03B18 /* Sets the NAK response timeout on Endpoint4 */
#define USB_EP_NI4_RXTYPE              0xFFC03B1C /* Sets the transaction protocol and peripheral endpoint number for the Host Rx endpoint4 */
#define USB_EP_NI4_RXINTERVAL          0xFFC03B20 /* Sets the polling interval for Interrupt/Isochronous transfers or the NAK response timeout on Bulk transfers for Host Rx endpoint4 */
#define USB_EP_NI4_TXCOUNT             0xFFC03B28 /* Number of bytes to be written to the endpoint4 Tx FIFO */
#define USB_EP_NI5_TXMAXP              0xFFC03B40 /* Maximum packet size for Host Tx endpoint5 */
#define USB_EP_NI5_TXCSR               0xFFC03B44 /* Control Status register for endpoint5 */
#define USB_EP_NI5_RXMAXP              0xFFC03B48 /* Maximum packet size for Host Rx endpoint5 */
#define USB_EP_NI5_RXCSR               0xFFC03B4C /* Control Status register for Host Rx endpoint5 */
#define USB_EP_NI5_RXCOUNT             0xFFC03B50 /* Number of bytes received in endpoint5 FIFO */
#define USB_EP_NI5_TXTYPE              0xFFC03B54 /* Sets the transaction protocol and peripheral endpoint number for the Host Tx endpoint5 */
#define USB_EP_NI5_TXINTERVAL          0xFFC03B58 /* Sets the NAK response timeout on Endpoint5 */
#define USB_EP_NI5_RXTYPE              0xFFC03B5C /* Sets the transaction protocol and peripheral endpoint number for the Host Rx endpoint5 */
#define USB_EP_NI5_RXINTERVAL          0xFFC03B60 /* Sets the polling interval for Interrupt/Isochronous transfers or the NAK response timeout on Bulk transfers for Host Rx endpoint5 */
#define USB_EP_NI5_TXCOUNT             0xFFC03B68 /* Number of bytes to be written to the endpoint5 Tx FIFO */
#define USB_EP_NI6_TXMAXP              0xFFC03B80 /* Maximum packet size for Host Tx endpoint6 */
#define USB_EP_NI6_TXCSR               0xFFC03B84 /* Control Status register for endpoint6 */
#define USB_EP_NI6_RXMAXP              0xFFC03B88 /* Maximum packet size for Host Rx endpoint6 */
#define USB_EP_NI6_RXCSR               0xFFC03B8C /* Control Status register for Host Rx endpoint6 */
#define USB_EP_NI6_RXCOUNT             0xFFC03B90 /* Number of bytes received in endpoint6 FIFO */
#define USB_EP_NI6_TXTYPE              0xFFC03B94 /* Sets the transaction protocol and peripheral endpoint number for the Host Tx endpoint6 */
#define USB_EP_NI6_TXINTERVAL          0xFFC03B98 /* Sets the NAK response timeout on Endpoint6 */
#define USB_EP_NI6_RXTYPE              0xFFC03B9C /* Sets the transaction protocol and peripheral endpoint number for the Host Rx endpoint6 */
#define USB_EP_NI6_RXINTERVAL          0xFFC03BA0 /* Sets the polling interval for Interrupt/Isochronous transfers or the NAK response timeout on Bulk transfers for Host Rx endpoint6 */
#define USB_EP_NI6_TXCOUNT             0xFFC03BA8 /* Number of bytes to be written to the endpoint6 Tx FIFO */
#define USB_EP_NI7_TXMAXP              0xFFC03BC0 /* Maximum packet size for Host Tx endpoint7 */
#define USB_EP_NI7_TXCSR               0xFFC03BC4 /* Control Status register for endpoint7 */
#define USB_EP_NI7_RXMAXP              0xFFC03BC8 /* Maximum packet size for Host Rx endpoint7 */
#define USB_EP_NI7_RXCSR               0xFFC03BCC /* Control Status register for Host Rx endpoint7 */
#define USB_EP_NI7_RXCOUNT             0xFFC03BD0 /* Number of bytes received in endpoint7 FIFO */
#define USB_EP_NI7_TXTYPE              0xFFC03BD4 /* Sets the transaction protocol and peripheral endpoint number for the Host Tx endpoint7 */
#define USB_EP_NI7_TXINTERVAL          0xFFC03BD8 /* Sets the NAK response timeout on Endpoint7 */
#define USB_EP_NI7_RXTYPE              0xFFC03BDC /* Sets the transaction protocol and peripheral endpoint number for the Host Rx endpoint7 */
#define USB_EP_NI7_RXINTERVAL          0xFFC03BF0 /* Sets the polling interval for Interrupt/Isochronous transfers or the NAK response timeout on Bulk transfers for Host Rx endpoint7 */
#define USB_EP_NI7_TXCOUNT             0xFFC03BF8 /* Number of bytes to be written to the endpoint7 Tx FIFO */
#define USB_DMA_INTERRUPT              0xFFC03C00 /* Indicates pending interrupts for the DMA channels */
#define USB_DMA0_CONTROL               0xFFC03C04 /* DMA master channel 0 configuration */
#define USB_DMA0_ADDRLOW               0xFFC03C08 /* Lower 16-bits of memory source/destination address for DMA master channel 0 */
#define USB_DMA0_ADDRHIGH              0xFFC03C0C /* Upper 16-bits of memory source/destination address for DMA master channel 0 */
#define USB_DMA0_COUNTLOW              0xFFC03C10 /* Lower 16-bits of byte count of DMA transfer for DMA master channel 0 */
#define USB_DMA0_COUNTHIGH             0xFFC03C14 /* Upper 16-bits of byte count of DMA transfer for DMA master channel 0 */
#define USB_DMA1_CONTROL               0xFFC03C24 /* DMA master channel 1 configuration */
#define USB_DMA1_ADDRLOW               0xFFC03C28 /* Lower 16-bits of memory source/destination address for DMA master channel 1 */
#define USB_DMA1_ADDRHIGH              0xFFC03C2C /* Upper 16-bits of memory source/destination address for DMA master channel 1 */
#define USB_DMA1_COUNTLOW              0xFFC03C30 /* Lower 16-bits of byte count of DMA transfer for DMA master channel 1 */
#define USB_DMA1_COUNTHIGH             0xFFC03C34 /* Upper 16-bits of byte count of DMA transfer for DMA master channel 1 */
#define USB_DMA2_CONTROL               0xFFC03C44 /* DMA master channel 2 configuration */
#define USB_DMA2_ADDRLOW               0xFFC03C48 /* Lower 16-bits of memory source/destination address for DMA master channel 2 */
#define USB_DMA2_ADDRHIGH              0xFFC03C4C /* Upper 16-bits of memory source/destination address for DMA master channel 2 */
#define USB_DMA2_COUNTLOW              0xFFC03C50 /* Lower 16-bits of byte count of DMA transfer for DMA master channel 2 */
#define USB_DMA2_COUNTHIGH             0xFFC03C54 /* Upper 16-bits of byte count of DMA transfer for DMA master channel 2 */
#define USB_DMA3_CONTROL               0xFFC03C64 /* DMA master channel 3 configuration */
#define USB_DMA3_ADDRLOW               0xFFC03C68 /* Lower 16-bits of memory source/destination address for DMA master channel 3 */
#define USB_DMA3_ADDRHIGH              0xFFC03C6C /* Upper 16-bits of memory source/destination address for DMA master channel 3 */
#define USB_DMA3_COUNTLOW              0xFFC03C70 /* Lower 16-bits of byte count of DMA transfer for DMA master channel 3 */
#define USB_DMA3_COUNTHIGH             0xFFC03C74 /* Upper 16-bits of byte count of DMA transfer for DMA master channel 3 */
#define USB_DMA4_CONTROL               0xFFC03C84 /* DMA master channel 4 configuration */
#define USB_DMA4_ADDRLOW               0xFFC03C88 /* Lower 16-bits of memory source/destination address for DMA master channel 4 */
#define USB_DMA4_ADDRHIGH              0xFFC03C8C /* Upper 16-bits of memory source/destination address for DMA master channel 4 */
#define USB_DMA4_COUNTLOW              0xFFC03C90 /* Lower 16-bits of byte count of DMA transfer for DMA master channel 4 */
#define USB_DMA4_COUNTHIGH             0xFFC03C94 /* Upper 16-bits of byte count of DMA transfer for DMA master channel 4 */
#define USB_DMA5_CONTROL               0xFFC03CA4 /* DMA master channel 5 configuration */
#define USB_DMA5_ADDRLOW               0xFFC03CA8 /* Lower 16-bits of memory source/destination address for DMA master channel 5 */
#define USB_DMA5_ADDRHIGH              0xFFC03CAC /* Upper 16-bits of memory source/destination address for DMA master channel 5 */
#define USB_DMA5_COUNTLOW              0xFFC03CB0 /* Lower 16-bits of byte count of DMA transfer for DMA master channel 5 */
#define USB_DMA5_COUNTHIGH             0xFFC03CB4 /* Upper 16-bits of byte count of DMA transfer for DMA master channel 5 */
#define USB_DMA6_CONTROL               0xFFC03CC4 /* DMA master channel 6 configuration */
#define USB_DMA6_ADDRLOW               0xFFC03CC8 /* Lower 16-bits of memory source/destination address for DMA master channel 6 */
#define USB_DMA6_ADDRHIGH              0xFFC03CCC /* Upper 16-bits of memory source/destination address for DMA master channel 6 */
#define USB_DMA6_COUNTLOW              0xFFC03CD0 /* Lower 16-bits of byte count of DMA transfer for DMA master channel 6 */
#define USB_DMA6_COUNTHIGH             0xFFC03CD4 /* Upper 16-bits of byte count of DMA transfer for DMA master channel 6 */
#define USB_DMA7_CONTROL               0xFFC03CE4 /* DMA master channel 7 configuration */
#define USB_DMA7_ADDRLOW               0xFFC03CE8 /* Lower 16-bits of memory source/destination address for DMA master channel 7 */
#define USB_DMA7_ADDRHIGH              0xFFC03CEC /* Upper 16-bits of memory source/destination address for DMA master channel 7 */
#define USB_DMA7_COUNTLOW              0xFFC03CF0 /* Lower 16-bits of byte count of DMA transfer for DMA master channel 7 */
#define USB_DMA7_COUNTHIGH             0xFFC03CF4 /* Upper 16-bits of byte count of DMA transfer for DMA master channel 7 */

#endif /* __BFIN_DEF_ADSP_BF524_proc__ */
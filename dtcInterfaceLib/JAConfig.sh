# Si538x/4x Registers Script
# 
# Part: Si5342
# Project File: Z:\mu2e\Mu2e Fanout Card\Ver2\DataSheets\JitterCleanerWithXtal\Si5342-RevD-Rev1-Project.slabtimeproj
# Design ID: Rev1
# Includes Pre/Post Download Control Register Writes: Yes
# Die Revision: B1
# Creator: ClockBuilder Pro v2.26.0.1 [2018-06-28]
# Created On: 2018-08-07 14:25:29 GMT-05:00
# Address,Data
# 
# Start configuration preamble
#set page B
my_cntl write 0x9168 0x68010B00
my_cntl write 0x916c 0x00000001

#page B registers
my_cntl write 0x9168 0x6824C000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68250000
my_cntl write 0x916c 0x00000001

#set page 5
my_cntl write 0x9168 0x68010500
my_cntl write 0x916c 0x00000001

#page 5 registers
my_cntl write 0x9168 0x68400100
my_cntl write 0x916c 0x00000001


# End configuration preamble
# 
# Delay 300 msec
#    Delay is worst case time for device to complete any calibration
#    that is running due to device state change previous to this script
#    being processed.
# 
# Start configuration registers
#set page 0
my_cntl write 0x9168 0x68010000
my_cntl write 0x916c 0x00000001

#page 0 registers
my_cntl write 0x9168 0x68060000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68070000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68080000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680B6800
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68160200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x6817DC00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68180000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x6819DD00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681ADF00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682B0200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682C0F00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682D5500
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682E3700
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682F0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68303700
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68310000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68323700
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68330000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68343700
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68350000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68363700
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68370000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68383700
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68390000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683A3700
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683B0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683C3700
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683D0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683FFF00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68400400
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68410E00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68420E00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68430E00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68440E00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68450C00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68463200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68473200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68483200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68493200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x684A3200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x684B3200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x684C3200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x684D3200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x684E5500
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x684F5500
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68500F00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68510300
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68520300
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68530300
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68540300
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68550300
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68560300
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68570300
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68580300
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68595500
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x685AAA00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x685BAA00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x685C0A00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x685D0100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x685EAA00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x685FAA00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68600A00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68610100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x6862AA00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x6863AA00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68640A00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68650100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x6866AA00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x6867AA00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68680A00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68690100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68920200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x6893A000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68950000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68968000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68986000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x689A0200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x689B6000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x689D0800
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x689E4000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68A02000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68A20000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68A98A00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68AA6100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68AB0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68AC0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68E52100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68EA0A00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68EB6000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68EC0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68ED0000
my_cntl write 0x916c 0x00000001



#set page 1
my_cntl write 0x9168 0x68010100
my_cntl write 0x916c 0x00000001

#page 1 registers
my_cntl write 0x9168 0x68020100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68120600
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68130900
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68143B00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68152800
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68170600
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68180900
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68193B00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681A2800
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683F1000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68400000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68414000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x6842FF00
my_cntl write 0x916c 0x00000001

#set page 2
my_cntl write 0x9168 0x68010200
my_cntl write 0x916c 0x00000001

#page 2 registers
my_cntl write 0x9168 0x68060000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68086400
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68090000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680A0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680B0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680C0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680D0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680E0100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680F0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68100000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68110000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68126400
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68130000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68140000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68150000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68160000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68170000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68180100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68190000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681A0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681B0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681C6400
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681D0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681E0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681F0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68200000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68210000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68220100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68230000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68240000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68250000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68266400
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68270000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68280000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68290000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682A0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682B0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682C0100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682D0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682E0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682F0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68310B00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68320B00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68330B00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68340B00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68350000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68360000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68370000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68388000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x6839D400
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683A0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683B0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683C0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683D0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683EC000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68500000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68510000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68520000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68530000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68540000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68550000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x686B5200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x686C6500
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x686D7600
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x686E3100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x686F2000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68702000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68712000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68722000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x688A0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x688B0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x688C0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x688D0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x688E0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x688F0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68900000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68910000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x6894B000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68960200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68970200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68990200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x689DFA00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x689E0100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x689F0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68A9CC00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68AA0400
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68AB0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68B7FF00
my_cntl write 0x916c 0x00000001

#set page 3
my_cntl write 0x9168 0x68010300
my_cntl write 0x916c 0x00000001

#page 3 registers
my_cntl write 0x9168 0x68020000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68030000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68040000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68050000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68061100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68070000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68080000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68090000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680A0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680B8000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680C0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680D0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680E0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680F0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68100000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68110000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68120000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68130000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68140000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68150000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68160000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68170000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68380000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68391F00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683B0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683C0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683D0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683E0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683F0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68400000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68410000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68420000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68430000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68440000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68450000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68460000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68590000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x685A0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x685B0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x685C0000
my_cntl write 0x916c 0x00000001

#set page 4
my_cntl write 0x9168 0x68010400
my_cntl write 0x916c 0x00000001

#page 4 registers
my_cntl write 0x9168 0x68870100
my_cntl write 0x916c 0x00000001

#set page 5
my_cntl write 0x9168 0x68010500
my_cntl write 0x916c 0x00000001

#page 5 registers
my_cntl write 0x9168 0x68081000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68091F00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680A0C00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680B0B00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680C3F00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680D3F00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680E1300
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680F2700
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68100900
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68110800
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68123F00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68133F00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68150000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68160000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68170000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68180000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x6819A800
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681A0200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681B0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681C0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681D0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681E0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681F8000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68212B00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682A0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682B0100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682C8700
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682D0300
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682E1900
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682F1900
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68310000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68324200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68330300
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68340000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68350000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68360000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68370000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68380000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68390000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683A0200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683B0300
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683C0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683D1100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683E0600
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68890D00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x688A0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x689BFA00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x689D1000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x689E2100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x689F0C00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68A00B00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68A13F00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68A23F00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68A60300
my_cntl write 0x916c 0x00000001

#set page 8
my_cntl write 0x9168 0x68010800
my_cntl write 0x916c 0x00000001

#page 8 registers
my_cntl write 0x9168 0x68023500
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68030500
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68040000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68050000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68060000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68070000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68080000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68090000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680A0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680B0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680C0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680D0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680E0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x680F0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68100000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68110000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68120000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68130000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68140000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68150000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68160000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68170000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68180000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68190000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681A0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681B0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681C0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681D0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681E0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681F0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68200000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68210000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68220000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68230000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68240000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68250000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68260000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68270000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68280000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68290000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682A0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682B0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682C0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682D0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682E0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x682F0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68300000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68310000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68320000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68330000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68340000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68350000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68360000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68370000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68380000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68390000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683A0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683B0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683C0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683D0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683E0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x683F0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68400000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68410000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68420000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68430000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68440000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68450000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68460000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68470000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68480000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68490000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x684A0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x684B0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x684C0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x684D0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x684E0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x684F0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68500000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68510000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68520000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68530000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68540000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68550000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68560000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68570000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68580000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68590000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x685A0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x685B0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x685C0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x685D0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x685E0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x685F0000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68600000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68610000
my_cntl write 0x916c 0x00000001

#set page 9
my_cntl write 0x9168 0x68010900
my_cntl write 0x916c 0x00000001

#page 9 registers
my_cntl write 0x9168 0x680E0200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68430100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68490F00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x684A0F00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x684E4900
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x684F0200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x685E0000
my_cntl write 0x916c 0x00000001

#set page A
my_cntl write 0x9168 0x68010A00
my_cntl write 0x916c 0x00000001

#page A registers
my_cntl write 0x9168 0x68020000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68030100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68040100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68050100
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68140000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x681A0000
my_cntl write 0x916c 0x00000001

#set page B
my_cntl write 0x9168 0x68010B00
my_cntl write 0x916c 0x00000001

#page B registers
my_cntl write 0x9168 0x68442F00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68460000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68470000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68480000
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x684A0200
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68570E00
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68580100
my_cntl write 0x916c 0x00000001


# End configuration registers
# 
# Start configuration postamble
#set page 5
my_cntl write 0x9168 0x68010500
my_cntl write 0x916c 0x00000001

#page 5 registers
my_cntl write 0x9168 0x68140100
my_cntl write 0x916c 0x00000001

#set page 0
my_cntl write 0x9168 0x68010000
my_cntl write 0x916c 0x00000001

#page 0 registers
my_cntl write 0x9168 0x681C0100
my_cntl write 0x916c 0x00000001

#set page 5
my_cntl write 0x9168 0x68010500
my_cntl write 0x916c 0x00000001

#page 5 registers
my_cntl write 0x9168 0x68400000
my_cntl write 0x916c 0x00000001

#set page B
my_cntl write 0x9168 0x68010B00
my_cntl write 0x916c 0x00000001

#page B registers
my_cntl write 0x9168 0x6824C300
my_cntl write 0x916c 0x00000001

my_cntl write 0x9168 0x68250200
my_cntl write 0x916c 0x00000001

# End configuration postamble



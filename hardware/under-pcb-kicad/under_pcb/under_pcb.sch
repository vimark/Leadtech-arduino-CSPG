EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Connector_Generic:Conn_02x18_Odd_Even J5
U 1 1 5E4CABE2
P 5200 4850
F 0 "J5" V 5296 5729 50  0000 L CNN
F 1 "Conn_02x18_Odd_Even" V 5205 5729 50  0000 L CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_2x18_P2.54mm_Vertical" H 5200 4850 50  0001 C CNN
F 3 "~" H 5200 4850 50  0001 C CNN
	1    5200 4850
	0    -1   -1   0   
$EndComp
$Comp
L Connector_Generic:Conn_01x08 J1
U 1 1 5E4CE9DC
P 6100 3950
F 0 "J1" V 6350 3950 50  0000 C CNN
F 1 "Conn_01x08" V 6250 3950 50  0000 C CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_1x08_P2.54mm_Vertical" H 6100 3950 50  0001 C CNN
F 3 "~" H 6100 3950 50  0001 C CNN
	1    6100 3950
	-1   0    0    1   
$EndComp
$Comp
L Connector_Generic:Conn_01x08 J2
U 1 1 5E4CF89D
P 6100 3000
F 0 "J2" V 6350 3000 50  0000 C CNN
F 1 "Conn_01x08" V 6250 3000 50  0000 C CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_1x08_P2.54mm_Vertical" H 6100 3000 50  0001 C CNN
F 3 "~" H 6100 3000 50  0001 C CNN
	1    6100 3000
	-1   0    0    1   
$EndComp
$Comp
L Connector_Generic:Conn_01x08 J3
U 1 1 5E4D1069
P 4400 3850
F 0 "J3" H 4480 3842 50  0000 L CNN
F 1 "Conn_01x08" H 4480 3751 50  0000 L CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_1x08_P2.54mm_Vertical" H 4400 3850 50  0001 C CNN
F 3 "~" H 4400 3850 50  0001 C CNN
	1    4400 3850
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x08 J6
U 1 1 5E4E81BF
P 7800 2500
F 0 "J6" V 8050 2550 50  0000 R CNN
F 1 "Conn_01x08" V 7950 2750 50  0000 R CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_1x08_P2.54mm_Vertical" H 7800 2500 50  0001 C CNN
F 3 "~" H 7800 2500 50  0001 C CNN
	1    7800 2500
	0    -1   -1   0   
$EndComp
$Comp
L Connector_Generic:Conn_01x04 J7
U 1 1 5E4E9BB6
P 5150 1600
F 0 "J7" V 5114 1312 50  0000 R CNN
F 1 "Conn_01x04" V 5023 1312 50  0000 R CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_1x04_P2.54mm_Vertical" H 5150 1600 50  0001 C CNN
F 3 "~" H 5150 1600 50  0001 C CNN
	1    5150 1600
	0    -1   -1   0   
$EndComp
Wire Wire Line
	6300 3200 8100 3200
Wire Wire Line
	8100 3200 8100 2700
$Comp
L power:GND #PWR0101
U 1 1 5E4EFC96
P 8000 2900
F 0 "#PWR0101" H 8000 2650 50  0001 C CNN
F 1 "GND" H 8005 2727 50  0000 C CNN
F 2 "" H 8000 2900 50  0001 C CNN
F 3 "" H 8000 2900 50  0001 C CNN
	1    8000 2900
	1    0    0    -1  
$EndComp
Wire Wire Line
	8000 2700 8000 2900
Wire Wire Line
	6300 3100 7500 3100
Wire Wire Line
	7500 3100 7500 2700
Wire Wire Line
	6300 2800 7600 2800
Wire Wire Line
	7600 2800 7600 2700
Wire Wire Line
	6300 3000 7700 3000
Wire Wire Line
	7700 3000 7700 2700
Wire Wire Line
	6300 2900 7800 2900
Wire Wire Line
	7800 2900 7800 2700
$Comp
L power:+5V #PWR0102
U 1 1 5E4F2CA2
P 8400 2700
F 0 "#PWR0102" H 8400 2550 50  0001 C CNN
F 1 "+5V" H 8415 2873 50  0000 C CNN
F 2 "" H 8400 2700 50  0001 C CNN
F 3 "" H 8400 2700 50  0001 C CNN
	1    8400 2700
	1    0    0    -1  
$EndComp
Wire Wire Line
	8200 2700 8400 2700
$Comp
L Connector_Generic:Conn_01x06 J4
U 1 1 5E4D205D
P 4400 3100
F 0 "J4" H 4318 2575 50  0000 C CNN
F 1 "Conn_01x06" H 4318 2666 50  0000 C CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_1x06_P2.54mm_Vertical" H 4400 3100 50  0001 C CNN
F 3 "~" H 4400 3100 50  0001 C CNN
	1    4400 3100
	1    0    0    1   
$EndComp
$Comp
L power:GND #PWR0103
U 1 1 5E4F551F
P 4000 3250
F 0 "#PWR0103" H 4000 3000 50  0001 C CNN
F 1 "GND" H 4005 3077 50  0000 C CNN
F 2 "" H 4000 3250 50  0001 C CNN
F 3 "" H 4000 3250 50  0001 C CNN
	1    4000 3250
	1    0    0    -1  
$EndComp
Wire Wire Line
	4200 3100 4000 3100
Wire Wire Line
	4000 3100 4000 3200
Wire Wire Line
	4200 3200 4000 3200
Connection ~ 4000 3200
Wire Wire Line
	4000 3200 4000 3250
$Comp
L power:+5V #PWR0104
U 1 1 5E4F69D7
P 4000 3000
F 0 "#PWR0104" H 4000 2850 50  0001 C CNN
F 1 "+5V" H 4015 3173 50  0000 C CNN
F 2 "" H 4000 3000 50  0001 C CNN
F 3 "" H 4000 3000 50  0001 C CNN
	1    4000 3000
	1    0    0    -1  
$EndComp
Wire Wire Line
	4200 3000 4000 3000
$EndSCHEMATC

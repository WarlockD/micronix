Ù
Copyright (c) 1983, Morrow Designs, Inc.
This program swaps physical character devices
and is intended for Morrow Multio and Wunderbuss I/O only.* . $~<��#~�6�.n%~��###~����:] � ���o�>	����>����>����>����>* o#~#fo+~��_ !^#V	� �a�!�_�K�,͌�X�L�7ͳ�X�C�B���X�P�M�r�X�M��͌�r!m ~� �f6 �* 6 ��* .^#V�.^#V.s#r.�s#r�* .^#V��#^#V* .s#r.
^#V.s#r.
�s#r�* .^#V��#^#V* ..s#r.^#V.s#r�.s#r�* .^#V��#^#V* ..s#r.^#V.s#r.�s#r�ZA-�Console Keyboard$Console Screen$Serial Port 3 Input$Centronics Parallel Port$Serial Port 3 Output$
Current KBD: is $
Current CRT: is $
Current RDR: is $
Current PUN: is $
Current LST: is $





















The SWAP program will re-assign devices as follows:
"K" swaps the current KBD: and RDR: input devices,
"C" swaps the current CRT: and LST: output devices,
"L" swaps the current LST: and PUN: output devices,
"P" swaps the current PUN: and CRT: output devices
"M" swaps both the current KBD/CRT: and RDR/PUN: pairs.

SWAP will accept one or two choices at a time.

Following cold boot, "SWAP L M" will transfer the console
to Serial Port 3, (for a modem), and places the LST: device
on the Centronics Port

Here is the current status:$                                                                                                                         
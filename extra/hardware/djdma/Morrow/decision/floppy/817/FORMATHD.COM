1^���
���
�
��  �C�~�D�.�F�@�L�%�	̈́͢͐>2:���`}��
�
�  �Y�c�N�D�R�c�Q>2:���:���uY�1w�1���
�
��  �1ڄ�8҄�02�����
�
��  �1ڢ�5Ң�12���
�
��  �Aڿ�Dҿ�A2��>2���>2�ɯ2�5��
�
��  �F��M��>2����
�
��  �1��͋4�4:��3�44���1_ ͋!`�U:��!d�U!g~2�#~2�� @� z� b��\��
�2��
��  �H��D�k>2��:���:��2��R>�P�!  �P� ��(��
�P� ��+|�°°��1!  @�P�G�P����#�P����"��P���1>�P>�P>�P�0�P��1! " 6�#x���P���X�P��"�P��͋����@:���*�+|�#+�P�M�C�n��
�!  "�"��2�>22�Y
:��p/��G:�2��R!>�u
�o�!͋ ʣ>�Q~#�S©>�Q�P�ʵ£!�͋ ��>�Q�P���>�Q�S�S�Sw#�����P��1!�͋��T��5#�!��
�E͡
��	��g̀�%��g����}����
���! ͋ �R^#V#{�����! �@%�m%��
#��
��++ �}�<<�͋!ʏ�>2:��FB��
:��F8��
:�FJ��
����I%��
�� �@* ���@�1*:�w#:�w#:w#6�">2�����R:��> 2�%��
����
�!  "�"��2�<2�͡
:��> �?>�2�>2 2�Y
�o! �͋>�^>�u
�:��w���n���<҈���i��:�����
��:��w� �b���n>�Q�P�ʠ�P���Q��> �Q�S�S:�G���S���,�S��,�S��,�S��,����� ! ��
�M�E��1̀�%��1������:�����
��:�<2��͋!�2�ؐ<2�=�> �Q ~�S#�C�C>�Q�P��S�P�7��Q�7��P�7�?�>�Q!�~�S#�w�*6�#:�w#O�:w#:w#6�:���2�:��ʬ>2�>�2�͋>ʾ>2��͡
�n! �<�͋> ��>2�͡
�n! �<��2��2�2�2�� y2��X�P���0���
@�P�G�P����͢��k͐���
�rY�1͢�k͐�RY�1���
!  "�"�͋>��k>�2��2���2�:�2�O �%��
> �Q�>��S�S�S�S͡
��	�P��1:������͋��:������:����:�����͋��:���������n���O� �¯��%:�G:���!�4͋>�!:���w�!�͋>��2>�4���A ���:���#��
:��?�%��
x�Gy��:�������R��R�h���
>
2�:�=2���A!�^#V#{�ʀ��͠�Î���2		  �2�2�2�<2�͋>���>�2��O	͋>���>�2�2��2�=2�û�2�<2�2�<2�͋>���>�2�2�<<2�>�2�2��O	�2�2�2�2�͋>��,	>�2��O	>�2�<2�<2�͋>��I	>�2�2���	�o:�O:��2��|	:�O:��2��|	:�=2��R	�G:���x2�� ڐ	/</<O�>�Q�P�ʚ	>�Q�S�S�S�SG:����[��
:��F|��
�x�F���
��2��>2�:��>���	�2��͋ $
��	D
!�w++>�Q~�S#�
>�Q�P��
��	�	
 	
! ͋  �h
 6 #6 #x��h
�2 ~�͋ ʄ
���! V�w#/
<
�<
�!~G:�2���/���2��R��?o&  ^#Vr+s�:�~��
:�> 2~��
>2~///=<w��:�������	� %
� :&�>�:'�a��{�� �
 ��
���
�
�  |��}��o& ���PX	�O��|��L�>0������_� ������ ��|��
� �
�Y�  %��
Discus M26 and M10 Hard Disk Format Program Rev. 2.2$
Enter Physical Drive Number to be Tested or Formated (1 - 4, RETURN to exit): $
Enter Logical Drive Number to be Formated (1 - 3, RETURN to exit): $
Enter Amount of Formatting Desired:
	H = Format Headers Only (Data Remains Intact).
	D = Erase Data Field Also.
Function: $
Drive is not ready.$
Controller not halted.$
Write fault on drive.$.$
$
Waiting for drive to become ready, could take as long as 2 minutes.$
Testing sector headers, will take about 30 minutes.
$
Formatting the entire physical disk, will take about 4 minutes.$
Formatting a logical drive, will take about 1.5 minutes.$Sector Error, Track $, Sector $, Head $, Error count $
All Finished, Returning to CP/M.$
Want to abort ? (Yes or No): $
Testing sector data field, will take about 12-14 Hours.
$
Error Writing Sector Map to Track Zero.$
Bad Sector Map Overflow.$
Testing Track Seek Mechanism, will take about 15 minutes.
$
Seek error, Looking For Track $, Found Track $.
$
Unknown Command.$
Bad Drive Number.$
Choose The Desired Function:
	L = Format a Logical Drive.
	F = Format an Entire Physical Drive.
	C = Continue An Interupted Test.
	D = Run a Diagnostic Test.
Function (RETURN to Exit): $
Not Yet Implemented.$
Select The Drive Type:
	A = Discus M26, 26 Megabyte Drive.
	B = Discus M10, 10 Megabyte Drive.
	C = Discus M20, 20 Megabyte Drive.
Drive type (RETURN to Exit): $
Select drive type:
	F = Fujitsu 2301
	M = Memorex 101
Type (RETURN to Exit): $
How Much of the Diagnostic Do You Want to Run: 
	1 = Sector Header Field Test Only.
	2 = Sector Data Field Test Only.
	4 = Seek Mechanism Test Only.
Choose the Diagnostic by Adding together the Desired Options.
Options (RETURN to Exit): $
Has the Drive Been Preformated ? (Y or N, RETURN to Exit): $          $
Press RETURN to return to CP/M: $                st O �     �                                                                                                                          
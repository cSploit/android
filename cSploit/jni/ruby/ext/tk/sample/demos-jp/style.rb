# -*- coding: euc-jp -*-
#
# text (display styles) widget demo (called by 'widget')
#

# toplevel widget ��¸�ߤ���к������
if defined?($style_demo) && $style_demo
  $style_demo.destroy
  $style_demo = nil
end


# demo �Ѥ� toplevel widget ������
$style_demo = TkToplevel.new {|w|
  title("Text Demonstration - Display Styles")
  iconname("style")
  positionWindow(w)
}

base_frame = TkFrame.new($style_demo).pack(:fill=>:both, :expand=>true)

# frame ����
TkFrame.new(base_frame) {|frame|
  TkButton.new(frame) {
    #text 'λ��'
    text '�Ĥ���'
    command proc{
      tmppath = $style_demo
      $style_demo = nil
      tmppath.destroy
    }
  }.pack('side'=>'left', 'expand'=>'yes')

  TkButton.new(frame) {
    text '�����ɻ���'
    command proc{showCode 'style'}
  }.pack('side'=>'left', 'expand'=>'yes')
}.pack('side'=>'bottom', 'fill'=>'x', 'pady'=>'2m')


# text ����
txt = TkText.new(base_frame){|t|
  # ����
  setgrid 'true'
  #width  70
  #height 32
  wrap 'word'
  font $font
  TkScrollbar.new(base_frame) {|s|
    pack('side'=>'right', 'fill'=>'y')
    command proc{|*args| t.yview(*args)}
    t.yscrollcommand proc{|first,last| s.set first,last}
  }
  pack('expand'=>'yes', 'fill'=>'both')

  # �ƥ����ȥ������� (�ե���ȴ�Ϣ)
  family = 'Courier'

  if $tk_version =~ /^4.*/
    style_tag_bold = TkTextTag.new(t, 'font'=>'-*-Courier-Bold-O-Normal--*-120-*-*-*-*-*-*')
    style_tag_big = TkTextTag.new(t, 'font'=>'-*-Courier-Bold-R-Normal--*-140-*-*-*-*-*-*', 'kanjifont'=>$msg_kanji_font)
    style_tag_verybig = TkTextTag.new(t, 'font'=>'-*-Helvetica-Bold-R-Normal--*-240-*-*-*-*-*-*')
    # style_tag_small = TkTextTag.new(t, 'font'=>'-Adobe-Helvetica-Bold-R-Normal-*-100-*', 'kanjifont'=>$kanji_font)
    style_tag_small = TkTextTag.new(t, 'font'=>'-Adobe-Helvetica-Bold-R-Normal-*-100-*')
  else
    style_tag_bold = TkTextTag.new(t, 'font'=>[family, 12, :bold, :italic])
    style_tag_big = TkTextTag.new(t, 'font'=>[family, 14, :bold])
    style_tag_verybig = TkTextTag.new(t, 'font'=>['Helvetica', 24, :bold])
    style_tag_small = TkTextTag.new(t, 'font'=>'Times 8 bold')
  end
###
#  case($tk_version)
#  when /^4.*/
#    style_tag_big = TkTextTag.new(t, 'font'=>'-*-Courier-Bold-R-Normal--*-140-*-*-*-*-*-*', 'kanjifont'=>$msg_kanji_font)
#    style_tag_small = TkTextTag.new(t, 'font'=>'-Adobe-Helvetica-Bold-R-Normal-*-100-*', 'kanjifont'=>$kanji_font)
#  when /^8.*/
#    unless $style_demo_do_first
#      $style_demo_do_first = true
#      Tk.tk_call('font', 'create', '@bigascii',
#                '-copy', '-*-Courier-Bold-R-Normal--*-140-*-*-*-*-*-*')
#      Tk.tk_call('font', 'create', '@smallascii',
#                '-copy', '-Adobe-Helvetica-Bold-R-Normal-*-100-*')
#      Tk.tk_call('font', 'create', '@cBigFont',
#                '-compound', '@bigascii @msg_knj')
#      Tk.tk_call('font', 'create', '@cSmallFont',
#                '-compound', '@smallascii @kanji')
#    end
#    style_tag_big = TkTextTag.new(t, 'font'=>'@cBigFont')
#    style_tag_small = TkTextTag.new(t, 'font'=>'@cSmallFont')
#  end

  # �ƥ����ȥ������� (������꡼�մ�Ϣ)
  if TkWinfo.depth($root).to_i > 1
    style_tag_color1 = TkTextTag.new(t, 'background'=>'#a0b7ce')
    style_tag_color2 = TkTextTag.new(t, 'foreground'=>'red')
    style_tag_raised = TkTextTag.new(t, 'relief'=>'raised', 'borderwidth'=>1)
    style_tag_sunken = TkTextTag.new(t, 'relief'=>'sunken', 'borderwidth'=>1)
  else
    style_tag_color1 = TkTextTag.new(t, 'background'=>'black',
                                     'foreground'=>'white')
    style_tag_color2 = TkTextTag.new(t, 'background'=>'black',
                                     'foreground'=>'white')
    style_tag_raised = TkTextTag.new(t, 'background'=>'white',
                                     'relief'=>'raised', 'borderwidth'=>1)
    style_tag_sunken = TkTextTag.new(t, 'background'=>'white',
                                     'relief'=>'sunken', 'borderwidth'=>1)
  end

  # �ƥ����ȥ������� (����¾)
  if $tk_version =~ /^4\.[01]/
    style_tag_bgstipple = TkTextTag.new(t, 'background'=>'black',
                                        'borderwidth'=>0,
                                        'bgstipple'=>'gray25')
  else
    style_tag_bgstipple = TkTextTag.new(t, 'background'=>'black',
                                        'borderwidth'=>0,
                                        'bgstipple'=>'gray12')
  end
  style_tag_fgstipple = TkTextTag.new(t, 'fgstipple'=>'gray50')
  style_tag_underline = TkTextTag.new(t, 'underline'=>'on')
  style_tag_overstrike = TkTextTag.new(t, 'overstrike'=>'on')
  style_tag_right  = TkTextTag.new(t, 'justify'=>'right')
  style_tag_center = TkTextTag.new(t, 'justify'=>'center')
  if $tk_version =~ /^4.*/
    style_tag_super = TkTextTag.new(t, 'offset'=>'4p', 'font'=>'-Adobe-Courier-Medium-R-Normal--*-100-*-*-*-*-*-*')
    style_tag_sub = TkTextTag.new(t, 'offset'=>'-2p', 'font'=>'-Adobe-Courier-Medium-R-Normal--*-100-*-*-*-*-*-*')
  else
    style_tag_super = TkTextTag.new(t, 'offset'=>'4p', 'font'=>[family, 10])
    style_tag_sub = TkTextTag.new(t, 'offset'=>'-2p', 'font'=>[family, 10])
  end
  style_tag_margins = TkTextTag.new(t, 'lmargin1'=>'12m', 'lmargin2'=>'6m',
                                    'rmargin'=>'10m')
  style_tag_spacing = TkTextTag.new(t, 'spacing1'=>'10p', 'spacing2'=>'2p',
                                    'lmargin1'=>'12m', 'lmargin2'=>'6m',
                                    'rmargin'=>'10m')

  # �ƥ���������
  insert('end', '���Τ褦�˥ƥ����� widget �Ͼ�����͡��ʥ��������ɽ�����뤳��
���Ǥ��ޤ���')
  insert('end', '����', style_tag_big)
  insert('end', '�Ȥ����ᥫ�˥���ǥ���ȥ��뤵��ޤ���
�����Ȥϥƥ����� widget ��Τ���ʸ�� (���ϰ�)���Ф���Ŭ�ѤǤ���
ñ�ʤ�̾���Τ��ȤǤ����������͡���ɽ���������������Ǥ��ޤ���
���ꤹ��ȡ����Υ����ΤĤ���ʸ���ϻ��ꤷ�����������ɽ�������
�褦�ˤʤ�ޤ������ѤǤ���ɽ����������ϼ����̤�Ǥ���
')
  insert('end', '
1. �ե����', style_tag_big)
  insert('end', '    �ɤ�� X �Υե���ȤǤ�Ȥ��ޤ���')
  insert('end', 'large', style_tag_verybig)
  insert('end', '
�Ȥ�')
#  insert('end', '������', style_tag_small)
  insert('end', 'small', style_tag_small)
  insert('end', '�Ȥ���
')
  insert('end', '
2. ��', style_tag_big)
  insert('end', '  ')
  insert('end', '�طʿ�', style_tag_color1)
  insert('end', '��')
  insert('end', '���ʿ�', style_tag_color2)
  insert('end', '��')
  insert('end', 'ξ��', style_tag_color1, style_tag_color2)
  insert('end', '�Ȥ��Ѥ��뤳�Ȥ��Ǥ��ޤ���
')
  insert('end', '
3. �֤���', style_tag_big)
  insert('end', '  ���Τ褦������κݤ�')
  insert('end', '�طʤ�', style_tag_bgstipple)
  insert('end', 'ʸ����', style_tag_fgstipple)
  insert('end', 'ñ�ʤ��ɤ�Ĥ֤�
�Ǥʤ����֤�����Ȥ����Ȥ��Ǥ��ޤ���
')
  insert('end', '
4. ����', style_tag_big)
  insert('end', '  ���Τ褦��')
  insert('end', 'ʸ���˲��������', style_tag_underline)
  insert('end', '���Ȥ��Ǥ��ޤ���
')
  insert('end', '
5. �Ǥ��ä���', style_tag_big)
  insert('end', '  ���Τ褦��')
  insert('end', 'ʸ���˽Ťͤ��������', style_tag_overstrike)
  insert('end', '���Ȥ��Ǥ��ޤ���
')
  insert('end', '
6. 3D ����', style_tag_big)
  insert('end', '  �طʤ��Ȥ�Ĥ��ơ�ʸ����')
  insert('end', '���ӽФ�', style_tag_raised)
  insert('end', '�褦�ˤ�����')
  insert('end', '����', style_tag_sunken)
  insert('end', '
�褦�ˤǤ��ޤ���
')
  insert('end', '
7. ��·��', style_tag_big)
  insert('end', ' ���Τ褦�˹Ԥ�
')
  insert('end', '����·������
')
  insert('end', '����·������
', style_tag_right)
  insert('end', '�����·������Ǥ��ޤ���
', style_tag_center)
  insert('end', '
8. ���դ�ʸ����ź��', style_tag_big)
  insert('end', '  10')
  insert('end', 'n', style_tag_super)
  insert('end', ' �Τ褦�˸��դ�ʸ���θ��̤䡢')
  insert('end', '
X')
  insert('end', 'i', style_tag_sub)
  insert('end', '�Τ褦��ź���θ��̤�Ф����Ȥ��Ǥ��ޤ���
')
  insert('end', '
9. �ޡ�����', style_tag_big)
  insert('end', '�ƥ����Ȥκ�¦��;ʬ�ʶ�����֤����Ȥ��Ǥ��ޤ�:
')
  insert('end', '��������ϥޡ�����λ�����Ǥ��������꡼��',
         style_tag_margins)
  insert('end', '����ޤ��֤����ɽ������Ƥ���1�ԤΥƥ����ȤǤ���',
         style_tag_margins)
  insert('end', '��¦�ˤ�2����Υޡ����������ޤ���', style_tag_margins)
  insert('end', '1���ܤ��Ф����Τȡ�', style_tag_margins)
  insert('end', '2���ܰʹߤ�Ϣ³�����ޡ�����', style_tag_margins)
  insert('end', '�Ǥ����ޤ���¦�ˤ�ޡ����󤬤���ޤ���', style_tag_margins)
  insert('end', '�Ԥ��ޤ��֤����֤���뤿��˻��Ѥ��뤳�Ȥ��Ǥ��ޤ���
', style_tag_margins)
  insert('end', '
10. ���ڡ�����', style_tag_big)
  insert('end', '3�ĤΥѥ�᡼���ǹԤΥ��ڡ����󥰤�')
  insert('end', '���椹
�뤳�Ȥ��Ǥ��ޤ���Spacing1�ǡ��Ԥ�')
  insert('end', '��ˤɤΤ��餤�ζ��֤��֤�����
spacing3')
  insert('end', '�ǹԤβ��ˤɤΤ��餤�ζ��֤��֤�����')
  insert('end', '�Ԥ��ޤ��֤���Ƥ���ʤ�
�С�spacing2�ǡ�')
  insert('end', '�ƥ����ȹԤ��������Ƥ���Ԥδ֤ˤɤΤ��餤')
  insert('end', '�ζ��֤���
�����򼨤��ޤ���
')
  insert('end', '�����Υ���ǥ�Ȥ��줿����ϤɤΤ褦��',
         style_tag_spacing)
  insert('end', '���ڡ����󥰤����Ԥ���Τ��򼨤��ޤ���',
         style_tag_spacing)
  insert('end', '������ϼºݤϥƥ�����widget', style_tag_spacing)
  insert('end', '��1�Ԥǡ�widget�ˤ�ä��ޤ���ޤ�Ƥ��ޤ���
', style_tag_spacing)
  insert('end', 'Spacing1�Ϥ��Υƥ����ȤǤ�10point��', style_tag_spacing)
  insert('end', '���ꤵ��Ƥ��ޤ���', style_tag_spacing)
  insert('end', '����ˤ�ꡢ����δ֤��礭�ʴֳ֤�', style_tag_spacing)
  insert('end', '¸�ߤ��Ƥ��ޤ���', style_tag_spacing)
  insert('end', 'Spacing2��2point�����ꤵ��Ƥ��ޤ���', style_tag_spacing)
  insert('end', '������������ˤۤ�ξ����ֳ֤�¸�ߤ��Ƥ��ޤ���',
         style_tag_spacing)
  insert('end', 'Spacing3�Ϥ�����Ǥϻ��Ѥ���Ƥ��ޤ���
', style_tag_spacing)
  insert('end', '�ֳ֤��ɤ��ˤ��뤫�򸫤�����С������������',
         style_tag_spacing)
  insert('end', '�ʤ��ǥƥ����Ȥ����򤷤Ƥ��������������', style_tag_spacing)
  insert('end', 'ȿž������ʬ�ˤ�;ʬ�ˤȤ�줿�ֳ֤�', style_tag_spacing)
  insert('end', '�ޤޤ�Ƥ��ޤ���
', style_tag_spacing)

}

txt.width 70
txt.height 32

# -*- coding: euc-jp -*-
#
# message boxes widget demo (called by 'widget')
#

# toplevel widget ��¸�ߤ���к������
if defined?($msgbox2_demo) && $msgbox2_demo
  $msgbox2_demo.destroy
  $msgbox2_demo = nil
end

# demo �Ѥ� toplevel widget ������
$msgbox2_demo = TkToplevel.new {|w|
  title("Message Box Demonstration")
  iconname("messagebox")
  positionWindow(w)
}

base_frame = TkFrame.new($msgbox2_demo).pack(:fill=>:both, :expand=>true)

# label ����
TkLabel.new(base_frame, 'font'=>$font, 'wraplength'=>'4i', 'justify'=>'left',
            'text'=>"�ޤ�ɽ�����륢������ȥ�å������ܥå����μ��������ǲ����������θ��\"��å������ܥå���\"�ܥ���򲡤��ȡ����ꤵ�줿�����ǡ���å������Ⱦܺ٥ƥ����ȤȤ���ä���å������ܥå�����ɽ������ޤ���").pack('side'=>'top')

# frame ����
TkFrame.new(base_frame) {|frame|
  TkButton.new(frame) {
    #text 'λ��'
    text '�Ĥ���'
    command proc{
      tmppath = $msgbox2_demo
      $msgbox2_demo = nil
      tmppath.destroy
    }
  }.pack('side'=>'left', 'expand'=>'yes')

  TkButton.new(frame) {
    text '�����ɻ���'
    command proc{showCode 'msgbox2'}
  }.pack('side'=>'left', 'expand'=>'yes')

  TkButton.new(frame) {
    text '��å������ܥå���'
    command proc{showMessageBox2 $msgbox2_demo}
  }.pack('side'=>'left', 'expand'=>'yes')
}.pack('side'=>'bottom', 'fill'=>'x', 'pady'=>'2m')

# frame ����
$msgbox_leftframe  = TkFrame.new(base_frame)
$msgbox_rightframe = TkFrame.new(base_frame)
$msgbox_leftframe .pack('side'=>'left', 'expand'=>'yes', 'fill'=>'y',
                        'pady'=>'.5c', 'padx'=>'.5c')
$msgbox_rightframe.pack('side'=>'left', 'expand'=>'yes', 'fill'=>'y',
                        'pady'=>'.5c', 'padx'=>'.5c')

TkLabel.new($msgbox_leftframe, 'text'=>'��������').pack('side'=>'top')
TkFrame.new($msgbox_leftframe, 'relief'=>'ridge', 'bd'=>1, 'height'=>2)\
.pack('side'=>'top', 'fill'=>'x', 'expand'=>'no')

$msgboxIcon = TkVariable.new('info')
['error', 'info', 'question', 'warning'].each {|icon|
  TkRadioButton.new($msgbox_leftframe, 'text'=>icon, 'variable'=>$msgboxIcon,
                    'relief'=>'flat', 'value'=>icon, 'width'=>16,
                    'anchor'=>'w').pack('side'=>'top', 'pady'=>2,
                                        'anchor'=>'w', 'fill'=>'x')
}

TkLabel.new($msgbox_rightframe, 'text'=>'����').pack('side'=>'top')
TkFrame.new($msgbox_rightframe, 'relief'=>'ridge', 'bd'=>1, 'height'=>2)\
.pack('side'=>'top', 'fill'=>'x', 'expand'=>'no')

$msgboxType = TkVariable.new('ok')
['abortretryignore', 'ok', 'okcancel',
  'retrycancel', 'yesno', 'yesnocancel'].each {|type|
  TkRadioButton.new($msgbox_rightframe, 'text'=>type, 'variable'=>$msgboxType,
                    'relief'=>'flat', 'value'=>type, 'width'=>16,
                    'anchor'=>'w').pack('side'=>'top', 'pady'=>2,
                                        'anchor'=>'w', 'fill'=>'x')
}

def showMessageBox2(w)
  button = Tk.messageBox('icon'=>$msgboxIcon.value, 'type'=>$msgboxType.value,
                         'title'=>'Message', 'parent'=>w,
                         'message'=>"\"#{$msgboxType.value}\"�����פΥ�å������ܥå���",
                         'detail'=>"�����\"#{$msgboxType.value}\"�Ȥ�������Υ�å������ܥå����ǡ�\"#{$msgboxIcon.value}\"�Υ�������ɽ������Ƥ��ޤ������Υܥ���Τ����줫�����򤷤ƥ���å����Ƥ���������")

  Tk.messageBox('icon'=>'info', 'type'=>'ok', 'parent'=>w,
                'message'=>"���ʤ��� \"#{button}\" �򲡤��ޤ����͡�")
end


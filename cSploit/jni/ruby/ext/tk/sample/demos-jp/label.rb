# -*- coding: euc-jp -*-
#
# label widget demo (called by 'widget')
#

# toplevel widget ��¸�ߤ���к������
if defined?($label_demo) && $label_demo
  $label_demo.destroy
  $label_demo = nil
end

# demo �Ѥ� toplevel widget ������
$label_demo = TkToplevel.new {|w|
  title("Label Demonstration")
  iconname("label")
  positionWindow(w)
}

base_frame = TkFrame.new($label_demo).pack(:fill=>:both, :expand=>true)

# label ����
msg = TkLabel.new(base_frame) {
  font $font
  wraplength '4i'
  justify 'left'
  text "���ˤ�5�ĤΥ�٥뤬ɽ������Ƥ��ޤ�����¦�ˤϥƥ����ȥ�٥뤬3�Ĥ��ꡢ��¦�ˤϥӥåȥޥåץ�٥�ȥƥ����ȥ�٥뤬����ޤ�����٥�Ȥ����ΤϤ��ޤ����򤤤�ΤǤϤ���ޤ��󡣤ʤ��ʤ�į���ʳ�����Ǥ��ʤ�����Ǥ���"
}
msg.pack('side'=>'top')

# frame ����
TkFrame.new(base_frame) {|frame|
  TkButton.new(frame) {
    #text 'λ��'
    text '�Ĥ���'
    command proc{
      tmppath = $label_demo
      $label_demo = nil
      tmppath.destroy
    }
  }.pack('side'=>'left', 'expand'=>'yes')

  TkButton.new(frame) {
    text '�����ɻ���'
    command proc{showCode 'label'}
  }.pack('side'=>'left', 'expand'=>'yes')

}.pack('side'=>'bottom', 'fill'=>'x', 'pady'=>'2m')

# label demo �ѥե졼������
f_left = TkFrame.new(base_frame)
f_right = TkFrame.new(base_frame)
[f_left, f_right].each{|w| w.pack('side'=>'left', 'expand'=>'yes',
                                  'padx'=>10, 'pady'=>10, 'fill'=>'both')}

# label ����
[ TkLabel.new(f_left, 'text'=>'�ǽ�Υ�٥�'),
  TkLabel.new(f_left, 'text'=>'2 ���ܡ�����ä��⤭�夬�餻�Ƥߤޤ���',
              'relief'=>'raised'),
  TkLabel.new(f_left, 'text'=>'3 ���ܡ�����Ǥ��ޤ� ', 'relief'=>'sunken')
].each{|w| w.pack('side'=>'top', 'expand'=>'yes', 'pady'=>2, 'anchor'=>'w')}

# TkLabel.new(f_right) {
Tk::Label.new(f_right) {
  bitmap('@' + [$demo_dir,'..','images','face.xbm'].join(File::Separator))
  borderwidth 2
  relief 'sunken'
}.pack('side'=>'top')

TkLabel.new(f_right) { text 'Tcl/Tk ��ͭ��' }.pack('side'=>'top')

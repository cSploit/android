# -*- coding: euc-jp -*-
#
# button widget demo (called by 'widget')
#
#

# toplevel widget ��¸�ߤ���к������
if defined?($button_demo) && $button_demo
  $button_demo.destroy
  $button_demo = nil
end

# demo �Ѥ� toplevel widget ������
$button_demo = TkToplevel.new {|w|
  title("Button Demonstration")
  iconname("button")
  positionWindow(w)
}

# label ����
msg = TkLabel.new($button_demo) {
  font $kanji_font
  wraplength '4i'
  justify 'left'
  text "�ܥ���򥯥�å�����ȡ��ܥ�����طʿ������Υܥ���˽񤫤�Ƥ��뿧�ˤʤ�ޤ����ܥ��󤫤�ܥ���ؤΰ�ư�ϥ��֤򲡤����ȤǤ��ǽ�Ǥ����ޤ����ڡ����Ǽ¹Ԥ��뤳�Ȥ��Ǥ��ޤ���"
}
msg.pack('side'=>'top')

# frame ����
$button_buttons = Tk::Frame.new($button_demo) {|frame|
  TkButton.new(frame) {
    #text 'λ��'
    text '�Ĥ���'
    command proc{
      tmppath = $button_demo
      $button_demo = nil
      tmppath.destroy
    }
  }.pack('side'=>'left', 'expand'=>'yes')

  TkButton.new(frame) {
    text '�����ɻ���'
    command proc{showCode 'button'}
  }.pack('side'=>'left', 'expand'=>'yes')

}.pack('side'=>'bottom', 'fill'=>'x', 'pady'=>'2m')

# button ����
TkButton.new($button_demo){
  text "Peach Puff"
  width 10
  command proc{
    $button_demo.configure('bg','PeachPuff1')
    $button_buttons.configure('bg','PeachPuff1')
  }
}.pack('side'=>'top', 'expand'=>'yes', 'pady'=>2)

TkButton.new($button_demo){
  text "Light Blue"
  width 10
  command proc{
    $button_demo.configure('bg','LightBlue1')
    $button_buttons.configure('bg','LightBlue1')
  }
}.pack('side'=>'top', 'expand'=>'yes', 'pady'=>2)

TkButton.new($button_demo){
  text "Sea Green"
  width 10
  command proc{
    $button_demo.configure('bg','SeaGreen2')
    $button_buttons.configure('bg','SeaGreen2')
  }
}.pack('side'=>'top', 'expand'=>'yes', 'pady'=>2)

TkButton.new($button_demo){
  text "Yellow"
  width 10
  command proc{
    $button_demo.configure('bg','Yellow1')
    $button_buttons.configure('bg','Yellow1')
  }
}.pack('side'=>'top', 'expand'=>'yes', 'pady'=>2)

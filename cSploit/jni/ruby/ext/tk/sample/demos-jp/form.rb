# -*- coding: euc-jp -*-
#
# form widget demo (called by 'widget')
#

# toplevel widget ��¸�ߤ���к������
if defined?($form_demo) && $form_demo
  $form_demo.destroy
  $form_demo = nil
end

# demo �Ѥ� toplevel widget ������
$form_demo = TkToplevel.new {|w|
  title("Form Demonstration")
  iconname("form")
  positionWindow(w)
}

base_frame = TkFrame.new($form_demo).pack(:fill=>:both, :expand=>true)

# label ����
msg = TkLabel.new(base_frame) {
  font $font
  wraplength '4i'
  justify 'left'
  text "���Υ�����ɥ��ϴ�ñ�ʥե����������ѤˤʤäƤ��ơ����ޤ��ޤʥ���ȥ�����Ϥ��Ǥ��ޤ������֤ǥ���ȥ�����ؤ����Ǥ��ޤ���"
}
msg.pack('side'=>'top', 'fill'=>'x')

# frame ����
TkFrame.new(base_frame) {|frame|
  TkButton.new(frame) {
    #text 'λ��'
    text '�Ĥ���'
    command proc{
      tmppath = $form_demo
      $form_demo = nil
      tmppath.destroy
    }
  }.pack('side'=>'left', 'expand'=>'yes')

  TkButton.new(frame) {
    text '�����ɻ���'
    command proc{showCode 'form'}
  }.pack('side'=>'left', 'expand'=>'yes')
}.pack('side'=>'bottom', 'fill'=>'x', 'pady'=>'2m')

# entry ����
form_data = []
(1..5).each{|i|
  f = TkFrame.new(base_frame, 'bd'=>2)
  e = TkEntry.new(f, 'relief'=>'sunken', 'width'=>40)
  l = TkLabel.new(f)
  e.pack('side'=>'right')
  l.pack('side'=>'left')
  form_data[i] = {'frame'=>f, 'entry'=>e, 'label'=>l}
}

# ʸ��������
form_data[1]['label'].text('̾��:')
form_data[2]['label'].text('����:')
form_data[5]['label'].text('����:')

# pack
(1..5).each{|i| form_data[i]['frame'].pack('side'=>'top', 'fill'=>'x')}


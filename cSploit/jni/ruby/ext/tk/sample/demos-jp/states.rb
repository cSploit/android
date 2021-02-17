# -*- coding: euc-jp -*-
#
# listbox widget demo 'states' (called by 'widget')
#

# toplevel widget ��¸�ߤ���к������
if defined?($states_demo) && $states_demo
  $states_demo.destroy
  $states_demo = nil
end

# demo �Ѥ� toplevel widget ������
$states_demo = TkToplevel.new {|w|
  title("Listbox Demonstration (states)")
  iconname("states")
  positionWindow(w)
}

base_frame = TkFrame.new($states_demo).pack(:fill=>:both, :expand=>true)

# label ����
msg = TkLabel.new(base_frame) {
  font $font
  wraplength '4i'
  justify 'left'
  text "���ˤ���Τ���ƻ�ܸ�̾�����ä���������С��դΥꥹ�ȥܥå����Ǥ����ꥹ�Ȥ򥹥����뤵����Τϥ�������С��Ǥ�Ǥ��ޤ������ꥹ�ȥܥå�������ǥޥ����Υܥ���2(��ܥ���)�򲡤����ޤޥɥ�å����Ƥ�Ǥ��ޤ���"
}
msg.pack('side'=>'top')

# frame ����
TkFrame.new(base_frame) {|frame|
  TkButton.new(frame) {
    #text 'λ��'
    text '�Ĥ���'
    command proc{
      tmppath = $states_demo
      $states_demo = nil
      tmppath.destroy
    }
  }.pack('side'=>'left', 'expand'=>'yes')

  TkButton.new(frame) {
    text '�����ɻ���'
    command proc{showCode 'states'}
  }.pack('side'=>'left', 'expand'=>'yes')

}.pack('side'=>'bottom', 'fill'=>'x', 'pady'=>'2m')

# frame ����
states_lbox = nil
TkFrame.new(base_frame, 'borderwidth'=>'.5c') {|w|
  s = TkScrollbar.new(w)
  states_lbox = TkListbox.new(w) {
    setgrid 1
    height 12
    yscrollcommand proc{|first,last| s.set first,last}
  }
  s.command(proc{|*args| states_lbox.yview(*args)})
  s.pack('side'=>'right', 'fill'=>'y')
  states_lbox.pack('side'=>'left', 'expand'=>1, 'fill'=>'both')
}.pack('side'=>'top', 'expand'=>'yes', 'fill'=>'y')

ins_data = [
  '����','�Ŀ�','����','����','���','���','��ɲ',
  '��ʬ','���','����','����','����','������','������',
  '����','����','����','����','����','���','����',
  '����','�Ų�','�纬','����','���','����','����',
  'Ļ��','�ٻ�','Ĺ��','Ĺ��','����','����','ʼ��',
  '����','ʡ��','ʡ��','ʡ��','�̳�ƻ','����','�ܾ�',
  '�ܺ�','����','����','����','�²λ�'
]

states_lbox.insert(0, *ins_data)


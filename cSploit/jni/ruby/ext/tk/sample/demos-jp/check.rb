# -*- coding: euc-jp -*-
#
# checkbutton widget demo (called by 'widget')
#

# toplevel widget ��¸�ߤ���к������
if defined?($check_demo) && $check_demo
  $check_demo.destroy
  $check_demo = nil
end

# demo �Ѥ� toplevel widget ������
$check_demo = TkToplevel.new {|w|
  title("Checkbutton Demonstration")
  iconname("check")
  positionWindow(w)
}

base_frame = TkFrame.new($check_demo).pack(:fill=>:both, :expand=>true)

# label ����
msg = TkLabel.new(base_frame) {
  font $font
  wraplength '4i'
  justify 'left'
  text "���ˤ� 3 �ĤΥ����å��ܥ���ɽ������Ƥ��ޤ�������å�����ȥܥ����������֤��Ѥ�ꡢTcl �ѿ� ( TkVariable ���֥������Ȥǥ��������Ǥ��ޤ� ) �ˤ��Υܥ���ξ��֤򼨤��ͤ����ꤷ�ޤ������ߤ��ѿ����ͤ򸫤�ˤϡ��ѿ����ȡץܥ���򥯥�å����Ƥ���������"
}
msg.pack('side'=>'top')

# �ѿ�����
wipers = TkVariable.new(0)
brakes = TkVariable.new(0)
sober  = TkVariable.new(0)

# frame ����
TkFrame.new(base_frame) {|frame|
  TkButton.new(frame) {
    #text 'λ��'
    text '�Ĥ���'
    command proc{
      tmppath = $check_demo
      $check_demo = nil
      $showVarsWin[tmppath.path] = nil
      tmppath.destroy
    }
  }.pack('side'=>'left', 'expand'=>'yes')

  TkButton.new(frame) {
    text '�����ɻ���'
    command proc{showCode 'check'}
  }.pack('side'=>'left', 'expand'=>'yes')


  TkButton.new(frame) {
    text '�ѿ�����'
    command proc{
      showVars(base_frame,
               ['wipers', wipers], ['brakes', brakes], ['sober', sober])
    }
  }.pack('side'=>'left', 'expand'=>'yes')

}.pack('side'=>'bottom', 'fill'=>'x', 'pady'=>'2m')


# checkbutton ����
[ TkCheckButton.new(base_frame, 'text'=>'�磻�ѡ� OK', 'variable'=>wipers),
  TkCheckButton.new(base_frame, 'text'=>'�֥졼�� OK', 'variable'=>brakes),
  TkCheckButton.new(base_frame, 'text'=>'��ž�� ����', 'variable'=>sober)
].each{|w| w.relief('flat'); w.pack('side'=>'top', 'pady'=>2, 'anchor'=>'w')}


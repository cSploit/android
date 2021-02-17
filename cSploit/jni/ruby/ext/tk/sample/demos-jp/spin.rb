# -*- coding: euc-jp -*-
#
# spin.rb --
#
# This demonstration script creates several spinbox widgets.
#
# based on Tcl/Tk8.4.4 widget demos

if defined?($spin_demo) && $spin_demo
  $spin_demo.destroy
  $spin_demo = nil
end

$spin_demo = TkToplevel.new {|w|
  title("Spinbox Demonstration")
  iconname("spin")
  positionWindow(w)
}

base_frame = TkFrame.new($spin_demo).pack(:fill=>:both, :expand=>true)

TkLabel.new(base_frame,
            :font=>$font, :wraplength=>'5i', :justify=>:left,
            :text=><<EOL).pack(:side=>:top)
���ˤϣ�����Υ��ԥ�ܥå�����ɽ������Ƥ��ޤ���
���줾�졢�ޥ��������򤷤�ʸ�������Ϥ��뤳�Ȥ��Ǥ��ޤ���
�Խ����Ȥ��Ƥϡ�Emacs ������¿���˲ä��ơ�����Ū��
Motif �����Υ��������ݡ��Ȥ���Ƥ��ޤ������Ȥ��С�
Backspace �� Control-h �Ȥ����ϥ�������κ�¦��ʸ����
�������Delete �� Control-d �Ȥϱ�¦��ʸ���������ޤ���
�����Ȥ�Ĺ����ۤ���褦��Ĺ��ʸ��������Ϥ������ˤϡ�
�ޥ����Υܥ��󣲤򲡤��ƥɥ�å����뤳�Ȥǡ�����ʸ����
�򥹥���󤹤뤳�Ȥ���ǽ�Ǥ���
�ʤ����ǽ�Υ��ԥ�ܥå����ϡ������ͤȤߤʤ����褦��
ʸ���󤷤����Ϥ�������ʤ����Ȥ���դ��Ƥ����������ޤ���
�����ܤΥ��ԥ�ܥå������������˸����Τϥ������ȥ�
�ꥢ���Ի�̾�Υꥹ�ȤȤʤäƤ��ޤ���
�⤷���ʤ����ȤäƤ��� Ruby �˥�󥯤���Ƥ��� Tk �饤
�֥�꤬ spinbox ���������åȤ�������Ƥ��ʤ���硢����
�ǥ�Ϥ��ޤ�ư���ʤ��Ϥ��Ǥ������ξ��ˤ� spinbox ����
�����åȤ���������Ƥ���褦�ʤ�꿷�����С������� Tk
���Ȥ߹�碌�ƻ�褦�ˤ��Ƥ���������
EOL

TkFrame.new(base_frame){|f|
  pack(:side=>:bottom, :fill=>:x, :pady=>'2m')

  TkButton.new(f, :text=>'�Ĥ���', :width=>15, :command=>proc{
                 $spin_demo.destroy
                 $spin_demo = nil
               }).pack(:side=>:left, :expand=>true)

  TkButton.new(f, :text=>'�����ɻ���', :width=>15, :command=>proc{
                 showCode 'spin'
               }).pack(:side=>:left, :expand=>true)
}

australianCities = [
    'Canberra', 'Sydney', 'Melbourne', 'Perth', 'Adelaide',
    'Brisbane', 'Hobart', 'Darwin', 'Alice Springs'
]

[
  TkSpinbox.new(base_frame, :from=>1, :to=>10, :width=>10, :validate=>:key,
                :validatecommand=>[
                  proc{|s| s == '' || /^[+-]?\d+$/ =~ s }, '%P'
                ]),
  TkSpinbox.new(base_frame, :from=>0, :to=>3, :increment=>0.5,
                :format=>'%05.2f', :width=>10),
  TkSpinbox.new(base_frame, :values=>australianCities, :width=>10)
].each{|sbox| sbox.pack(:side=>:top, :pady=>5, :padx=>10)}

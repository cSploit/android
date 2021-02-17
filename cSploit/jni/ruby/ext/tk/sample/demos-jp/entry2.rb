# -*- coding: euc-jp -*-
#
# entry (with scrollbars) widget demo (called by 'widget')
#

# toplevel widget ��¸�ߤ���к������
if defined?($entry2_demo) && $entry2_demo
  $entry2_demo.destroy
  $entry2_demo = nil
end

# demo �Ѥ� toplevel widget ������
$entry2_demo = TkToplevel.new {|w|
  title("Entry Demonstration (with scrollbars)")
  iconname("entry2")
  positionWindow(w)
}

base_frame = TkFrame.new($entry2_demo).pack(:fill=>:both, :expand=>true)

# label ����
msg = TkLabel.new(base_frame) {
  font $font
  wraplength '5i'
  justify 'left'
  text "3����ΰۤʤ륨��ȥ꤬�ơ���������С��դ�ɽ������Ƥ��ޤ���ʸ�������Ϥ���ˤϥݥ��󥿤���äƹԤ�������å����Ƥ��饿���פ��Ƥ���������ɸ��Ū��Motif���Խ���ǽ����Emacs�Υ����Х���ɤȤȤ�ˡ����ݡ��Ȥ���Ƥ��ޤ����㤨�С��Хå����ڡ����ȥ���ȥ���-H�ϥ�������κ���ʸ�����������ǥ꡼�ȥ����ȥ���ȥ���-D�ϥ�������α�¦��ʸ���������ޤ���Ĺ�᤮�ƥ�����ɥ��������ڤ�ʤ���Τϡ��ޥ����Υܥ���2�򲡤����ޤޥɥ�å����뤳�Ȥǥ������뤵���뤳�Ȥ��Ǥ��ޤ������ܸ�����Ϥ���Τϥ���ȥ���-�Хå�����å���Ǥ���kinput2��ư���Ƥ�������Ϥ��뤳�Ȥ��Ǥ��ޤ���"
}
msg.pack('side'=>'top')

# frame ����
TkFrame.new(base_frame) {|frame|
  TkButton.new(frame) {
    #text 'λ��'
    text '�Ĥ���'
    command proc{
      tmppath = $entry2_demo
      $entry2_demo = nil
      tmppath.destroy
    }
  }.pack('side'=>'left', 'expand'=>'yes')

  TkButton.new(frame) {
    text '�����ɻ���'
    command proc{showCode 'entry2'}
  }.pack('side'=>'left', 'expand'=>'yes')
}.pack('side'=>'bottom', 'fill'=>'x', 'pady'=>'2m')

# frame ����
TkFrame.new(base_frame, 'borderwidth'=>10) {|w|
  # entry 1
  s1 = TkScrollbar.new(w, 'relief'=>'sunken', 'orient'=>'horiz')
  e1 = TkEntry.new(w, 'relief'=>'sunken') {
    xscrollcommand proc{|first,last| s1.set first,last}
  }
  s1.command(proc{|*args| e1.xview(*args)})
  e1.pack('side'=>'top', 'fill'=>'x')
  s1.pack('side'=>'top', 'fill'=>'x')

  # spacer
  TkFrame.new(w, 'width'=>20, 'height'=>10).pack('side'=>'top', 'fill'=>'x')

  # entry 2
  s2 = TkScrollbar.new(w, 'relief'=>'sunken', 'orient'=>'horiz')
  e2 = TkEntry.new(w, 'relief'=>'sunken') {
    xscrollcommand proc{|first,last| s2.set first,last}
  }
  s2.command(proc{|*args| e2.xview(*args)})
  e2.pack('side'=>'top', 'fill'=>'x')
  s2.pack('side'=>'top', 'fill'=>'x')

  # spacer
  TkFrame.new(w, 'width'=>20, 'height'=>10).pack('side'=>'top', 'fill'=>'x')

  # entry 3
  s3 = TkScrollbar.new(w, 'relief'=>'sunken', 'orient'=>'horiz')
  e3 = TkEntry.new(w, 'relief'=>'sunken') {
    xscrollcommand proc{|first,last| s3.set first,last}
  }
  s3.command(proc{|*args| e3.xview(*args)})
  e3.pack('side'=>'top', 'fill'=>'x')
  s3.pack('side'=>'top', 'fill'=>'x')

  # ���������
  e1.insert(0, '�����')
  e2.insert('end', "���Υ���ȥ�ˤ�Ĺ��ʸ�������äƤ��ơ�")
  e2.insert('end', "Ĺ�����ƥ�����ɥ��ˤ������ڤ�ʤ��Τǡ�")
  e2.insert('end', "�ºݤν꽪��ޤǸ���ˤϥ������뤵���ʤ����")
  e2.insert('end', "�ʤ�ʤ��Ǥ��礦��")

}.pack('side'=>'top', 'fill'=>'x', 'expand'=>'yes')


# -*- coding: euc-jp -*-
#
# text (basic facilities) widget demo (called by 'widget')
#

# toplevel widget ��¸�ߤ���к������
if defined?($text_demo) && $text_demo
  $text_demo.destroy
  $text_demo = nil
end

# demo �Ѥ� toplevel widget ������
$text_demo = TkToplevel.new {|w|
  title("Text Demonstration - Basic Facilities")
  iconname("text")
  positionWindow(w)
}

base_frame = TkFrame.new($text_demo).pack(:fill=>:both, :expand=>true)

# version check
if ((Tk::TK_VERSION.split('.').collect{|n| n.to_i} <=> [8,4]) < 0)
  undo_support = false
else
  undo_support = true
end

# frame ����
TkFrame.new(base_frame) {|frame|
  TkButton.new(frame) {
    #text 'λ��'
    text '�Ĥ���'
    command proc{
      tmppath = $text_demo
      $text_demo = nil
      tmppath.destroy
    }
  }.pack('side'=>'left', 'expand'=>'yes')

  TkButton.new(frame) {
    text '�����ɻ���'
    command proc{showCode 'text'}
  }.pack('side'=>'left', 'expand'=>'yes')
}.pack('side'=>'bottom', 'fill'=>'x', 'pady'=>'2m')

# text ����
TkText.new(base_frame){|t|
  # ����
  relief 'sunken'
  bd 2
  setgrid 1
  height 30
  TkScrollbar.new(base_frame) {|s|
    pack('side'=>'right', 'fill'=>'y')
    command proc{|*args| t.yview(*args)}
    t.yscrollcommand proc{|first,last| s.set first,last}
  }
  pack('expand'=>'yes', 'fill'=>'both')

  # �ƥ���������
  insert('0.0', <<EOT)
���Υ�����ɥ��ϥƥ����� widget �Ǥ���1�Ԥޤ��Ϥ���ʾ�Υƥ����Ȥ�ɽ
�����Խ����뤳�Ȥ��Ǥ��ޤ����ʲ��ϥƥ����� widget �ǤǤ������ˤĤ���
�ޤȤ᤿��ΤǤ���

1. �������롣��������С��ǥƥ����Ȥ�ɽ����ʬ��ư�������Ȥ��Ǥ��ޤ���

2. ������˥󥰡��ƥ����ȤΥ�����ɥ��ǥޥ����ܥ���2 (��ܥ����) ��
���ƾ岼�˥ɥ�å����Ƥ�����������������ȥƥ����Ȥ���®�ǥɥ�å����졢
���Ƥ򤶤ä�į��뤳�Ȥ��Ǥ��ޤ���

3. �ƥ����Ȥ��������ޥ����ܥ���1 (���ܥ���) �򲡤���������������򥻥�
�Ȥ��Ƥ���ƥ����Ȥ����Ϥ��Ƥ������������Ϥ�����Τ� widget ������ޤ���

4. ���򡣤����ϰϤ�ʸ�������򤹤�ˤϥޥ����ܥ���1 �򲡤����ɥ�å���
�Ƥ������������٥ܥ����Υ�����顢���եȥ����򲡤��ʤ���ܥ���1 �򲡤�
���Ȥ������ϰϤ�Ĵ�����Ǥ��ޤ�������������ϰϤκǸ��ޥ������������
�Ǥ�ᤤ���֤˥ꥻ�åȤ����ܥ����Υ�����˥ޥ�����ɥ�å����뤳�ȤǤ�
��������ϰϤ�Ĵ���Ǥ��ޤ������֥륯��å��ǥ�ɤ򡢤ޤ��ȥ�ץ륯���
���ǹ����Τ����򤹤뤳�Ȥ��Ǥ��ޤ���

5. �õ���ִ����ƥ����Ȥ�õ��ˤϡ��õ����ʸ�������򤷤ƥХå�
���ڡ������ǥ꡼�ȥ��������Ϥ��Ƥ������������뤤�ϡ��������ƥ����Ȥ�
���Ϥ�������򤵤줿�ƥ����Ȥ��ִ�����ޤ���

6. ������ʬ�Υ��ԡ���������ʬ�򤳤Υ�����ɥ�����Τɤ����˥��ԡ�����
�ˤϡ��ޤ����ԡ��������������(�����ǡ����뤤���̤Υ��ץꥱ��������)
�����ܥ��� 2 �򥯥�å����ơ�������������ΰ��֤˥��ԡ����Ƥ���������

7. �Խ����ƥ����� widget �� Emacs �Υ����Х���ɤ˲ä���ɸ��Ū�ʤ� Motif
���Խ���ǽ�򥵥ݡ��Ȥ��Ƥ��ޤ����Хå����ڡ����ȥ���ȥ���-H ������
��������κ�¦��ʸ���������ޤ����ǥ꡼�ȥ����ȥ���ȥ���-D ������
��������α�¦��ʸ���������ޤ���Meta-�Хå����ڡ������������������
��¦��ñ���������Meta-D ��������������κ�¦��ñ��������ޤ���
����ȥ���-K �������������뤫������ޤǤ����������ΰ��֤˲���
�����ʤ��ä����ϡ����Ԥ������ޤ���#{
      if undo_support
        undo_text = "Control-z �ϺǸ�˹Ԥä��ѹ��μ��ä�(undo)��Ԥ���"
        case $tk_platform['platform']
        when "unix", "macintosh"
          undo_text << "Control-Shift-z"
        else # 'windows'
          undo_text << "Control-y"
        end
        undo_text << "��undo�����ѹ��κ�Ŭ��(redo)��Ԥ��ޤ���"
      else
        ""
      end
}


8. ������ɥ��Υꥵ���������� widget �� "setGrid" ���ץ����򥪥�ˤ�
�Ƥ���ޤ��Τǡ�������ɥ���ꥵ����������ˤϹ⤵�����Ͼ��ʸ�����ʸ
�����������ܤˤʤ�ޤ����ޤ���������ɥ��򶹤��������ˤ�Ĺ���Ԥ���ư
Ū���ޤ��֤��졢������Ƥ����Ƥ�������褦�ˤʤäƤ��ޤ���
EOT

  set_insert('0.0')
}


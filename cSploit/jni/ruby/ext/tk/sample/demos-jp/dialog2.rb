# -*- coding: euc-jp -*-
#
# a dialog box with a global grab (called by 'widget')
#
class TkDialog_Demo2 < TkDialog
  ###############
  private
  ###############
  def title
    "Dialog with global grab"
  end

  def message
    '���Υ��������ܥå����ϥ����Х륰��֤���Ѥ��Ƥ��ޤ������Υܥ����¹Ԥ���ޤǡ��ǥ����ץ쥤��Τ����ʤ��ΤȤ����äǤ��ޤ��󡣥����Х륰��֤���Ѥ��뤳�Ȥϡ��ޤ��ɤ��ͤ��ǤϤ���ޤ��󡣤ɤ����Ƥ�ɬ�פˤʤ�ޤǻȤ����Ȼפ�ʤ��ǲ�������'
  end

  def bitmap
    'info'
  end

  def default_button
    0
  end

  def buttons
#    "λ�� ����󥻥� �����ɻ���"
    ["λ��", "����󥻥�", "�����ɻ���"]
  end
end

ret =  TkDialog_Demo2.new('message_config'=>{'wraplength'=>'4i'},
                          'prev_command'=>proc{|dialog|
                            Tk.after 100, proc{dialog.grab('global')}
                          }).value
case ret
when 0
  print "���ʤ��ϡ�λ��פ򲡤��ޤ����͡�\n"
when 1
  print "���ʤ��ϡ֥���󥻥�פ򲡤��ޤ����͡�\n"
when 2
  showCode 'dialog2'
end


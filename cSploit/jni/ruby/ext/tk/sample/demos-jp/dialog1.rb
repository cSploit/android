# -*- coding: euc-jp -*-
#
# a dialog box with a local grab (called by 'widget')
#
class TkDialog_Demo1 < TkDialog
  ###############
  private
  ###############
  def title
    "Dialog with local grab"
  end

  def message
    '�⡼������������ܥå����Ǥ���Tk �� "grab" ���ޥ�ɤ���Ѥ��ƥ��������ܥå����ǡ֥����륰��֡פ��Ƥ��ޤ������Τ����줫�Υܥ����¹Ԥ��뤳�Ȥˤ�äơ����Υ���������������ޤǡ����Υ���֤ˤ�äƥ��ץꥱ��������¾�Υ�����ɥ��Ǥϡ��ݥ��󥿴ط��Υ��٥�Ȥ������뤳�Ȥ��Ǥ��ʤ��ʤäƤ��ޤ���'
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

ret =  TkDialog_Demo1.new('message_config'=>{'wraplength'=>'4i'}).value
case ret
when 0
  print "���ʤ��ϡ�λ��פ򲡤��ޤ����͡�\n"
when 1
  print "���ʤ��ϡ֥���󥻥�פ򲡤��ޤ����͡�\n"
when 2
  showCode 'dialog1'
end

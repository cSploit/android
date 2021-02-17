# -*- coding: euc-jp -*-
#
# paned1.rb
#
# This demonstration script creates a toplevel window containing
# a paned window that separates two windows horizontally.
#
# based on "Id: paned1.tcl,v 1.1 2002/02/22 14:07:01 dkf Exp"

if defined?($paned1_demo) && $paned1_demo
  $paned1_demo.destroy
  $paned1_demo = nil
end

$paned1_demo = TkToplevel.new {|w|
  title("Horizontal Paned Window Demonstration")
  iconname("paned1")
  positionWindow(w)
}

base_frame = TkFrame.new($paned1_demo).pack(:fill=>:both, :expand=>true)

TkLabel.new(base_frame,
            :font=>$font, :wraplength=>'4i', :justify=>:left,
            :text=><<EOL).pack(:side=>:top)
���ο��դ����줿��ĤΥ�����ɥ��δ֤λ��ڤ��Ȥϡ���Ĥ��ΰ�򤽤줾��Υ�����ɥ��Τ����ʬ�䤹�뤿��Τ�ΤǤ������ܥ���ǻ��ڤ������ȡ�ʬ�䥵�����ѹ����������ǤϺ�ɽ���Ϥʤ��줺�����ꤵ�����Ȥ���ɽ������������ޤ����ޥ����ˤ����ڤ�������ɿ路�ƥ��������ѹ�����ɽ�����ʤ���褦�ˤ��������ϡ��ޥ���������ܥ����ȤäƤ���������
�⤷���ʤ����ȤäƤ��� Ruby �˥�󥯤���Ƥ��� Tk �饤�֥�꤬ panedwindow ��������Ƥ��ʤ�
��硢���Υǥ�Ϥ��ޤ�ư���ʤ��Ϥ��Ǥ������ξ��ˤ� panedwindow ����������Ƥ���褦��
��꿷�����С������� Tk ���Ȥ߹�碌�ƻ
�褦�ˤ��Ƥ���������
EOL

# The bottom buttons
TkFrame.new(base_frame){|f|
  pack(:side=>:bottom, :fill=>:x, :pady=>'2m')

  TkButton.new(f, :text=>'�Ĥ���', :width=>15, :command=>proc{
                 $paned1_demo.destroy
                 $paned1_demo = nil
               }).pack(:side=>:left, :expand=>true)

  TkButton.new(f, :text=>'�����ɻ���', :width=>15, :command=>proc{
                 showCode 'paned1'
               }).pack(:side=>:left, :expand=>true)
}

TkPanedwindow.new(base_frame, :orient=>:horizontal){|f|
  add(Tk::Label.new(f, :text=>"This is the\nleft side", :bg=>'yellow'),
      Tk::Label.new(f, :text=>"This is the\nright side", :bg=>'cyan'))

  pack(:side=>:top, :expand=>true, :fill=>:both, :pady=>2, :padx=>'2m')
}

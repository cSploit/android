# -*- coding: euc-jp -*-
#
# ttkprogress.rb --
#
# This demonstration script creates several progress bar widgets.
#
# based on "Id: ttkprogress.tcl,v 1.3 2007/12/13 15:27:07 dgp Exp"

if defined?($ttkprogress_demo) && $ttkprogress_demo
  $ttkprogress_demo.destroy
  $ttkprogress_demo = nil
end

$ttkprogress_demo = TkToplevel.new {|w|
  title("Progress Bar Demonstration")
  iconname("ttkprogress")
  positionWindow(w)
}

base_frame = TkFrame.new($ttkprogress_demo).pack(:fill=>:both, :expand=>true)

Ttk::Label.new(base_frame, :font=>$font, :wraplength=>'4i', :justify=>:left,
               :text=><<EOL).pack(:side=>:top, :fill=>:x)
���ˤ���Τ���ĤΥץ��쥹�С��Ǥ���\
��Τ�Τ�"determinate"�����פΥץ��쥹�С��ǡ�\
�㤨�Хץ���बͿ����줿��������λ����ޤǤˤɤΤ��餤�����뤫��\
�����Ȥ��ʤɤ��Ѥ����ޤ���\
���Τ�Τ�"indeterminate"�����פΥץ��쥹�С��ǡ�\
�㤨�Хץ���ब�¹���(busy)�Ǥ����Τ�\
��λ�ޤǤˤɤ줯�餤�����뤫��ʬ����ʤ��Ȥ������֤�\
�����Ȥ��ʤɤ��Ѥ����ޤ���\
������Υץ��쥹�С��⡤�������ˤ���ܥ����Ȥ����Ȥ�\
��ư���˥᡼�����⡼�ɤ�ON/OFF�����ؤ��뤳�Ȥ��Ǥ��ޤ���
EOL

## See Code / Dismiss buttons
Ttk::Frame.new(base_frame) {|frame|
  sep = Ttk::Separator.new(frame)
  Tk.grid(sep, :columnspan=>4, :row=>0, :sticky=>'ew', :pady=>2)
  TkGrid('x',
         Ttk::Button.new(frame, :text=>'�����ɻ���',
                         :image=>$image['view'], :compound=>:left,
                         :command=>proc{showCode 'ttkprogress'}),
         Ttk::Button.new(frame, :text=>'�Ĥ���',
                         :image=>$image['delete'], :compound=>:left,
                         :command=>proc{
                           $ttkprogress_demo.destroy
                           $ttkprogress_demo = nil
                         }),
         :padx=>4, :pady=>4)
  grid_columnconfigure(0, :weight=>1)
  pack(:side=>:bottom, :fill=>:x)
}

frame = Ttk::Frame.new(base_frame).pack(:fill=>:both, :expand=>true)

p1 = Ttk::Progressbar.new(frame, :mode=>:determinate)
p2 = Ttk::Progressbar.new(frame, :mode=>:indeterminate)

start = Ttk::Button.new(frame, :text=>'Start Progress',
                        :command=>proc{ p1.start; p2.start })
stop  = Ttk::Button.new(frame, :text=>'Stop Progress',
                        :command=>proc{ p1.stop; p2.stop })

Tk.grid(p1, '-', :pady=>5, :padx=>10)
Tk.grid(p2, '-', :pady=>5, :padx=>10)
Tk.grid(start, stop, :padx=>10, :pady=>5)
start.grid_configure(:sticky=>'e')
stop.grid_configure(:sticky=>'w')
frame.grid_columnconfigure(:all, :weight=>1)


# -*- coding: euc-jp -*-
#
# ttkmenu.rb --
#
# This demonstration script creates a toplevel window containing several Ttk
# menubutton widgets.
#
# based on "Id: ttkmenu.tcl,v 1.3 2007/12/13 15:27:07 dgp Exp"

if defined?($ttkmenu_demo) && $ttkmenu_demo
  $ttkmenu_demo.destroy
  $ttkmenu_demo = nil
end

$ttkmenu_demo = TkToplevel.new {|w|
  title("Ttk Menu Buttons")
  iconname("ttkmenu")
  positionWindow(w)
}

base_frame = Ttk::Frame.new($ttkmenu_demo).pack(:fill=>:both, :expand=>true)

Ttk::Label.new(base_frame, :font=>$font, :wraplength=>'4i', :justify=>:left,
               :text=><<EOL).pack(:side=>:top, :fill=>:x)
Ttk�Ȥϡ��ơ��޻����ǽ�ʿ��������������åȽ���Ǥ���\
����ˤ��ơ��ޤ��б����뤳�Ȥ��Ǥ���褦�ˤʤä����������åȤΤҤȤĤ�\
��˥塼�ܥ��󤬤���ޤ���\
�ʲ��Ǥϡ��ơ��ޤ��б�������˥塼�ܥ��󤬤����Ĥ�ɽ������Ƥ��ޤ���\
������Ȥäơ����߻�����Υơ��ޤ��ѹ����뤳�Ȥ���ǽ�Ǥ���\
�ơ��ޤ����򤬥�˥塼�ܥ��󼫿Ȥθ��ݤ����Ѳ��������ͻҤ䡤\
����Υ�˥塼�ܥ���������ۤʤ륹������\
(�ġ���С��Ǥΰ���Ū��ɽ����Ŭ�������)��ɽ������Ƥ����ͻҤ�\
���ܤ��Ƥ���������\
�ʤ�����˥塼�ܥ���ˤĤ��Ƥϥơ��ޤ��б��������������åȤ�����ޤ�����\
��˥塼�ˤĤ��Ƥϥơ��ޤ��б��������������åȤϴޤޤ�Ƥ��ޤ���\
������ͳ�ϡ�ɸ���Tk�Υ�˥塼���������åȤ�\
���٤ƤΥץ�åȥۡ���ǽ�ʬ���ɹ��ʸ��ݤ������������äƤ��롤\
�äˡ�¿���δĶ��Ǥ��δĶ����������ηϤȤʤ�褦�˼�������Ƥ����\
Ƚ�Ǥ��줿���Ȥˤ��ޤ���
EOL

Ttk::Separator.new(base_frame).pack(:side=>:top, :fill=>:x)

## See Code / Dismiss
Ttk::Frame.new($ttkmenu_demo) {|frame|
  sep = Ttk::Separator.new(frame)
  Tk.grid(sep, :columnspan=>4, :row=>0, :sticky=>'ew', :pady=>2)
  TkGrid('x',
         Ttk::Button.new(frame, :text=>'�����ɻ���',
                         :image=>$image['view'], :compound=>:left,
                         :command=>proc{showCode 'ttkmenu'}),
         Ttk::Button.new(frame, :text=>'�Ĥ���',
                         :image=>$image['delete'], :compound=>:left,
                         :command=>proc{
                           $ttkmenu_demo.destroy
                           $ttkmenu_demo = nil
                         }),
         :padx=>4, :pady=>4)
  grid_columnconfigure(0, :weight=>1)
  pack(:side=>:bottom, :fill=>:x)
}

b1 = Ttk::Menubutton.new(base_frame,:text=>'�ơ��ޤ�����',:direction=>:above)
b2 = Ttk::Menubutton.new(base_frame,:text=>'�ơ��ޤ�����',:direction=>:left)
b3 = Ttk::Menubutton.new(base_frame,:text=>'�ơ��ޤ�����',:direction=>:right)
b4 = Ttk::Menubutton.new(base_frame,:text=>'�ơ��ޤ�����',:direction=>:flush,
                         :style=>Ttk::Menubutton.style('Toolbutton'))
b5 = Ttk::Menubutton.new(base_frame,:text=>'�ơ��ޤ�����',:direction=>:below)

b1.menu(m1 = Tk::Menu.new(b1, :tearoff=>false))
b2.menu(m2 = Tk::Menu.new(b2, :tearoff=>false))
b3.menu(m3 = Tk::Menu.new(b3, :tearoff=>false))
b4.menu(m4 = Tk::Menu.new(b4, :tearoff=>false))
b5.menu(m5 = Tk::Menu.new(b5, :tearoff=>false))

Ttk.themes.each{|theme|
  m1.add(:command, :label=>theme, :command=>proc{Ttk.set_theme theme})
  m2.add(:command, :label=>theme, :command=>proc{Ttk.set_theme theme})
  m3.add(:command, :label=>theme, :command=>proc{Ttk.set_theme theme})
  m4.add(:command, :label=>theme, :command=>proc{Ttk.set_theme theme})
  m5.add(:command, :label=>theme, :command=>proc{Ttk.set_theme theme})
}

f = Ttk::Frame.new(base_frame).pack(:fill=>:x)
f1 = Ttk::Frame.new(base_frame).pack(:fill=>:both, :expand=>true)
f.lower

f.grid_anchor(:center)
TkGrid('x', b1, 'x', :in=>f, :padx=>3, :pady=>2)
TkGrid(b2,  b4, b3,  :in=>f, :padx=>3, :pady=>2)
TkGrid('x', b5, 'x', :in=>f, :padx=>3, :pady=>2)

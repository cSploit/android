# -*- coding: euc-jp -*-
#
# ttkbut.rb
#
# This demonstration script creates a toplevel window containing several
# simple Ttk widgets, such as labels, labelframes, buttons, checkbuttons and
# radiobuttons.
#
# based on "Id: ttkbut.tcl,v 1.4 2007/12/13 15:27:07 dgp Exp"

if defined?($ttkbut_demo) && $ttkbut_demo
  $ttkbut_demo.destroy
  $ttkbut_demo = nil
end

$ttkbut_demo = TkToplevel.new {|w|
  title("Simple Ttk Widgets")
  iconname("ttkbut")
  positionWindow(w)
}

base_frame = TkFrame.new($ttkbut_demo).pack(:fill=>:both, :expand=>true)

Ttk::Label.new(base_frame, :font=>$font, :wraplength=>'4i', :justify=>:left,
               :text=><<EOL).pack(:side=>:top, :fill=>:x)
Ttk�Ȥϡ��ơ��޻����ǽ�ʿ��������������åȽ���Ǥ���\
�������ʤ����ܤˤ��Ƥ���Τ�Ttk�Υơ��޲���٥�ǡ�\
���ˤ�Ttk�Υ�٥�ե졼�����˻��ĤΥ��롼�פ�Ttk���������åȤ�\
ɽ������Ƥ��ޤ���
�ǽ�Υ��롼�פ����ƥܥ���Ǥ��ꡤ\
���줾�쥯��å�����и��ߤΥ��ץꥱ�������Υơ��ޤ����ꤵ��ޤ���
�����ܤΥ��롼�פϻ��ĤΥ����å��ܥ��󽸹�Ǥ���\
�ƽ���δ֤ˤϡ����ѥ졼�����������åȤ��֤���Ƥ��ޤ���\
�ʤ���ͭ�����ץܥ���ϡ����Υȥåץ�٥륦�������å����\
¾�Τ��٤ƤΥơ��޲����������åȤξ���(state)��"disabled"���ɤ�����\
����ȥ��뤹�뤳�Ȥ���դ��Ƥ���������
�����ܤΥ��롼�פϴ�Ϣ�դ���줿�饸���ܥ��󽸹�ȤʤäƤ��ޤ���
EOL

## Add buttons for setting the theme
buttons = Ttk::Labelframe.new(base_frame, :text=>'�ܥ���')
# Ttk::Style.theme_names.each{|theme|
#   Ttk::Button.new(buttons, :text=>theme,
#                   :command=>proc{Ttk::Style.theme_use theme}).pack(:pady=>2)
# }
Ttk.themes.each{|theme|
  Ttk::Button.new(buttons, :text=>theme,
                  :command=>proc{Ttk.set_theme theme}).pack(:pady=>2)
}

## Helper procedure for the top checkbutton
def setState(root, value, *excepts)
  return if excepts.member?(root)

  ## Non-Ttk widgets (e.g. the toplevel) will fail, so make it silent
  begin
    root.state = value
  rescue
  end

  ## Recursively invoke on all children of this root that are in the same
  ## toplevel widget
  root.winfo_children.each{|w|
    setState(w, value, *excepts) if w.winfo_toplevel == root.winfo_toplevel
  }
end

## Set up the checkbutton group
checks = Ttk::Labelframe.new(base_frame, :text=>'�����å��ܥ���')
enabled = TkVariable.new(true)
e = Ttk::Checkbutton.new(checks, :text=>'ͭ����', :variable=>enabled,
                         :command=>proc{
                           setState($ttkbut_demo,
                                    ((enabled.bool)? "!disabled" : "disabled"),
                                    e)
                         })

## See ttk_widget(n) for other possible state flags
sep1 = Ttk::Separator.new(checks)
sep2 = Ttk::Separator.new(checks)

cheese  = TkVariable.new
tomato  = TkVariable.new
basil   = TkVariable.new
oregano = TkVariable.new

c1 = Ttk::Checkbutton.new(checks, :text=>'������',   :variable=>cheese)
c2 = Ttk::Checkbutton.new(checks, :text=>'�ȥޥ�',   :variable=>tomato)
c3 = Ttk::Checkbutton.new(checks, :text=>'�Х���',   :variable=>basil)
c4 = Ttk::Checkbutton.new(checks, :text=>'���쥬��', :variable=>oregano)

Tk.pack(e, sep1, c1, c2, sep2, c3, c4, :fill=>:x, :pady=>2)

## Set up the radiobutton group
radios = Ttk::Labelframe.new(base_frame, :text=>'�饸���ܥ���')

happyness = TkVariable.new

r1 = Ttk::Radiobutton.new(radios, :variable=>happyness,
                          :text=>'Great', :value=>'great')
r2 = Ttk::Radiobutton.new(radios, :variable=>happyness,
                          :text=>'Good', :value=>'good')
r3 = Ttk::Radiobutton.new(radios, :variable=>happyness,
                          :text=>'Ok', :value=>'ok')
r4 = Ttk::Radiobutton.new(radios, :variable=>happyness,
                          :text=>'Poor', :value=>'poor')
r5 = Ttk::Radiobutton.new(radios, :variable=>happyness,
                          :text=>'Awful', :value=>'awful')

Tk.pack(r1, r2, r3, r4, r5, :fill=>:x, :padx=>3, :pady=>2)

## See Code / Dismiss
Ttk::Frame.new(base_frame) {|frame|
  sep = Ttk::Separator.new(frame)
  Tk.grid(sep, :columnspan=>4, :row=>0, :sticky=>'ew', :pady=>2)
  TkGrid('x',
         Ttk::Button.new(frame, :text=>'�ѿ�����',
                         :image=>$image['view'], :compound=>:left,
                         :command=>proc{
                           showVars(base_frame, ['ͭ����', enabled],
                                    ['������', cheese], ['�ȥޥ�', tomato],
                                    ['�Х���', basil], ['���쥬��', oregano],
                                    ['��ʡ��', happyness])
                         }),
         Ttk::Button.new(frame, :text=>'�����ɻ���',
                         :image=>$image['view'], :compound=>:left,
                         :command=>proc{showCode 'ttkbut'}),
         Ttk::Button.new(frame, :text=>'�Ĥ���',
                         :image=>$image['delete'], :compound=>:left,
                         :command=>proc{
                           tmppath = $ttkbut_demo
                           $ttkbut_demo = nil
                           $showVarsWin[tmppath.path] = nil
                           tmppath.destroy
                         }),
         :padx=>4, :pady=>4)
  grid_columnconfigure(0, :weight=>1)
  pack(:side=>:bottom, :fill=>:x, :expand=>true)
}

## Arrange things neatly
f = Ttk::Frame.new(base_frame).pack(:fill=>:both, :expand=>true)
f.lower
Tk.grid(buttons, checks, radios, :in=>f, :sticky=>'nwe', :pady=>2, :padx=>3)
f.grid_columnconfigure([0, 1, 2], :weight=>1, :uniform=>:yes)

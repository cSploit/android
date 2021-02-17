# -*- coding: euc-jp -*-
#
# ttknote.rb --
#
# This demonstration script creates a toplevel window containing a Ttk
# notebook widget.
#
# based on "Id: ttknote.tcl,v 1.5 2007/12/13 15:27:07 dgp Exp"

if defined?($ttknote_demo) && $ttknote_demo
  $ttknote_demo.destroy
  $ttknote_demo = nil
end

$ttknote_demo = TkToplevel.new {|w|
  title("Ttk Notebook Widget")
  iconname("ttknote")
  positionWindow(w)
}

## See Code / Dismiss
Ttk::Frame.new($ttknote_demo) {|frame|
  sep = Ttk::Separator.new(frame)
  Tk.grid(sep, :columnspan=>4, :row=>0, :sticky=>'ew', :pady=>2)
  TkGrid('x',
         Ttk::Button.new(frame, :text=>'�����ɻ���',
                         :image=>$image['view'], :compound=>:left,
                         :command=>proc{showCode 'ttknote'}),
         Ttk::Button.new(frame, :text=>'�Ĥ���',
                         :image=>$image['delete'], :compound=>:left,
                         :command=>proc{
                           $ttknote_demo.destroy
                           $ttknote_demo = nil
                         }),
         :padx=>4, :pady=>4)
  grid_columnconfigure(0, :weight=>1)
  pack(:side=>:bottom, :fill=>:x)
}

base_frame = Ttk::Frame.new($ttknote_demo).pack(:fill=>:both, :expand=>true)

## Make the notebook and set up Ctrl+Tab traversal
notebook = Ttk::Notebook.new(base_frame).pack(:fill=>:both, :expand=>true,
                                              :padx=>2, :pady=>3)
notebook.enable_traversal

## Popuplate the first pane
f_msg = Ttk::Frame.new(notebook)
msg_m = Ttk::Label.new(f_msg, :font=>$font, :wraplength=>'5i',
                       :justify=>:left, :anchor=>'n', :text=><<EOL)
Ttk�Ȥϡ��ơ��޻����ǽ�ʿ��������������åȽ���Ǥ���\
������˴ޤޤ�륦�������åȤΤҤȤĤ˥Ρ��ȥ֥å����������åȤ�����ޤ���\
�Ρ��ȥ֥å����������åȤϡ�\
���̤����Ƥ���ä��ѥͥ뤫���������ǽ�ˤ���褦��\
���֤ν���ʥ��֥��åȡˤ����ޤ���\
���֥��åȤϺǶ��¿���Υ桼�����󥿡��ե������Ǹ����뵡ǽ�Ǥ���\
���֤�����ϡ��ޥ����ˤ������Ǥʤ���\
�Ρ��ȥ֥å����������åȤΥڡ����θ��Ф������򤵤�Ƥ�����Ǥ����\
Ctrl+Tab���������Ϥˤ�äƤ�Ԥ����Ȥ��Ǥ��ޤ���\
���Υǥ�Ǥϡ����Ф��ǲ����դ���ʸ���Υ�����Alt�����Ȥ��Ȥ߹�碌�뤳�Ȥ�\
�ڡ��������򤹤뤳�Ȥ��Ǥ���褦�ˤ����ꤷ�Ƥ��ޤ���\
�������������ܤΥ��֤�̵�������������Ǥ��ʤ��褦�ˤʤäƤ��뤳�Ȥˤ�\
��դ��Ƥ���������
EOL
neat = TkVariable.new
after_id = nil
msg_b = Ttk::Button.new(f_msg, :text=>'���Ƥ�����(Neat!)', :underline=>6,
                        :command=>proc{
                          neat.value = '���������ΤȤ��ꤵ������'
                          Tk.after_cancel(after_id) if after_id
                          after_id = Tk.after(500){neat.value = ''}
                        })
msg_b.winfo_toplevel.bind('Alt-n'){ msg_b.focus; msg_b.invoke }
msg_l = Ttk::Label.new(f_msg, :textvariable=>neat)
notebook.add(f_msg, :text=>'����(Description)', :underline=>3, :padding=>2)
Tk.grid(msg_m, '-', :sticky=>'new', :pady=>2)
Tk.grid(msg_b, msg_l, :pady=>[2, 4], :padx=>20)
msg_b.grid_configure(:sticky=>'e')
msg_l.grid_configure(:sticky=>'w')
f_msg.grid_rowconfigure(1, :weight=>1)
f_msg.grid_columnconfigure([0, 1], :weight=>1, :uniform=>1)

## Populate the second pane. Note that the content doesn't really matter
f_disabled = Ttk::Frame.new(notebook)
notebook.add(f_disabled, :text=>'̵�������줿����', :state=>:disabled)

## Popuplate the third pane
f_editor = Ttk::Frame.new(notebook)
notebook.add(f_editor, :text=>'�ƥ����ȥ��ǥ���(Text Editor)', :underline=>9)
editor_t = Tk::Text.new(f_editor, :width=>40, :height=>10, :wrap=>:char)
if Tk.windowingsystem != 'aqua'
  editor_s = editor_t.yscrollbar(Ttk::Scrollbar.new(f_editor))
else
  editor_s = editor_t.yscrollbar(Tk::Scrollbar.new(f_editor))
end
editor_s.pack(:side=>:right, :fill=>:y, :padx=>[0,2], :pady=>2)
editor_t.pack(:fill=>:both, :expand=>true, :padx=>[2,0], :pady=>2)

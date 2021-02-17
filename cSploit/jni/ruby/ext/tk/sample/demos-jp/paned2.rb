# -*- coding: euc-jp -*-
#
# paned2.rb --
#
# This demonstration script creates a toplevel window containing
# a paned window that separates two windows vertically.
#
# based on "Id: paned2.tcl,v 1.1 2002/02/22 14:07:01 dkf Exp"

if defined?($paned2_demo) && $paned2_demo
  $paned2_demo.destroy
  $paned2_demo = nil
end

$paned2_demo = TkToplevel.new {|w|
  title("Vertical Paned Window Demonstration")
  iconname("paned2")
  positionWindow(w)
}

base_frame = TkFrame.new($paned2_demo).pack(:fill=>:both, :expand=>true)

TkLabel.new(base_frame,
            :font=>$font, :wraplength=>'4i', :justify=>:left,
            :text=><<EOL).pack(:side=>:top)
���Υ�������С��դ��Υ��������åȤ��֤��줿��ĤΥ�����ɥ��δ֤λ��ڤ��Ȥϡ���Ĥ��ΰ�򤽤줾��Υ�����ɥ��Τ����ʬ�䤹�뤿��Τ�ΤǤ������ܥ���ǻ��ڤ������ȡ�ʬ�䥵�����ѹ����������ǤϺ�ɽ���Ϥʤ��줺�����ꤵ�����Ȥ���ɽ������������ޤ����ޥ����ˤ����ڤ�������ɿ路�ƥ��������ѹ�����ɽ�����ʤ���褦�ˤ��������ϡ��ޥ���������ܥ����ȤäƤ���������
�⤷���ʤ����ȤäƤ��� Ruby �˥�󥯤���Ƥ��� Tk �饤�֥�꤬ panedwindow ��������Ƥ��ʤ�
��硢���Υǥ�Ϥ��ޤ�ư���ʤ��Ϥ��Ǥ������ξ��ˤ� panedwindow ����������Ƥ���褦��
��꿷�����С������� Tk ���Ȥ߹�碌�ƻ
�褦�ˤ��Ƥ���������
EOL

# The bottom buttons
TkFrame.new(base_frame){|f|
  pack(:side=>:bottom, :fill=>:x, :pady=>'2m')

  TkButton.new(f, :text=>'�Ĥ���', :width=>15, :command=>proc{
                 $paned2_demo.destroy
                 $paned2_demo = nil
               }).pack(:side=>:left, :expand=>true)

  TkButton.new(f, :text=>'�����ɻ���', :width=>15, :command=>proc{
                 showCode 'paned2'
               }).pack(:side=>:left, :expand=>true)
}

paneList = TkVariable.new  # define as normal variable (not array)
paneList.value = [         # ruby's array --> tcl's list
    'Ruby/Tk �Υ��������åȰ���',
    'TkButton',
    'TkCanvas',
    'TkCheckbutton',
    'TkEntry',
    'TkFrame',
    'TkLabel',
    'TkLabelframe',
    'TkListbox',
    'TkMenu',
    'TkMenubutton',
    'TkMessage',
    'TkPanedwindow',
    'TkRadiobutton',
    'TkScale',
    'TkScrollbar',
    'TkSpinbox',
    'TkText',
    'TkToplevel'
]

# Create the pane itself
TkPanedwindow.new(base_frame, :orient=>:vertical){|f|
  pack(:side=>:top, :expand=>true, :fill=>:both, :pady=>2, :padx=>'2m')

  add(TkFrame.new(f){|paned2_top|
        TkListbox.new(paned2_top, :listvariable=>paneList) {
          # Invert the first item to highlight it
          itemconfigure(0, :background=>self.cget(:foreground),
                           :foreground=>self.cget(:background) )
          yscrollbar(TkScrollbar.new(paned2_top).pack(:side=>:right,
                                                      :fill=>:y))
          pack(:fill=>:both, :expand=>true)
        }
      },

      TkFrame.new(f, :height=>120) {|paned2_bottom|
        # The bottom window is a text widget with scrollbar
        paned2_xscr = TkScrollbar.new(paned2_bottom)
        paned2_yscr = TkScrollbar.new(paned2_bottom)
        paned2_text = TkText.new(paned2_bottom, :width=>30, :wrap=>:non) {
          insert('1.0', '���������֤���Ƥ���Τϡ�' +
                        '�������̤Υƥ����ȥ��������åȤǤ���')
          xscrollbar(paned2_xscr)
          yscrollbar(paned2_yscr)
        }
        Tk.grid(paned2_text, paned2_yscr, :sticky=>'nsew')
        Tk.grid(paned2_xscr, :sticky=>'nsew')
        TkGrid.columnconfigure(paned2_bottom, 0, :weight=>1)
        TkGrid.rowconfigure(paned2_bottom, 0, :weight=>1)
      } )
}

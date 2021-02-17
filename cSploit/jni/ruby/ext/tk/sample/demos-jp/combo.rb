# -*- coding: euc-jp -*-
#
# combo.rb --
#
# This demonstration script creates several combobox widgets.
#
# based on "Id: combo.tcl,v 1.3 2007/12/13 15:27:07 dgp Exp"

if defined?($combo_demo) && $combo_demo
  $combo_demo.destroy
  $combo_demo = nil
end

$combo_demo = TkToplevel.new {|w|
  title("Combobox Demonstration")
  iconname("combo")
  positionWindow(w)
}

base_frame = TkFrame.new($combo_demo).pack(:fill=>:both, :expand=>true)

Ttk::Label.new(base_frame, :font=>$font, :wraplength=>'5i', :justify=>:left,
               :text=><<EOL).pack(:side=>:top, :fill=>:x)
�ʲ��Ǥ�3����Υ���ܥܥå�����ɽ������Ƥ��ޤ���\
�ǽ�Τ�Τϡ�����ȥꥦ�������åȤ�Ʊ���ͤˡ�\
�ݥ���Ȥ����ꡤ����å������ꡤ�����פ����ꤹ�뤳�Ȥ��Ǥ��ޤ���\
�ޤ���Return���������Ϥ���и��ߤ��ͤ��ꥹ�Ȥ��ɲä��졤\
�ɥ�åץ�����ꥹ�Ȥ������򤹤뤳�Ȥ��Ǥ���褦�ˤʤ�ޤ���\
��(���������)�����򲡤���ɽ�����줿�ꥹ�Ȥ���\
���������¾�θ���������Return�����򲡤��С��ͤ�����Ǥ��ޤ���\
2���ܤΥ���ܥܥå�����������ͤ˸��ꤵ��Ƥ��ꡤ�����ѹ��Ǥ��ޤ���\
3���ܤΤ�Τϥ������ȥ�ꥢ���ԻԤΥɥ�åץ�����ꥹ�Ȥ���\
���򤹤뤳�Ȥ�������ǽ�ȤʤäƤ��ޤ���
EOL

## variables
firstValue  = TkVariable.new
secondValue = TkVariable.new
ozCity      = TkVariable.new

## See Code / Dismiss buttons
Ttk::Frame.new(base_frame) {|frame|
  sep = Ttk::Separator.new(frame)
  Tk.grid(sep, :columnspan=>4, :row=>0, :sticky=>'ew', :pady=>2)
  TkGrid('x',
         Ttk::Button.new(frame, :text=>'�ѿ�����',
                         :image=>$image['view'], :compound=>:left,
                         :command=>proc{
                           showVars(base_frame,
                                    ['firstVariable', firstValue],
                                    ['secondVariable', secondValue],
                                    ['ozCity', ozCity])
                         }),
         Ttk::Button.new(frame, :text=>'�����ɻ���',
                         :image=>$image['view'], :compound=>:left,
                         :command=>proc{showCode 'combo'}),
         Ttk::Button.new(frame, :text=>'�Ĥ���',
                         :image=>$image['delete'], :compound=>:left,
                         :command=>proc{
                           $combo_demo.destroy
                           $combo_demo = nil
                         }),
         :padx=>4, :pady=>4)
  grid_columnconfigure(0, :weight=>1)
  pack(:side=>:bottom, :fill=>:x)
}

frame = Ttk::Frame.new(base_frame).pack(:fill=>:both, :expand=>true)

australianCities = [
  '�����٥�', '���ɥˡ�', '���ܥ��', '�ѡ���', '���ǥ졼��',
  '�֥ꥹ�١���', '�ۥС���', '����������', '���ꥹ ���ץ�󥰥�'
]


secondValue.value = '�ѹ��Բ�'
ozCity.value = '���ɥˡ�'

Tk.pack(Ttk::Labelframe.new(frame, :text=>'Fully Editable'){|f|
          Ttk::Combobox.new(f, :textvariable=>firstValue){|b|
            b.bind('Return', '%W'){|w|
              w.values <<= w.value unless w.values.include?(w.value)
            }
          }.pack(:pady=>5, :padx=>10)
        },

        Ttk::LabelFrame.new(frame, :text=>'Disabled'){|f|
          Ttk::Combobox.new(f, :textvariable=>secondValue, :state=>:disabled) .
            pack(:pady=>5, :padx=>10)
        },

        Ttk::LabelFrame.new(frame, :text=>'Defined List Only'){|f|
          Ttk::Combobox.new(f, :textvariable=>ozCity, :state=>:readonly,
                            :values=>australianCities) .
            pack(:pady=>5, :padx=>10)
        },

        :side=>:top, :pady=>5, :padx=>10)

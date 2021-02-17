# -*- coding: euc-jp -*-
#
# labelframe.rb
#
# This demonstration script creates a toplevel window containing
# several labelframe widgets.
#
# based on "Id: labelframe.tcl,v 1.2 2001/10/30 11:21:50 dkf Exp"


if defined?($labelframe_demo) && $labelframe_demo
  $labelframe_demo.destroy
  $labelframe_demo = nil
end

$labelframe_demo = TkToplevel.new {|w|
  title("Labelframe Demonstration")
  iconname("labelframe")
  positionWindow(w)
}

base_frame = TkFrame.new($labelframe_demo).pack(:fill=>:both, :expand=>true)

# Some information
TkLabel.new(base_frame,
            :font=>$font, :wraplength=>'4i', :justify=>:left,
            :text=><<EOL).pack(:side=>:top)
TkLabelFrame ���������åȤϴ�Ϣ���� widget
����ޤȤ�Ƽ�갷��������Ѥ����ޤ�����
�٥���̾��ʸ����Ǥⲿ�餫�Υ��������å�
�Ǥ⤫�ޤ��ޤ��󡣤⤷���ʤ����ȤäƤ���
Ruby �˥�󥯤���Ƥ��� Tk �饤�֥�꤬
labelframe ���������åȤ�������Ƥ��ʤ�
��硢���Υǥ�Ϥ��ޤ�ư���ʤ��Ϥ��Ǥ���
���ξ��ˤ� labelframe ���������åȤ�����
����Ƥ���褦�ʤ�꿷�����С������� Tk
���Ȥ߹�碌�ƻ�褦�ˤ��Ƥ���������
EOL

# The bottom buttons
TkFrame.new(base_frame){|f|
  pack(:side=>:bottom, :fill=>:x, :pady=>'2m')

  TkButton.new(f, :text=>'�Ĥ���', :width=>15, :command=>proc{
                 $labelframe_demo.destroy
                 $labelframe_demo = nil
               }).pack(:side=>:left, :expand=>true)

  TkButton.new(f, :text=>'�����ɻ���', :width=>15, :command=>proc{
                 showCode 'labelframe'
               }).pack(:side=>:left, :expand=>true)
}

# Demo area
w = TkFrame.new(base_frame).pack(:side=>:bottom, :fill=>:both,
                                       :expand=>true)

# A group of radiobuttons in a labelframe
TkLabelFrame.new(w, :text=>'������',
                 :padx=>2, :pady=>2) {|f|
  grid(:row=>0, :column=>0, :pady=>'2m', :padx=>'2m')

  v = TkVariable.new
  (1..4).each{|i|
    TkRadiobutton.new(f, :text=>"This is value #{i}",
                      :variable=>v, :value=>i) {
      pack(:side=>:top, :fill=>:x, :pady=>2)
    }
  }
}


# Using a label window to control a group of options.
$lfdummy = TkVariable.new(0)

def lfEnableButtons(w)
  TkWinfo.children(w).each{|child|
    next if child.path =~ /\.cb$/
    if $lfdummy == 1
      child.state(:normal)
    else
      child.state(:disabled)
    end
  }
end

TkLabelFrame.new(w, :pady=>2, :padx=>2){|f|
  TkCheckButton.new(f, :widgetname=>'cb', :variable=>$lfdummy,
                    :text=>"���ץ��������", :padx=>0) {|cb|
    command proc{lfEnableButtons(f)}
    f.labelwidget(cb)
  }
  grid(:row=>0, :column=>1, :pady=>'2m', :padx=>'2m')

  %w(���ץ����1 ���ץ����2 ���ץ����3).each{|str|
    TkCheckbutton.new(f, :text=>str).pack(:side=>:top, :fill=>:x, :pady=>2)
  }

  lfEnableButtons(f)
}

TkGrid.columnconfigure(w, [0,1], :weight=>1)

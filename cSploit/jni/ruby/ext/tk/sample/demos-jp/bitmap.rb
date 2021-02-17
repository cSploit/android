# -*- coding: euc-jp -*-
#
# bitmap widget demo (called by 'widget')
#

# bitmapRow --
# Create a row of bitmap items in a window.
#
# Arguments:
# w -           The parent window that is to contain the row.
# args -        The names of one or more bitmaps, which will be displayed
#               in a new row across the bottom of w along with their
#               names.

def bitmapRow(w,*args)
  TkFrame.new(w){|row|
    pack('side'=>'top', 'fill'=>'both')
    for bitmap in args
      TkFrame.new(row){|base|
        pack('side'=>'left', 'fill'=>'both', 'pady'=>'.25c', 'padx'=>'.25c')
        TkLabel.new(base, 'text'=>bitmap, 'width'=>9).pack('side'=>'bottom')
        Tk::Label.new(base, 'bitmap'=>bitmap).pack('side'=>'bottom')
      }
    end
  }
end

# toplevel widget ��¸�ߤ���к������
if defined?($bitmap_demo) && $bitmap_demo
  $bitmap_demo.destroy
  $bitmap_demo = nil
end

# demo �Ѥ� toplevel widget ������
$bitmap_demo = TkToplevel.new {|w|
  title("Bitmap Demonstration")
  iconname("bitmap")
  positionWindow(w)
}

base_frame = TkFrame.new($bitmap_demo).pack(:fill=>:both, :expand=>true)

# label ����
TkLabel.new(base_frame,'font'=>$font,'wraplength'=>'4i','justify'=>'left',
            'text'=>"���Υ�����ɥ��ˤϡ�Tk ���Ȥ߹��ޤ줿���٤ƤΥӥåȥޥåפ���������̾���ȶ���ɽ������Ƥ��ޤ���Tcl �Υ�����ץ���Ǥϡ����줾���̾�����Ѥ��ƻ��Ȥ��ޤ���"){
  pack('side'=>'top')
}

# frame ����
$bitmap_buttons = TkFrame.new(base_frame) {|frame|
  TkButton.new(frame) {
    #text 'λ��'
    text '�Ĥ���'
    command proc{
      tmppath = $bitmap_demo
      $bitmap_demo = nil
      tmppath.destroy
    }
  }.pack('side'=>'left', 'expand'=>'yes')

  TkButton.new(frame) {
    text '�����ɻ���'
    command proc{showCode 'bitmap'}
  }.pack('side'=>'left', 'expand'=>'yes')
}
$bitmap_buttons.pack('side'=>'bottom', 'fill'=>'x', 'pady'=>'2m')

# frame ����
TkFrame.new(base_frame){|f|
  bitmapRow(f,'error','gray25','gray50','hourglass')
  bitmapRow(f,'info','question','questhead','warning')
  pack('side'=>'top', 'expand'=>'yes', 'fill'=>'both')
}


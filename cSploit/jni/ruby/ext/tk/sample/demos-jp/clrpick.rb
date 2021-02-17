# -*- coding: euc-jp -*-
#
# widget demo prompts the user to select a color (called by 'widget')
#
#  Note: don't support ttk_wrapper. work with standard widgets only.
#

# toplevel widget ��¸�ߤ���к������
if defined?($clrpick_demo) && $clrpick_demo
  $clrpick_demo.destroy
  $clrpick_demo = nil
end

# demo �Ѥ� toplevel widget ������
$clrpick_demo = TkToplevel.new {|w|
  title("Color Selection Dialogs")
  iconname("colors")
  positionWindow(w)
}

# label ����
#TkLabel.new($clrpick_demo,'font'=>$font,'wraplength'=>'4i','justify'=>'left',
Tk::Label.new($clrpick_demo,'font'=>$font,'wraplength'=>'4i','justify'=>'left',
            'text'=>"�ʲ��Υܥ���򲡤��ơ����Υ�����ɥ���ˤ��륦�������åȤ����ʿ����طʿ������򤷤Ʋ�������").pack('side'=>'top')

# frame ����
# TkFrame.new($clrpick_demo) {|frame|
Tk::Frame.new($clrpick_demo) {|frame|
  # TkButton.new(frame) {
  Tk::Button.new(frame) {
    #text 'λ��'
    text '�Ĥ���'
    command proc{
      tmppath = $clrpick_demo
      $clrpick_demo = nil
      tmppath.destroy
    }
  }.pack('side'=>'left', 'expand'=>'yes')

  # TkButton.new(frame) {
  Tk::Button.new(frame) {
    text '�����ɻ���'
    command proc{showCode 'clrpick'}
  }.pack('side'=>'left', 'expand'=>'yes')
}.pack('side'=>'bottom', 'fill'=>'x', 'pady'=>'2m')

# button ����
# TkButton.new($clrpick_demo, 'text'=>'�طʿ������� ...') {|b|
Tk::Button.new($clrpick_demo, 'text'=>'�طʿ������� ...') {|b|
  command(proc{setColor $clrpick_demo, b, 'background',
              ['background', 'highlightbackground']})
  pack('side'=>'top', 'anchor'=>'c', 'pady'=>'2m')
}

# TkButton.new($clrpick_demo, 'text'=>'���ʿ������� ...') {|b|
Tk::Button.new($clrpick_demo, 'text'=>'���ʿ������� ...') {|b|
  command(proc{setColor $clrpick_demo, b, 'foreground', ['foreground']})
  pack('side'=>'top', 'anchor'=>'c', 'pady'=>'2m')
}

def setColor(w,button,name,options)
  w.grab
  initialColor = button[name]
  color = Tk.chooseColor('title'=>"Choose a #{name} color", 'parent'=>w,
                         'initialcolor'=>initialColor)
  if color != ""
    setColor_helper(w,options,color)
  end

  w.grab('release')
end

def setColor_helper(w, options, color)
  options.each{|opt|
    begin
      w[opt] = color
    rescue
    end
  }
  TkWinfo.children(w).each{|child|
    setColor_helper child, options, color
  }
end


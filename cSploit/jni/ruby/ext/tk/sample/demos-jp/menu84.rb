# -*- coding: euc-jp -*-
#
# menus widget demo (called by 'widget')
#

# toplevel widget
if defined?($menu84_demo) && $menu84_demo
  $menu84_demo.destroy
  $menu84_demo = nil
end

# demo toplevel widget
$menu84_demo = TkToplevel.new {|w|
  title("File Selection Dialogs")
  iconname("menu84")
  positionWindow(w)
}

base_frame = TkFrame.new($menu84_demo).pack(:fill=>:both, :expand=>true)

begin
  windowingsystem = Tk.windowingsystem()
rescue
  windowingsystem = ""
end

# label
TkLabel.new(base_frame,'font'=>$font,'wraplength'=>'4i','justify'=>'left') {
  if $tk_platform['platform'] == 'macintosh' ||
      windowingsystem == "classic" || windowingsystem == "aqua"
    text("���Υ�����ɥ��ˤϥ��������ɥ�˥塼����ĥ�˥塼�С����դ����Ƥ��ޤ���Command+x ('x'�ϥ��ޥ�ɥ�������ܥ��³����ɽ������Ƥ���ʸ���Ǥ�) �ȥ����פ��뤳�Ȥˤ�äƤ���ܤε�ǽ��ƤӽФ����Ȥ��Ǥ��ޤ����Ǹ�Υ�˥塼�ϡ��ޥ����ǥ�����ɥ��γ��˥ɥ�å����뤳�Ȥˤ�äơ���Ω�����ѥ�åȤȤʤ�褦���ڤ��������Ȥ���ǽ�Ǥ���")
  else
    text("���Υ�����ɥ��ˤϥ��������ɥ�˥塼����ĥ�˥塼�С����դ����Ƥ��ޤ���Alt+x ('x'�ϥ�˥塼��ǲ����������줿ʸ���Ǥ�) �ȥ����פ��뤳�Ȥˤ�äƤ��˥塼��ƤӽФ����Ȥ��Ǥ��ޤ������������Ȥäơ���˥塼�֤��ư���뤳�Ȥ��ǽ�Ǥ�����˥塼��ɽ������Ƥ�����ˤϡ����߰��֤ι��ܤ򥹥ڡ������������򤷤��ꡢ�����������줿ʸ�������Ϥ��뤳�ȤǤ��ι��ܤ����򤷤��ꤹ�뤳�Ȥ��Ǥ��ޤ����⤷���ܤ˥�������졼���λ��꤬�ʤ���Ƥ����ʤ�С����λ��ꤵ�줿�������Ϥ�Ԥ����Ȥǡ���˥塼��ɽ�������뤳�Ȥʤ�ľ�ܤ��ι��ܤε�ǽ��ƤӽФ��ޤ����Ǹ�Υ�˥塼�ϡ���˥塼�κǽ�ι��ܤ����򤹤뤳�Ȥˤ�äơ���Ω�����ѥ�åȤȤʤ�褦���ڤ��������Ȥ���ǽ�Ǥ���")
  end
}.pack('side'=>'top')


menustatus = TkVariable.new("    ")
TkFrame.new(base_frame) {|frame|
  TkLabel.new(frame, 'textvariable'=>menustatus, 'relief'=>'sunken',
              'bd'=>1, 'font'=>['Helvetica', '10'],
              'anchor'=>'w').pack('side'=>'left', 'padx'=>2,
                                  'expand'=>true, 'fill'=>'both')
  pack('side'=>'bottom', 'fill'=>'x', 'pady'=>2)
}


# frame
TkFrame.new(base_frame) {|frame|
  TkButton.new(frame) {
    #text 'λ��'
    text '�Ĥ���'
    command proc{
      tmppath = $menu84_demo
      $menu84_demo = nil
      tmppath.destroy
    }
  }.pack('side'=>'left', 'expand'=>'yes')

  TkButton.new(frame) {
    text '�����ɻ���'
    command proc{showCode 'menu84'}
  }.pack('side'=>'left', 'expand'=>'yes')
}.pack('side'=>'bottom', 'fill'=>'x', 'pady'=>'2m')


# create menu frame
$menu84_frame = TkMenu.new($menu84_demo, 'tearoff'=>false)

# menu
TkMenu.new($menu84_frame, 'tearoff'=>false) {|m|
  $menu84_frame.add('cascade', 'label'=>'File', 'menu'=>m, 'underline'=>0)
  add('command', 'label'=>'Open...', 'command'=>proc{fail '�����ñ�ʤ�ǥ�Ǥ����顢"Open..." ���ܤε�ǽ���ä��������ƤϤ��ޤ���'})
  add('command', 'label'=>'New', 'command'=>proc{fail '�����ñ�ʤ�ǥ�Ǥ����顢"New" ���ܤε�ǽ���ä��������ƤϤ��ޤ���'})
  add('command', 'label'=>'Save', 'command'=>proc{fail '�����ñ�ʤ�ǥ�Ǥ����顢"Save" ���ܤε�ǽ���ä��������ƤϤ��ޤ���'})
  add('command', 'label'=>'Save As...', 'command'=>proc{fail '�����ñ�ʤ�ǥ�Ǥ����顢"Save As..." ���ܤε�ǽ���ä��������ƤϤ��ޤ���'})
  add('separator')
  add('command', 'label'=>'Print Setup...', 'command'=>proc{fail '�����ñ�ʤ�ǥ�Ǥ����顢"Print Setup..." ���ܤε�ǽ���ä��������ƤϤ��ޤ���'})
  add('command', 'label'=>'Print...', 'command'=>proc{fail '�����ñ�ʤ�ǥ�Ǥ����顢"Print..." ���ܤε�ǽ���ä��������ƤϤ��ޤ���'})
  add('separator')
  add('command', 'label'=>'Dismiss Menus Demo', 'command'=>proc{$menu84_demo.destroy})
}

if $tk_platform['platform'] == 'macintosh' ||
    windowingsystem = "classic" || windowingsystem = "aqua"
  modifier = 'Command'
elsif $tk_platform['platform'] == 'windows'
  modifier = 'Control'
else
  modifier = 'Meta'
end

TkMenu.new($menu84_frame, 'tearoff'=>false) {|m|
  $menu84_frame.add('cascade', 'label'=>'Basic', 'menu'=>m, 'underline'=>0)
  add('command', 'label'=>'Long entry that does nothing')
  ['A','B','C','D','E','F','G'].each{|c|
    add('command', 'label'=>"Print letter \"#{c}\"",
        'underline'=>14, 'accelerator'=>"Meta+#{c}",
        'command'=>proc{print c,"\n"}, 'accelerator'=>"#{modifier}+#{c}")
    $menu84_demo.bind("#{modifier}-#{c.downcase}", proc{print c,"\n"})
  }
}

TkMenu.new($menu84_frame, 'tearoff'=>false) {|m|
  $menu84_frame.add('cascade', 'label'=>'Cascades', 'menu'=>m, 'underline'=>0)
  add('command', 'label'=>'Print hello',
      'command'=>proc{print "Hello\n"},
      'accelerator'=>"#{modifier}+H", 'underline'=>6)
  $menu84_demo.bind("#{modifier}-h", proc{print "Hello\n"})
  add('command', 'label'=>'Print goodbye',
      'command'=>proc{print "Goodbye\n"},
      'accelerator'=>"#{modifier}+G", 'underline'=>6)
  $menu84_demo.bind("#{modifier}-g", proc{print "Goodbye\n"})

  TkMenu.new(m, 'tearoff'=>false) {|cascade_check|
    m.add('cascade', 'label'=>'Check button',
          'menu'=>cascade_check, 'underline'=>0)
    oil = TkVariable.new(0)
    add('check', 'label'=>'�����븡��', 'variable'=>oil)
    trans = TkVariable.new(0)
    add('check', 'label'=>'�ȥ�󥹥ߥå���󸡺�', 'variable'=>trans)
    brakes = TkVariable.new(0)
    add('check', 'label'=>'�֥졼������', 'variable'=>brakes)
    lights = TkVariable.new(0)
    add('check', 'label'=>'�饤�ȸ���', 'variable'=>lights)
    add('separator')
    add('command', 'label'=>'Show current values',
        'command'=>proc{showVars($menu84_demo,
                                 ['������', oil],
                                 ['�ȥ�󥹥ߥå����', trans],
                                 ['�֥졼��', brakes],
                                 ['�饤��', lights])} )
    invoke 1
    invoke 3
  }

  TkMenu.new(m, 'tearoff'=>false) {|cascade_radio|
    m.add('cascade', 'label'=>'Radio buttons',
          'menu'=>cascade_radio, 'underline'=>0)
    pointSize = TkVariable.new
    add('radio', 'label'=>'10 point', 'variable'=>pointSize, 'value'=>10)
    add('radio', 'label'=>'14 point', 'variable'=>pointSize, 'value'=>14)
    add('radio', 'label'=>'18 point', 'variable'=>pointSize, 'value'=>18)
    add('radio', 'label'=>'24 point', 'variable'=>pointSize, 'value'=>24)
    add('radio', 'label'=>'32 point', 'variable'=>pointSize, 'value'=>32)
    add('separator')
    style = TkVariable.new
    add('radio', 'label'=>'Roman', 'variable'=>style, 'value'=>'roman')
    add('radio', 'label'=>'Bold', 'variable'=>style, 'value'=>'bold')
    add('radio', 'label'=>'Italic', 'variable'=>style, 'value'=>'italic')
    add('separator')
    add('command', 'label'=>'�����ͤ�ɽ��',
        'command'=>proc{showVars($menu84_demo,
                                 ['pointSize', pointSize],
                                 ['style', style])} )
    invoke 1
    invoke 7
  }
}

TkMenu.new($menu84_frame, 'tearoff'=>false) {|m|
  $menu84_frame.add('cascade', 'label'=>'Icons', 'menu'=>m, 'underline'=>0)
  add('command', 'hidemargin'=>1,
      'bitmap'=>'@'+[$demo_dir,'..',
                      'images','pattern.xbm'].join(File::Separator),
      'command'=>proc{TkDialog.new('title'=>'Bitmap Menu Entry',
                                   'text'=>'���ʤ������򤷤���˥塼���ܤϡ�ʸ���������˥ӥåȥޥåץ��᡼���ǹ��ܤ�ɽ��������ΤǤ�������ʳ������Ǥϡ��ۤ��Υ�˥塼���ܤȤδ֤��ä˰㤤������櫓�ǤϤ���ޤ���',
                                   'bitmap'=>'', 'default'=>0,
                                   'buttons'=>'�Ĥ���')} )
  ['info', 'questhead', 'error'].each{|icon|
    add('command', 'bitmap'=>icon, 'hidemargin'=>1,
        'command'=>proc{print "You invoked the #{icon} bitmap\n"})
  }

  entryconfigure(2, :columnbreak=>true)
}

TkMenu.new($menu84_frame, 'tearoff'=>false) {|m|
  $menu84_frame.add('cascade', 'label'=>'More', 'menu'=>m, 'underline'=>0)
  [ 'An entry','Another entry','Does nothing','Does almost nothing',
    'Make life meaningful' ].each{|i|
    add('command', 'label'=>i,
        'command'=>proc{print "You invoked \"#{i}\"\n"})
  }

  m.entryconfigure('Does almost nothing',
                   'bitmap'=>'questhead', 'compound'=>'left',
                   'command'=>proc{
                     TkDialog.new('title'=>'Compound Menu Entry',
                                  'message'=>'���ʤ������򤷤���˥塼���ܤϡ��ӥåȥޥåץ��᡼����ʸ����Ȥ�Ʊ���˰�Ĥι��ܤ�ɽ������褦�ˤ�����ΤǤ�������ʳ������Ǥϡ��ۤ��Υ�˥塼���ܤȤδ֤��ä˰㤤������櫓�ǤϤ���ޤ���',
                                  'buttons'=>['λ��'], 'bitmap'=>'')
                   })
}

TkMenu.new($menu84_frame) {|m|
  $menu84_frame.add('cascade', 'label'=>'Colors', 'menu'=>m, 'underline'=>0)
  ['red', 'orange', 'yellow', 'green', 'blue'].each{|c|
    add('command', 'label'=>c, 'background'=>c,
        'command'=>proc{print "You invoked \"#{c}\"\n"})
  }
}

$menu84_demo.menu($menu84_frame)

TkMenu.bind('<MenuSelect>', proc{|w|
              begin
                label = w.entrycget('active', 'label')
              rescue
                label = "    "
              end
              menustatus.value = label
              Tk.update(true)
            }, '%W')

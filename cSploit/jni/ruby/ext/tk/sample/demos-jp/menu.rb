# -*- coding: euc-jp -*-
#
# menus widget demo (called by 'widget')
#

# toplevel widget ��¸�ߤ���к������
if defined?($menu_demo) && $menu_demo
  $menu_demo.destroy
  $menu_demo = nil
end

# demo �Ѥ� toplevel widget ������
$menu_demo = TkToplevel.new {|w|
  title("File Selection Dialogs")
  iconname("menu")
  positionWindow(w)
}

base_frame = TkFrame.new($menu_demo).pack(:fill=>:both, :expand=>true)

# menu frame ����
$menu_frame = TkFrame.new(base_frame, 'relief'=>'raised', 'bd'=>2)
$menu_frame.pack('side'=>'top', 'fill'=>'x')

begin
  windowingsystem = Tk.windowingsystem()
rescue
  windowingsystem = ""
end

# label ����
TkLabel.new(base_frame,'font'=>$font,'wraplength'=>'4i','justify'=>'left') {
  if $tk_platform['platform'] == 'macintosh' ||
      windowingsystem == "classic" || windowingsystem == "aqua"
    text("���Υ�����ɥ����͡��ʥ�˥塼�ȥ��������ɥ�˥塼���鹽������Ƥ��ޤ���Command-X �����Ϥ���ȡ�X�����ޥ�ɥ��������³����ɽ������Ƥ���ʸ���ʤ�С���������졼����Ȥä����ܵ�ư��Ԥ����Ȥ��Ǥ��ޤ�����˥塼�����桢�Ǹ�Τ�Τϡ����Υ�˥塼�κǽ�ι��ܤ����򤹤뤳�Ȥ���Ω�����뤳�Ȥ��Ǥ��ޤ���")
  else
    text("���Υ�����ɥ����͡��ʥ�˥塼�ȥ��������ɥ�˥塼���鹽������Ƥ��ޤ���Alt-X �����Ϥ���ȡ�X����˥塼�˥�������饤���դ���ɽ������Ƥ���ʸ���ʤ�С������ܡ��ɤ���λ��꤬�Ǥ��ޤ�����������ǥ�˥塼�Υȥ�С������ǽ�Ǥ�����˥塼�����ꤵ�줿�ݤˤϡ����ڡ��������Ǽ¹Ԥ��뤳�Ȥ��Ǥ��ޤ������뤤�ϡ���������饤���դ���ʸ�������Ϥ��뤳�ȤǤ�¹ԤǤ��ޤ�����˥塼�Υ���ȥ꤬��������졼������äƤ�����ϡ����Υ�������졼�������Ϥ��뤳�Ȥǥ�˥塼����ꤹ�뤳�Ȥʤ��˼¹Ԥ��뤳�Ȥ��Ǥ��ޤ�����˥塼�����桢�Ǹ�Τ�Τϡ����Υ�˥塼�κǽ�ι��ܤ����򤹤뤳�Ȥ���Ω�����뤳�Ȥ��Ǥ��ޤ���")
  end
}.pack('side'=>'top')

# frame ����
TkFrame.new(base_frame) {|frame|
  TkButton.new(frame) {
    #text 'λ��'
    text '�Ĥ���'
    command proc{
      tmppath = $menu_demo
      $menu_demo = nil
      tmppath.destroy
    }
  }.pack('side'=>'left', 'expand'=>'yes')

  TkButton.new(frame) {
    text '�����ɻ���'
    command proc{showCode 'menu'}
  }.pack('side'=>'left', 'expand'=>'yes')
}.pack('side'=>'bottom', 'fill'=>'x', 'pady'=>'2m')

# menu ����
TkMenubutton.new($menu_frame, 'text'=>'File', 'underline'=>0) {|m|
  pack('side'=>'left')
  TkMenu.new(m, 'tearoff'=>false) {|file_menu|
    m.configure('menu'=>file_menu)
    add('command', 'label'=>'���� ...', 'command'=>proc{fail '����ϡ��ǥ�Ǥ��Τ�"���� ..."���Ф��륢���������������Ƥ��ޤ���'})
    add('command', 'label'=>'����', 'command'=>proc{fail '����ϡ��ǥ�Ǥ��Τ�"����"���Ф��륢���������������Ƥ��ޤ���'})
    add('command', 'label'=>'��¸', 'command'=>proc{fail '����ϡ��ǥ�Ǥ��Τ�"��¸"���Ф��륢���������������Ƥ��ޤ���'})
    add('command', 'label'=>'��¸(����) ...', 'command'=>proc{fail '����ϡ��ǥ�Ǥ��Τ�"��¸(����) ..."���Ф��륢���������������Ƥ��ޤ���'})
    add('separator')
    add('command', 'label'=>'�ץ������� ...', 'command'=>proc{fail '����ϡ��ǥ�Ǥ��Τ�"�ץ������� ..."���Ф��륢���������������Ƥ��ޤ���'})
    add('command', 'label'=>'�ץ��� ...', 'command'=>proc{fail '����ϡ��ǥ�Ǥ��Τ�"�ץ��� ..."���Ф��륢���������������Ƥ��ޤ���'})
    add('separator')
    add('command', 'label'=>'��λ', 'command'=>proc{$menu_demo.destroy})
  }
}

if $tk_platform['platform'] == 'macintosh' ||
    windowingsystem == "classic" || windowingsystem == "aqua"
  modifier = 'Command'
elsif $tk_platform['platform'] == 'windows'
  modifier = 'Control'
else
  modifier = 'Meta'
end

TkMenubutton.new($menu_frame, 'text'=>'Basic', 'underline'=>0) {|m|
  pack('side'=>'left')
  TkMenu.new(m, 'tearoff'=>false) {|basic_menu|
    m.configure('menu'=>basic_menu)
    add('command', 'label'=>'���⤷�ʤ�Ĺ������ȥ�')
    ['A','B','C','D','E','F','G'].each{|c|
      # add('command', 'label'=>"ʸ�� \"#{c}\" �����", 'underline'=>4,
      add('command', 'label'=>"Print letter \"#{c}\" (ʸ�� \"#{c}\" �����)",
          'underline'=>14, 'accelerator'=>"Meta+#{c}",
          'command'=>proc{print c,"\n"}, 'accelerator'=>"#{modifier}+#{c}")
      $menu_demo.bind("#{modifier}-#{c.downcase}", proc{print c,"\n"})
    }
  }
}

TkMenubutton.new($menu_frame, 'text'=>'Cascades', 'underline'=>0) {|m|
  pack('side'=>'left')
  TkMenu.new(m, 'tearoff'=>false) {|cascade_menu|
    m.configure('menu'=>cascade_menu)
    add('command', 'label'=>'Print hello(����ˤ���)',
        'command'=>proc{print "Hello(����ˤ���)\n"},
        'accelerator'=>"#{modifier}+H", 'underline'=>6)
    $menu_demo.bind("#{modifier}-h", proc{print "Hello(����ˤ���)\n"})
    add('command', 'label'=>'Print goodbye(���褦�ʤ�)',
        'command'=>proc{print "Goodbye(���褦�ʤ�)\n"},
        'accelerator'=>"#{modifier}+G", 'underline'=>6)
    $menu_demo.bind("#{modifier}-g", proc{print "Goodbye(���褦�ʤ�)\n"})

    # TkMenu.new(m, 'tearoff'=>false) {|cascade_check|
    TkMenu.new(cascade_menu, 'tearoff'=>false) {|cascade_check|
      cascade_menu.add('cascade', 'label'=>'Check buttons(�����å��ܥ���)',
                       'menu'=>cascade_check, 'underline'=>0)
      oil = TkVariable.new(0)
      add('check', 'label'=>'����������', 'variable'=>oil)
      trans = TkVariable.new(0)
      add('check', 'label'=>'�ȥ�󥹥ߥå��������', 'variable'=>trans)
      brakes = TkVariable.new(0)
      add('check', 'label'=>'�֥졼������', 'variable'=>brakes)
      lights = TkVariable.new(0)
      add('check', 'label'=>'�饤������', 'variable'=>lights)
      add('separator')
      add('command', 'label'=>'���ߤ��ͤ�ɽ��',
          'command'=>proc{showVars($menu_demo,
                                   ['����������', oil],
                                   ['�ȥ�󥹥ߥå��������', trans],
                                   ['�֥졼������', brakes],
                                   ['�饤������', lights])} )
      invoke 1
      invoke 3
    }

    #TkMenu.new(m, 'tearoff'=>false) {|cascade_radio|
    TkMenu.new(cascade_menu, 'tearoff'=>false) {|cascade_radio|
      cascade_menu.add('cascade', 'label'=>'Radio buttons(�饸���ܥ���)',
                       'menu'=>cascade_radio, 'underline'=>0)
      pointSize = TkVariable.new
      add('radio', 'label'=>'10 �ݥ����', 'variable'=>pointSize, 'value'=>10)
      add('radio', 'label'=>'14 �ݥ����', 'variable'=>pointSize, 'value'=>14)
      add('radio', 'label'=>'18 �ݥ����', 'variable'=>pointSize, 'value'=>18)
      add('radio', 'label'=>'24 �ݥ����', 'variable'=>pointSize, 'value'=>24)
      add('radio', 'label'=>'32 �ݥ����', 'variable'=>pointSize, 'value'=>32)
      add('separator')
      style = TkVariable.new
      add('radio', 'label'=>'���ޥ�', 'variable'=>style, 'value'=>'roman')
      add('radio', 'label'=>'�ܡ����', 'variable'=>style, 'value'=>'bold')
      add('radio', 'label'=>'������å�', 'variable'=>style, 'value'=>'italic')
      add('separator')
      add('command', 'label'=>'���ߤ��ͤ�ɽ��',
          'command'=>proc{showVars($menu_demo,
                                   ['�ݥ���ȥ�����', pointSize],
                                   ['��������', style])} )
      invoke 1
      invoke 7
    }
  }
}

TkMenubutton.new($menu_frame, 'text'=>'Icons', 'underline'=>0) {|m|
  pack('side'=>'left')
  TkMenu.new(m, 'tearoff'=>false) {|icon_menu|
    m.configure('menu'=>icon_menu)
    add('command',
        'bitmap'=>'@'+[$demo_dir,'..',
                        'images','pattern.xbm'].join(File::Separator),
        'command'=>proc{TkDialog.new('title'=>'Bitmap Menu Entry',
                                     'text'=>'�����ʤ������򤷤���˥塼�ι��ܤϥƥ����ȤǤϤʤ��ӥåȥޥåפ�ɽ�����Ƥ��ޤ���������ʳ������Ǥ�¾�Υ�˥塼���ܤ��Ѥ��ޤ���',
                                     'bitmap'=>'', 'default'=>0,
                                     'buttons'=>'λ��')} )
    ['info', 'questhead', 'error'].each{|icon|
      add('command', 'bitmap'=>icon,
          'command'=>proc{print "You invoked the #{icon} bitmap\n"})
    }
  }
}

TkMenubutton.new($menu_frame, 'text'=>'More', 'underline'=>0) {|m|
  pack('side'=>'left')
  TkMenu.new(m, 'tearoff'=>false) {|more_menu|
    m.configure('menu'=>more_menu)
    [ '����ȥ�','�̤Υ���ȥ�','���⤷�ʤ�','�ۤȤ�ɲ��⤷�ʤ�',
      '������յ������Τ�' ].each{|i|
      add('command', 'label'=>i,
          'command'=>proc{print "You invoked \"#{i}\"\n"})
    }
  }
}

TkMenubutton.new($menu_frame, 'text'=>'Colors', 'underline'=>0) {|m|
  pack('side'=>'left')
  TkMenu.new(m) {|colors_menu|
    m.configure('menu'=>colors_menu)
    ['red', 'orange', 'yellow', 'green', 'blue'].each{|c|
      add('command', 'label'=>c, 'background'=>c,
          'command'=>proc{print "You invoked \"#{c}\"\n"})
    }
  }
}

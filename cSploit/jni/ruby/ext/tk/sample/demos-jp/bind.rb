# -*- coding: euc-jp -*-
#
# text (tag bindings) widget demo (called by 'widget')
#

# toplevel widget ��¸�ߤ���к������
if defined?($bind_demo) && $bind_demo
  $bind_demo.destroy
  $bind_demo = nil
end

# demo �Ѥ� toplevel widget ������
$bind_demo = TkToplevel.new {|w|
  title("Text Demonstration - Tag Bindings")
  iconname("bind")
  positionWindow(w)
}

base_frame = TkFrame.new($bind_demo).pack(:fill=>:both, :expand=>true)

# frame ����
TkFrame.new(base_frame) {|frame|
  TkButton.new(frame) {
    #text 'λ��'
    text '�Ĥ���'
    command proc{
      tmppath = $bind_demo
      $bind_demo = nil
      tmppath.destroy
    }
  }.pack('side'=>'left', 'expand'=>'yes')

  TkButton.new(frame) {
    text '�����ɻ���'
    command proc{showCode 'bind'}
  }.pack('side'=>'left', 'expand'=>'yes')
}.pack('side'=>'bottom', 'fill'=>'x', 'pady'=>'2m')

# bind �ѥ᥽�å�
def tag_binding_for_bind_demo(tag, enter_style, leave_style)
  tag.bind('Any-Enter', proc{tag.configure enter_style})
  tag.bind('Any-Leave', proc{tag.configure leave_style})
end

# text ����
txt = TkText.new(base_frame){|t|
  # ����
  setgrid 'true'
  #width  60
  #height 24
  font $font
  wrap 'word'
  TkScrollbar.new(base_frame) {|s|
    pack('side'=>'right', 'fill'=>'y')
    command proc{|*args| t.yview(*args)}
    t.yscrollcommand proc{|first,last| s.set first,last}
  }
  pack('expand'=>'yes', 'fill'=>'both')

  # ������������
  if TkWinfo.depth($root).to_i > 1
    tagstyle_bold = {'background'=>'#43ce80', 'relief'=>'raised',
                     'borderwidth'=>1}
    tagstyle_normal = {'background'=>'', 'relief'=>'flat'}
  else
    tagstyle_bold = {'foreground'=>'white', 'background'=>'black'}
    tagstyle_normal = {'foreground'=>'', 'background'=>''}
  end

  # �ƥ���������
  insert 'insert', "�ƥ�����widget��ɽ��������������椹��Τ�Ʊ�������Υᥫ�˥����Ȥäơ��ƥ����Ȥ�Tcl�Υ��ޥ�ɤ������Ƥ뤳�Ȥ��Ǥ��ޤ�������ˤ�ꡢ�ޥ����䥭���ܡ��ɤΥ��������������Tcl�Υ��ޥ�ɤ��¹Ԥ����褦�ˤʤ�ޤ����㤨�С����Υ����Х��Υǥ�ץ����ˤĤ��Ƥ�����ʸ�ˤϤ��Τ褦�ʥ������Ĥ��Ƥ��ޤ����ޥ���������ʸ�ξ�˻��äƤ���������ʸ�����ꡢ�ܥ���1�򲡤��Ȥ��������Υǥ⤬�Ϥޤ�ޤ���

"
  insert('end', '1. �����Х� widget �˺�뤳�ȤΤǤ��륢���ƥ�μ������Ƥ˴ؤ��륵��ץ롣', (d1 = TkTextTag.new(t)) )
  insert('end', "\n\n")
  insert('end', '2. ��ñ�� 2�����Υץ�åȡ��ǡ�����ɽ������ư�������Ȥ��Ǥ��롣', (d2 = TkTextTag.new(t)) )
  insert('end', "\n\n")
  insert('end', '3. �ƥ����ȥ����ƥ�Υ��󥫡��ȹ�·����',
         (d3 = TkTextTag.new(t)) )
  insert('end', "\n\n")
  insert('end', '4. �饤�󥢥��ƥ�Τ���������Ƭ�η��Υ��ǥ�����',
         (d4 = TkTextTag.new(t)) )
  insert('end', "\n\n")
  insert('end', '5. ���֥��ȥåפ��ѹ����뤿��ε�ǽ�Ĥ��Υ롼�顼��',
         (d5 = TkTextTag.new(t)) )
  insert('end', "\n\n")
  insert('end',
         '6. �����Х����ɤ���äƥ������뤹��Τ��򼨤�����åɡ�',
         (d6 = TkTextTag.new(t)) )

  # binding
  [d1, d2, d3, d4, d5, d6].each{|tag|
    tag_binding_for_bind_demo(tag, tagstyle_bold, tagstyle_normal)
  }
  d1.bind('1',
          proc{
            eval_samplecode(`cat #{[$demo_dir,'items.rb'].join(File::Separator)}`, 'items.rb')
          })
  d2.bind('1',
          proc{
            eval_samplecode(`cat #{[$demo_dir,'plot.rb'].join(File::Separator)}`, 'plot.rb')
          })
  d3.bind('1',
          proc{
            eval_samplecode(`cat #{[$demo_dir,'ctext.rb'].join(File::Separator)}`, 'ctext.rb')
          })
  d4.bind('1',
          proc{
            eval_samplecode(`cat #{[$demo_dir,'arrow.rb'].join(File::Separator)}`, 'arrow.rb')
          })
  d5.bind('1',
          proc{
            eval_samplecode(`cat #{[$demo_dir,'ruler.rb'].join(File::Separator)}`, 'ruler.rb')
          })
  d6.bind('1',
          proc{
            eval_samplecode(`cat #{[$demo_dir,'cscroll.rb'].join(File::Separator)}`, 'cscroll.rb')
          })

  TkTextMarkInsert.new(t, '0.0')
  configure('state','disabled')
}

txt.width  60
txt.height 24

#! /usr/local/bin/ruby -d
# -*- encoding: euc-jp -*-
# -d ���ץ������դ����, �ǥХå������ɽ������.

# tcltk �饤�֥��Υ���ץ�

# �ޤ�, �饤�֥��� require ����.
require "tcltk"

# �ʲ���, Test1 �Υ��󥹥��󥹤� initialize() ��,
# tcl/tk �˴ؤ��������Ԥ���Ǥ���.
# ɬ�����⤽�Τ褦�ˤ���ɬ�פ�̵��,
# (�⤷, �������������) class �γ��� tcl/tk �˴ؤ��������ԤäƤ��ɤ�.

class Test1
  # �����(���󥿥ץ꥿���������ƥ��������åȤ���������).
  def initialize()

    #### �Ȥ����Τ��ޤ��ʤ�

    # ���󥿥ץ꥿������.
    ip = TclTkInterpreter.new()
    # ���ޥ�ɤ��б����륪�֥������Ȥ� c �����ꤷ�Ƥ���.
    c = ip.commands()
    # ���Ѥ��륳�ޥ�ɤ��б����륪�֥������Ȥ��ѿ�������Ƥ���.
    append, bind, button, destroy, incr, info, label, place, set, wm =
      c.values_at(
      "append", "bind", "button", "destroy", "incr", "info", "label", "place",
      "set", "wm")

    #### tcl/tk �Υ��ޥ�ɤ��б����륪�֥�������(TclTkCommand)�����

    # �¹Ԥ������, e() �᥽�åɤ�Ȥ�.
    # (�ʲ���, tcl/tk �ˤ����� info command r* ��¹�.)
    print info.e("command", "r*"), "\n"
    # ������, �ޤȤ᤿ʸ����ˤ��Ƥ�Ʊ��.
    print info.e("command r*"), "\n"
    # �ѿ����Ѥ��ʤ��Ȥ�¹ԤǤ��뤬, �����᤬����.
    print c["info"].e("command", "r*"), "\n"
    # ���󥿥ץ꥿�Υ᥽�åɤȤ��Ƥ�¹ԤǤ��뤬, ��Ψ������.
    print ip.info("command", "r*"), "\n"

    ####

    # �ʲ�, �����������֥������Ȥ��ѿ����������Ƥ����ʤ���
    # GC ���оݤˤʤäƤ��ޤ�.

    #### tcl/tk ���ѿ����б����륪�֥�������(TclTkVariable)�����

    # ������Ʊ�����ͤ����ꤹ��.
    v1 = TclTkVariable.new(ip, "20")
    # �ɤ߽Ф��� get �᥽�åɤ�Ȥ�.
    print v1.get(), "\n"
    # ����� set �᥽�åɤ�Ȥ�.
    v1.set(40)
    print v1.get(), "\n"
    # set ���ޥ�ɤ�Ȥä��ɤ߽Ф�, ����ϲ�ǽ���������᤬����.
    # e() �᥽�å����ΰ�����ľ�� TclTkObject ����ͤ�񤤤Ƥ��ɤ�.
    set.e(v1, 30)
    print set.e(v1), "\n"
    # tcl/tk �Υ��ޥ�ɤ��ѿ������Ǥ���.
    incr.e(v1)
    print v1.get(), "\n"
    append.e(v1, 10)
    print v1.get(), "\n"

    #### tcl/tk �Υ��������åȤ��б����륪�֥�������(TclTkWidget)�����

    # �롼�ȥ��������åȤ���Ф�.
    root = ip.rootwidget()
    # ���������åȤ����.
    root.e("configure -height 300 -width 300")
    # �����ȥ���դ���Ȥ��� wm ��Ȥ�.
    wm.e("title", root, $0)
    # �ƥ��������åȤȥ��ޥ�ɤ���ꤷ��, ���������åȤ���.
    l1 = TclTkWidget.new(ip, root, label, "-text {type `x' to print}")
    # place �����ɽ�������.
    place.e(l1, "-x 0 -rely 0.0 -relwidth 1 -relheight 0.1")
    # ���ޥ��̾��ʸ����ǻ��ꤷ�Ƥ��ɤ���, �����᤬����.
    # (���ޥ��̾����Ω���������Ǥʤ���Фʤ�ʤ�.)
    l2 = TclTkWidget.new(ip, root, "label")
    # ���������åȤ����.
    l2.e("configure -text {type `q' to exit}")
    place.e(l2, "-x 0 -rely 0.1 -relwidth 1 -relheight 0.1")

    #### tcl/tk �Υ�����Хå����б����륪�֥�������(TclTkCallback)�����

    # ������Хå�����������.
    c1 = TclTkCallback.new(ip, proc{sample(ip, root)})
    # ������Хå�����ĥ��������åȤ���������.
    b1 = TclTkWidget.new(ip, root, button, "-text sample -command", c1)
    place.e(b1, "-x 0 -rely 0.2 -relwidth 1 -relheight 0.1")
    # ���٥�ȥ롼�פ�ȴ����ˤ� destroy.e(root) ����.
    c2 = TclTkCallback.new(ip, proc{destroy.e(root)})
    b2 = TclTkWidget.new(ip, root, button, "-text exit -command", c2)
    place.e(b2, "-x 0 -rely 0.3 -relwidth 1 -relheight 0.1")

    #### ���٥�ȤΥХ����
    # script ���ɲ� (bind tag sequence +script) �Ϻ��ΤȤ���Ǥ��ʤ�.
    # (���ƥ졼���ѿ������꤬���ޤ������ʤ�.)

    # ����Ū�ˤϥ��������åȤ��Ф��륳����Хå���Ʊ��.
    c3 = TclTkCallback.new(ip, proc{print("q pressed\n"); destroy.e(root)})
    bind.e(root, "q", c3)
    # bind ���ޥ�ɤ� % �ִ��ˤ��ѥ�᡼���������ꤿ���Ȥ���,
    # proc{} �θ���ʸ����ǻ��ꤹ���,
    # �ִ���̤򥤥ƥ졼���ѿ����̤��Ƽ�����뤳�Ȥ��Ǥ���.
    # ������ proc{} �θ���ʸ�����,
    # bind ���ޥ�ɤ�Ϳ���륳����Хå��ʳ��ǻ��ꤷ�ƤϤ����ʤ�.
    c4 = TclTkCallback.new(ip, proc{|i| print("#{i} pressed\n")}, "%A")
    bind.e(root, "x", c4)
    # TclTkCallback �� GC ���оݤˤ��������,
    # dcb() (�ޤ��� deletecallbackkeys()) ����ɬ�פ�����.
    cb = [c1, c2, c3, c4]
    c5 = TclTkCallback.new(ip, proc{|w| TclTk.dcb(cb, root, w)}, "%W")
    bind.e(root, "<Destroy>", c5)
    cb.push(c5)

    #### tcl/tk �Υ��᡼�����б����륪�֥�������(TclTkImage)�����

    # �ǡ�������ꤷ����������.
    i1 = TclTkImage.new(ip, "photo", "-file maru.gif")
    # ��٥��ĥ���դ��Ƥߤ�.
    l3 = TclTkWidget.new(ip, root, label, "-relief raised -image", i1)
    place.e(l3, "-x 0 -rely 0.4 -relwidth 0.2 -relheight 0.2")
    # ���Υ��᡼�����������Ƹ������.
    i2 = TclTkImage.new(ip, "photo")
    # ���᡼��������.
    i2.e("copy", i1)
    i2.e("configure -gamma 0.5")
    l4 = TclTkWidget.new(ip, root, label, "-relief raised -image", i2)
    place.e(l4, "-relx 0.2 -rely 0.4 -relwidth 0.2 -relheight 0.2")

    ####
  end

  # ����ץ�Τ���Υ��������åȤ���������.
  def sample(ip, parent)
    bind, button, destroy, grid, toplevel, wm = ip.commands().values_at(
      "bind", "button", "destroy", "grid", "toplevel", "wm")

    ## toplevel

    # ������������ɥ��򳫤��ˤ�, toplevel ��Ȥ�.
    t1 = TclTkWidget.new(ip, parent, toplevel)
    # �����ȥ���դ��Ƥ���
    wm.e("title", t1, "sample")

    # ���������åȤ��˲����줿�Ȥ�, ������Хå��� GC ���оݤˤʤ�褦�ˤ���.
    cb = []
    cb.push(c = TclTkCallback.new(ip, proc{|w| TclTk.dcb(cb, t1, w)}, "%W"))
    bind.e(t1, "<Destroy>", c)

    # �ܥ��������.
    wid = []
    # toplevel ���������åȤ��˲�����ˤ� destroy ����.
    cb.push(c = TclTkCallback.new(ip, proc{destroy.e(t1)}))
    wid.push(TclTkWidget.new(ip, t1, button, "-text close -command", c))
    cb.push(c = TclTkCallback.new(ip, proc{test_label(ip, t1)}))
    wid.push(TclTkWidget.new(ip, t1, button, "-text label -command", c))
    cb.push(c = TclTkCallback.new(ip, proc{test_button(ip, t1)}))
    wid.push(TclTkWidget.new(ip, t1, button, "-text button -command", c))
    cb.push(c = TclTkCallback.new(ip, proc{test_checkbutton(ip, t1)}))
    wid.push(TclTkWidget.new(ip, t1, button, "-text checkbutton -command", c))
    cb.push(c = TclTkCallback.new(ip, proc{test_radiobutton(ip, t1)}))
    wid.push(TclTkWidget.new(ip, t1, button, "-text radiobutton -command", c))
    cb.push(c = TclTkCallback.new(ip, proc{test_scale(ip, t1)}))
    wid.push(TclTkWidget.new(ip, t1, button, "-text scale -command", c))
    cb.push(c = TclTkCallback.new(ip, proc{test_entry(ip, t1)}))
    wid.push(TclTkWidget.new(ip, t1, button, "-text entry -command", c))
    cb.push(c = TclTkCallback.new(ip, proc{test_text(ip, t1)}))
    wid.push(TclTkWidget.new(ip, t1, button, "-text text -command", c))
    cb.push(c = TclTkCallback.new(ip, proc{test_raise(ip, t1)}))
    wid.push(TclTkWidget.new(ip, t1, button, "-text raise/lower -command", c))
    cb.push(c = TclTkCallback.new(ip, proc{test_modal(ip, t1)}))
    wid.push(TclTkWidget.new(ip, t1, button, "-text message/modal -command",
      c))
    cb.push(c = TclTkCallback.new(ip, proc{test_menu(ip, t1)}))
    wid.push(TclTkWidget.new(ip, t1, button, "-text menu -command", c))
    cb.push(c = TclTkCallback.new(ip, proc{test_listbox(ip, t1)}))
    wid.push(TclTkWidget.new(ip, t1, button, "-text listbox/scrollbar",
      "-command", c))
    cb.push(c = TclTkCallback.new(ip, proc{test_canvas(ip, t1)}))
    wid.push(TclTkWidget.new(ip, t1, button, "-text canvas -command", c))

    # grid ��ɽ������.
    ro = co = 0
    wid.each{|w|
      grid.e(w, "-row", ro, "-column", co, "-sticky news")
      ro += 1
      if ro == 7
        ro = 0
        co += 1
      end
    }
  end

  # inittoplevel(ip, parent, title)
  #   �ʲ��ν�����ޤȤ�ƹԤ�.
  #       1. toplevel ���������åȤ��������.
  #       2. ������Хå�����Ͽ����������Ѱդ�, toplevel ���������åȤ�
  #         <Destroy> ���٥�Ȥ˥�����Хå����������³������Ͽ����.
  #       3. �������ܥ������.
  #     �������� toplevel ���������å�, �������ܥ���, ������Хå���Ͽ���ѿ�
  #     ���֤�.
  #   ip: ���󥿥ץ꥿
  #   parent: �ƥ��������å�
  #   title: toplevel ���������åȤΥ�����ɥ��Υ����ȥ�
  def inittoplevel(ip, parent, title)
    bind, button, destroy, toplevel, wm = ip.commands().values_at(
      "bind", "button", "destroy", "toplevel", "wm")

    # ������������ɥ��򳫤��ˤ�, toplevel ��Ȥ�.
    t1 = TclTkWidget.new(ip, parent, toplevel)
    # �����ȥ���դ��Ƥ���
    wm.e("title", t1, title)

    # ���������åȤ��˲����줿�Ȥ�, ������Хå��� GC ���оݤˤʤ�褦�ˤ���.
    cb = []
    cb.push(c = TclTkCallback.new(ip, proc{|w| TclTk.dcb(cb, t1, w)}, "%W"))
    bind.e(t1, "<Destroy>", c)
    # close �ܥ�����äƤ���.
    # toplevel ���������åȤ��˲�����ˤ� destroy ����.
    cb.push(c = TclTkCallback.new(ip, proc{destroy.e(t1)}))
    b1 = TclTkWidget.new(ip, t1, button, "-text close -command", c)

    return t1, b1, cb
  end

  # label �Υ���ץ�.
  def test_label(ip, parent)
    button, global, label, pack = ip.commands().values_at(
      "button", "global", "label", "pack")
    t1, b1, cb = inittoplevel(ip, parent, "label")

    ## label

    # ������ʷ��Υ�٥�.
    l1 = TclTkWidget.new(ip, t1, label, "-text {default(flat)}")
    l2 = TclTkWidget.new(ip, t1, label, "-text raised -relief raised")
    l3 = TclTkWidget.new(ip, t1, label, "-text sunken -relief sunken")
    l4 = TclTkWidget.new(ip, t1, label, "-text groove -relief groove")
    l5 = TclTkWidget.new(ip, t1, label, "-text ridge -relief ridge")
    l6 = TclTkWidget.new(ip, t1, label, "-bitmap error")
    l7 = TclTkWidget.new(ip, t1, label, "-bitmap questhead")

    # pack ���Ƥ�ɽ�������.
    pack.e(b1, l1, l2, l3, l4, l5, l6, l7, "-pady 3")

    ## -textvariable

    # tcltk �饤�֥��μ����Ǥ�, ������Хå��� tcl/tk ��``��³��''���̤���
    # �ƤФ��. �������ä�, ������Хå������(���)�ѿ��˥�����������Ȥ���,
    # global ����ɬ�פ�����.
    # global ���������ѿ����ͤ����ꤷ�Ƥ��ޤ��ȥ��顼�ˤʤ�Τ�,
    # tcl/tk �ˤ�����ɽ����������������, �ºݤ��ͤ����ꤷ�ʤ��褦��,
    # 2 ���ܤΰ����ˤ� nil ��Ϳ����.
    v1 = TclTkVariable.new(ip, nil)
    global.e(v1)
    v1.set(100)
    # -textvariable ���ѿ������ꤹ��.
    l6 = TclTkWidget.new(ip, t1, label, "-textvariable", v1)
    # ������Хå����椫���ѿ�������.
    cb.push(c = TclTkCallback.new(ip, proc{
      global.e(v1); v1.set(v1.get().to_i + 10)}))
    b2 = TclTkWidget.new(ip, t1, button, "-text +10 -command", c)
    cb.push(c = TclTkCallback.new(ip, proc{
      global.e(v1); v1.set(v1.get().to_i - 10)}))
    b3 = TclTkWidget.new(ip, t1, button, "-text -10 -command", c)
    pack.e(l6, b2, b3)
  end

  # button �Υ���ץ�.
  def test_button(ip, parent)
    button, pack = ip.commands().values_at("button", "pack")
    t1, b1, cb = inittoplevel(ip, parent, "button")

    ## button

    # ������Хå���ǻ��Ȥ����ѿ������������Ƥ����ʤ���Фʤ�ʤ�.
    b3 = b4 = nil
    cb.push(c = TclTkCallback.new(ip, proc{b3.e("flash"); b4.e("flash")}))
    b2 = TclTkWidget.new(ip, t1, button, "-text flash -command", c)
    cb.push(c = TclTkCallback.new(ip, proc{b2.e("configure -state normal")}))
    b3 = TclTkWidget.new(ip, t1, button, "-text normal -command", c)
    cb.push(c = TclTkCallback.new(ip, proc{b2.e("configure -state disabled")}))
    b4 = TclTkWidget.new(ip, t1, button, "-text disable -command", c)
    pack.e(b1, b2, b3, b4)
  end

  # checkbutton �Υ���ץ�.
  def test_checkbutton(ip, parent)
    checkbutton, global, pack = ip.commands().values_at(
      "checkbutton", "global", "pack")
    t1, b1, cb = inittoplevel(ip, parent, "checkbutton")

    ## checkbutton

    v1 = TclTkVariable.new(ip, nil)
    global.e(v1)
    # -variable ���ѿ������ꤹ��.
    ch1 = TclTkWidget.new(ip, t1, checkbutton, "-onvalue on -offvalue off",
      "-textvariable", v1, "-variable", v1)
    pack.e(b1, ch1)
  end

  # radiobutton �Υ���ץ�.
  def test_radiobutton(ip, parent)
    global, label, pack, radiobutton = ip.commands().values_at(
      "global", "label", "pack", "radiobutton")
    t1, b1, cb = inittoplevel(ip, parent, "radiobutton")

    ## radiobutton

    v1 = TclTkVariable.new(ip, nil)
    global.e(v1)
    # �̥륹�ȥ�󥰤� "{}" �ǻ��ꤹ��.
    v1.set("{}")
    l1 = TclTkWidget.new(ip, t1, label, "-textvariable", v1)
    # -variable ��Ʊ���ѿ�����ꤹ���Ʊ�����롼�פˤʤ�.
    ra1 = TclTkWidget.new(ip, t1, radiobutton,
      "-text radio1 -value r1 -variable", v1)
    ra2 = TclTkWidget.new(ip, t1, radiobutton,
      "-text radio2 -value r2 -variable", v1)
    cb.push(c = TclTkCallback.new(ip, proc{global.e(v1); v1.set("{}")}))
    ra3 = TclTkWidget.new(ip, t1, radiobutton,
      "-text clear -value r3 -variable", v1, "-command", c)
    pack.e(b1, l1, ra1, ra2, ra3)
  end

  # scale �Υ���ץ�.
  def test_scale(ip, parent)
    global, pack, scale = ip.commands().values_at(
      "global", "pack", "scale")
    t1, b1, cb = inittoplevel(ip, parent, "scale")

    ## scale

    v1 = TclTkVariable.new(ip, nil)
    global.e(v1)
    v1.set(219)
    # ������Хå���ǻ��Ȥ����ѿ������������Ƥ����ʤ���Фʤ�ʤ�.
    sca1 = nil
    cb.push(c = TclTkCallback.new(ip, proc{global.e(v1); v = v1.get();
      sca1.e("configure -background", format("#%02x%02x%02x", v, v, v))}))
    sca1 = TclTkWidget.new(ip, t1, scale,
      "-label scale -orient h -from 0 -to 255 -variable", v1, "-command", c)
    pack.e(b1, sca1)
  end

  # entry �Υ���ץ�.
  def test_entry(ip, parent)
    button, entry, global, pack = ip.commands().values_at(
      "button", "entry", "global", "pack")
    t1, b1, cb = inittoplevel(ip, parent, "entry")

    ## entry

    v1 = TclTkVariable.new(ip, nil)
    global.e(v1)
    # �̥륹�ȥ�󥰤� "{}" �ǻ��ꤹ��.
    v1.set("{}")
    en1 = TclTkWidget.new(ip, t1, entry, "-textvariable", v1)
    cb.push(c = TclTkCallback.new(ip, proc{
      global.e(v1); print(v1.get(), "\n"); v1.set("{}")}))
    b2 = TclTkWidget.new(ip, t1, button, "-text print -command", c)
    pack.e(b1, en1, b2)
  end

  # text �Υ���ץ�.
  def test_text(ip, parent)
    button, pack, text = ip.commands().values_at(
      "button", "pack", "text")
    t1, b1, cb = inittoplevel(ip, parent, "text")

    ## text

    te1 = TclTkWidget.new(ip, t1, text)
    cb.push(c = TclTkCallback.new(ip, proc{
      # 1 ���ܤ� 0 ʸ���ܤ���Ǹ�ޤǤ�ɽ����, �������.
      print(te1.e("get 1.0 end")); te1.e("delete 1.0 end")}))
    b2 = TclTkWidget.new(ip, t1, button, "-text print -command", c)
    pack.e(b1, te1, b2)
  end

  # raise/lower �Υ���ץ�.
  def test_raise(ip, parent)
    button, frame, lower, pack, raise = ip.commands().values_at(
      "button", "frame", "lower", "pack", "raise")
    t1, b1, cb = inittoplevel(ip, parent, "raise/lower")

    ## raise/lower

    # button �򱣤��ƥ��ȤΤ����, frame ��Ȥ�.
    f1 = TclTkWidget.new(ip, t1, frame)
    # ������Хå���ǻ��Ȥ����ѿ������������Ƥ����ʤ���Фʤ�ʤ�.
    b2 = nil
    cb.push(c = TclTkCallback.new(ip, proc{raise.e(f1, b2)}))
    b2 = TclTkWidget.new(ip, t1, button, "-text raise -command", c)
    cb.push(c = TclTkCallback.new(ip, proc{lower.e(f1, b2)}))
    b3 = TclTkWidget.new(ip, t1, button, "-text lower -command", c)
    lower.e(f1, b3)

    pack.e(b2, b3, "-in", f1)
    pack.e(b1, f1)
  end

  # modal �ʥ��������åȤΥ���ץ�.
  def test_modal(ip, parent)
    button, frame, message, pack, tk_chooseColor, tk_getOpenFile,
      tk_messageBox = ip.commands().values_at(
      "button", "frame", "message", "pack", "tk_chooseColor",
      "tk_getOpenFile", "tk_messageBox")
    # �ǽ�� load ����Ƥ��ʤ��饤�֥��� ip.commands() ��¸�ߤ��ʤ��Τ�,
    # TclTkLibCommand ����������ɬ�פ�����.
    tk_dialog = TclTkLibCommand.new(ip, "tk_dialog")
    t1, b1, cb = inittoplevel(ip, parent, "message/modal")

    ## message

    mes = "����� message ���������åȤΥƥ��ȤǤ�."
    mes += "�ʲ��� modal �ʥ��������åȤΥƥ��ȤǤ�."
    me1 = TclTkWidget.new(ip, t1, message, "-text {#{mes}}")

    ## modal

    # tk_messageBox
    cb.push(c = TclTkCallback.new(ip, proc{
      print tk_messageBox.e("-type yesnocancel -message messageBox",
      "-icon error -default cancel -title messageBox"), "\n"}))
    b2 = TclTkWidget.new(ip, t1, button, "-text messageBox -command", c)
    # tk_dialog
    cb.push(c = TclTkCallback.new(ip, proc{
      # ���������å�̾���������뤿��˥��ߡ��� frame ������.
      print tk_dialog.e(TclTkWidget.new(ip, t1, frame),
      "dialog dialog error 2 yes no cancel"), "\n"}))
    b3 = TclTkWidget.new(ip, t1, button, "-text dialog -command", c)
    # tk_chooseColor
    cb.push(c = TclTkCallback.new(ip, proc{
      print tk_chooseColor.e("-title chooseColor"), "\n"}))
    b4 = TclTkWidget.new(ip, t1, button, "-text chooseColor -command", c)
    # tk_getOpenFile
    cb.push(c = TclTkCallback.new(ip, proc{
      print tk_getOpenFile.e("-defaultextension .rb",
      "-filetypes {{{Ruby Script} {.rb}} {{All Files} {*}}}",
      "-title getOpenFile"), "\n"}))
    b5 = TclTkWidget.new(ip, t1, button, "-text getOpenFile -command", c)

    pack.e(b1, me1, b2, b3, b4, b5)
  end

  # menu �Υ���ץ�.
  def test_menu(ip, parent)
    global, menu, menubutton, pack = ip.commands().values_at(
      "global", "menu", "menubutton", "pack")
    tk_optionMenu = TclTkLibCommand.new(ip, "tk_optionMenu")
    t1, b1, cb = inittoplevel(ip, parent, "menu")

    ## menu

    # menubutton ����������.
    mb1 = TclTkWidget.new(ip, t1, menubutton, "-text menu")
    # menu ����������.
    me1 = TclTkWidget.new(ip, mb1, menu)
    # mb1 ���� me1 ����ư�����褦�ˤ���.
    mb1.e("configure -menu", me1)

    # cascade �ǵ�ư����� menu ����������.
    me11 = TclTkWidget.new(ip, me1, menu)
    # radiobutton �Υ���ץ�.
    v1 = TclTkVariable.new(ip, nil); global.e(v1); v1.set("r1")
    me11.e("add radiobutton -label radio1 -value r1 -variable", v1)
    me11.e("add radiobutton -label radio2 -value r2 -variable", v1)
    me11.e("add radiobutton -label radio3 -value r3 -variable", v1)
    # cascade �ˤ�� mb11 ����ư�����褦�ˤ���.
    me1.e("add cascade -label cascade -menu", me11)

    # checkbutton �Υ���ץ�.
    v2 = TclTkVariable.new(ip, nil); global.e(v2); v2.set("none")
    me1.e("add checkbutton -label check -variable", v2)
    # separator �Υ���ץ�.
    me1.e("add separator")
    # command �Υ���ץ�.
    v3 = nil
    cb.push(c = TclTkCallback.new(ip, proc{
      global.e(v1, v2, v3); print "v1: ", v1.get(), ", v2: ", v2.get(),
      ", v3: ", v3.get(), "\n"}))
    me1.e("add command -label print -command", c)

    ## tk_optionMenu

    v3 = TclTkVariable.new(ip, nil); global.e(v3); v3.set("opt2")
    om1 = TclTkWidget.new(ip, t1, tk_optionMenu, v3, "opt1 opt2 opt3 opt4")

    pack.e(b1, mb1, om1, "-side left")
  end

  # listbox �Υ���ץ�.
  def test_listbox(ip, parent)
    clipboard, frame, grid, listbox, lower, menu, menubutton, pack, scrollbar,
      selection = ip.commands().values_at(
      "clipboard", "frame", "grid", "listbox", "lower", "menu", "menubutton",
      "pack", "scrollbar", "selection")
    t1, b1, cb = inittoplevel(ip, parent, "listbox")

    ## listbox/scrollbar

    f1 = TclTkWidget.new(ip, t1, frame)
    # ������Хå���ǻ��Ȥ����ѿ������������Ƥ����ʤ���Фʤ�ʤ�.
    li1 = sc1 = sc2 = nil
    # �¹Ի���, ���˥ѥ�᡼�����Ĥ�������Хå���,
    # ���ƥ졼���ѿ��Ǥ��Υѥ�᡼���������뤳�Ȥ��Ǥ���.
    # (ʣ���Υѥ�᡼���ϤҤȤĤ�ʸ����ˤޤȤ����.)
    cb.push(c1 = TclTkCallback.new(ip, proc{|i| li1.e("xview", i)}))
    cb.push(c2 = TclTkCallback.new(ip, proc{|i| li1.e("yview", i)}))
    cb.push(c3 = TclTkCallback.new(ip, proc{|i| sc1.e("set", i)}))
    cb.push(c4 = TclTkCallback.new(ip, proc{|i| sc2.e("set", i)}))
    # listbox
    li1 = TclTkWidget.new(ip, f1, listbox,
      "-xscrollcommand", c3, "-yscrollcommand", c4,
      "-selectmode extended -exportselection true")
    for i in 1..20
      li1.e("insert end {line #{i} line #{i} line #{i} line #{i} line #{i}}")
    end
    # scrollbar
    sc1 = TclTkWidget.new(ip, f1, scrollbar, "-orient horizontal -command", c1)
    sc2 = TclTkWidget.new(ip, f1, scrollbar, "-orient vertical -command", c2)

    ## selection/clipboard

    mb1 = TclTkWidget.new(ip, t1, menubutton, "-text edit")
    me1 = TclTkWidget.new(ip, mb1, menu)
    mb1.e("configure -menu", me1)
    cb.push(c = TclTkCallback.new(ip, proc{
      # clipboard �򥯥ꥢ.
      clipboard.e("clear")
      # selection ����ʸ������ɤ߹��� clipboard ���ɲä���.
      clipboard.e("append {#{selection.e('get')}}")}))
    me1.e("add command -label {selection -> clipboard} -command",c)
    cb.push(c = TclTkCallback.new(ip, proc{
      # li1 �򥯥ꥢ.
      li1.e("delete 0 end")
      # clipboard ����ʸ�������Ф�, 1 �Ԥ���
      selection.e("get -selection CLIPBOARD").split(/\n/).each{|line|
        # li1 ����������.
        li1.e("insert end {#{line}}")}}))
    me1.e("add command -label {clipboard -> listbox} -command",c)

    grid.e(li1, "-row 0 -column 0 -sticky news")
    grid.e(sc1, "-row 1 -column 0 -sticky ew")
    grid.e(sc2, "-row 0 -column 1 -sticky ns")
    grid.e("rowconfigure", f1, "0 -weight 100")
    grid.e("columnconfigure", f1, "0 -weight 100")
    f2 = TclTkWidget.new(ip, t1, frame)
    lower.e(f2, b1)
    pack.e(b1, mb1, "-in", f2, "-side left")
    pack.e(f2, f1)
  end

  # canvas �Υ���ץ�.
  def test_canvas(ip, parent)
    canvas, lower, pack = ip.commands().values_at("canvas", "lower", "pack")
    t1, b1, cb = inittoplevel(ip, parent, "canvas")

    ## canvas

    ca1 = TclTkWidget.new(ip, t1, canvas, "-width 400 -height 300")
    lower.e(ca1, b1)
    # rectangle ����.
    idr = ca1.e("create rectangle 10 10 20 20")
    # oval ����.
    ca1.e("create oval 60 10 100 50")
    # polygon ����.
    ca1.e("create polygon 110 10 110 30 140 10")
    # line ����.
    ca1.e("create line 150 10 150 30 190 10")
    # arc ����.
    ca1.e("create arc 200 10 250 50 -start 0 -extent 90 -style pieslice")
    # i1 ��������, �ɤ������˲����ʤ���Фʤ�ʤ���, ���ݤʤΤ����äƤ���.
    i1 = TclTkImage.new(ip, "photo", "-file maru.gif")
    # image ����.
    ca1.e("create image 100 100 -image", i1)
    # bitmap ����.
    ca1.e("create bitmap 260 50 -bitmap questhead")
    # text ����.
    ca1.e("create text 320 50 -text {drag rectangle}")
    # window ����(�������ܥ���).
    ca1.e("create window 200 200 -window", b1)

    # bind �ˤ�� rectangle �� drag �Ǥ���褦�ˤ���.
    cb.push(c = TclTkCallback.new(ip, proc{|i|
      # i �� x �� y ��������Τ�, ���Ф�.
      x, y = i.split(/ /); x = x.to_f; y = y.to_f
      # ��ɸ���ѹ�����.
      ca1.e("coords current #{x - 5} #{y - 5} #{x + 5} #{y + 5}")},
      # x, y ��ɸ�����Ƕ��ڤä���Τ򥤥ƥ졼���ѿ����Ϥ��褦�˻���.
      "%x %y"))
    # rectangle �� bind ����.
    ca1.e("bind", idr, "<B1-Motion>", c)

    pack.e(ca1)
  end
end

# test driver

if ARGV.size == 0
  print "#{$0} n ��, n �ĤΥ��󥿥ץ꥿��ư���ޤ�.\n"
  n = 1
else
  n = ARGV[0].to_i
end

print "start\n"
ip = []

# ���󥿥ץ꥿, ���������å���������.
for i in 1 .. n
  ip.push(Test1.new())
end

# �Ѱդ��Ǥ����饤�٥�ȥ롼�פ�����.
TclTk.mainloop()
print "exit from mainloop\n"

# ���󥿥ץ꥿�� GC ����뤫�Υƥ���.
ip = []
print "GC.start\n" if $DEBUG
GC.start() if $DEBUG
print "end\n"

exit

# end

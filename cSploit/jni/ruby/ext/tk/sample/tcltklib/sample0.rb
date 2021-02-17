#! /usr/local/bin/ruby -vd

# tcltklib �饤�֥��Υƥ���

require "tcltklib"

def test
  # ���󥿥ץ꥿����������
  ip1 = TclTkIp.new()

  # ɾ�����Ƥߤ�
  print ip1._return_value().inspect, "\n"
  print ip1._eval("puts {abc}").inspect, "\n"

  # �ܥ�����äƤߤ�
  print ip1._return_value().inspect, "\n"
  print ip1._eval("button .lab -text exit -command \"destroy .\"").inspect,
    "\n"
  print ip1._return_value().inspect, "\n"
  print ip1._eval("pack .lab").inspect, "\n"
  print ip1._return_value().inspect, "\n"

  # ���󥿥ץ꥿���� ruby ���ޥ�ɤ�ɾ�����Ƥߤ�
#  print ip1._eval(%q/ruby {print "print by ruby\n"}/).inspect, "\n"
  print ip1._eval(%q+puts [ruby {print "print by ruby\n"; "puts by tcl/tk"}]+).inspect, "\n"
  print ip1._return_value().inspect, "\n"

  # �⤦��ĥ��󥿥ץ꥿���������Ƥߤ�
  ip2 = TclTkIp.new()
  ip2._eval("button .lab -text test -command \"puts test ; destroy .\"")
  ip2._eval("pack .lab")

  TclTkLib.mainloop
end

test
GC.start

print "exit\n"

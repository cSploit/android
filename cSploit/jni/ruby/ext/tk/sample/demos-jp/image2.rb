# -*- coding: euc-jp -*-
#
# widget demo 'load image' (called by 'widget')
#

# toplevel widget ��¸�ߤ���к������
if defined?($image2_demo) && $image2_demo
  $image2_demo.destroy
  $image2_demo = nil
end

# demo �Ѥ� toplevel widget ������
$image2_demo = TkToplevel.new {|w|
  title('Image Demonstration #2')
  iconname("Image2")
  positionWindow(w)
}

base_frame = TkFrame.new($image2_demo).pack(:fill=>:both, :expand=>true)

# label ����
msg = TkLabel.new(base_frame) {
  font $font
  wraplength '4i'
  justify 'left'
  text "���Υǥ�Ǥ�Tk�� photo image ����Ѥ��Ʋ����򸫤뤳�Ȥ��Ǥ��ޤ����ǽ�˥���ȥ���ˤ˥ǥ��쥯�ȥ�̾������Ʋ����������˲��Υꥹ�ȥܥå����ˤ��Υǥ��쥯�ȥ����ɤ��뤿�ᡢ�꥿����򲡤��Ƥ������������θ塢���������򤹤뤿��˥ꥹ�ȥܥå�������Υե�����̾����֥륯��å����Ʋ�������"
}
msg.pack('side'=>'top')

# frame ����
TkFrame.new(base_frame) {|frame|
  TkButton.new(frame) {
    #text 'λ��'
    text '�Ĥ���'
    command proc{
      tmppath = $image2_demo
      $image2_demo = nil
      tmppath.destroy
    }
  }.pack('side'=>'left', 'expand'=>'yes')

  TkButton.new(frame) {
    text '�����ɻ���'
    command proc{showCode 'image2'}
  }.pack('side'=>'left', 'expand'=>'yes')

}.pack('side'=>'bottom', 'fill'=>'x', 'pady'=>'2m')

# �ѿ�����
$dirName = TkVariable.new([$demo_dir,'..','images'].join(File::Separator))

# image ����
$image2a = TkPhotoImage.new

# �ե�����̾������
TkLabel.new(base_frame, 'text'=>'�ǥ��쥯�ȥ�:')\
.pack('side'=>'top', 'anchor'=>'w')

image2_e = TkEntry.new(base_frame) {
  width 30
  textvariable $dirName
}.pack('side'=>'top', 'anchor'=>'w')

TkFrame.new(base_frame, 'height'=>'3m', 'width'=>20)\
.pack('side'=>'top', 'anchor'=>'w')

TkLabel.new(base_frame, 'text'=>'�ե�����:')\
.pack('side'=>'top', 'anchor'=>'w')

TkFrame.new(base_frame){|w|
  s = TkScrollbar.new(w)
  l = TkListbox.new(w) {
    width 20
    height 10
    yscrollcommand proc{|first,last| s.set first,last}
  }
  s.command(proc{|*args| l.yview(*args)})
  l.pack('side'=>'left', 'expand'=>'yes', 'fill'=>'y')
  s.pack('side'=>'left', 'expand'=>'yes', 'fill'=>'y')
  #l.insert(0,'earth.gif', 'earthris.gif', 'mickey.gif', 'teapot.ppm')
  l.insert(0,'earth.gif', 'earthris.gif', 'teapot.ppm')
  l.bind('Double-1', proc{|x,y| loadImage $image2a,l,x,y}, '%x %y')

  image2_e.bind 'Return', proc{loadDir l}

}.pack('side'=>'top', 'anchor'=>'w')

# image ����
[ TkFrame.new(base_frame, 'height'=>'3m', 'width'=>20),
  TkLabel.new(base_frame, 'text'=>'����:'),
  # TkLabel.new(base_frame, 'image'=>$image2a)
  Tk::Label.new(base_frame, 'image'=>$image2a)
].each{|w| w.pack('side'=>'top', 'anchor'=>'w')}

# �᥽�å����
def loadDir(w)
  w.delete(0,'end')
  Dir.glob([$dirName,'*'].join(File::Separator)).sort.each{|f|
    w.insert('end',File.basename(f))
  }
end

def loadImage(img,w,x,y)
  img.file([$dirName, w.get("@#{x},#{y}")].join(File::Separator))
end


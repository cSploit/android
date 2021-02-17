# -*- coding: euc-jp -*-
#
# iconic button widget demo (called by 'widget')
#

# toplevel widget ��¸�ߤ���к������
if defined?($icon_demo) && $icon_demo
  $icon_demo.destroy
  $icon_demo = nil
end

# demo �Ѥ� toplevel widget ������
$icon_demo = TkToplevel.new {|w|
  title("Iconic Button Demonstration")
  iconname("icon")
  positionWindow(w)
}

base_frame = TkFrame.new($icon_demo).pack(:fill=>:both, :expand=>true)

# label ����
msg = TkLabel.new(base_frame) {
  font $font
  wraplength '5i'
  justify 'left'
  text "���Υ�����ɥ��ˤϥ饸���ܥ���ȥ����å��ܥ����˥ӥåȥޥåפ������ɽ������ 3 �Ĥ���ˡ�򼨤��Ƥ��ޤ������ˤ���Τ�2�ĤΥ饸���ܥ���ǡ����줾�줬���ӥåȥޥåפ�����򼨤����󥸥������ǤǤ��Ƥ��ޤ�������ˤ���Τϡ�����Ѥߤ��ɤ����ˤ�äưۤʤ������ɽ����������å��ܥ���Ǥ�����¦�ˤ���Τ�����Ѥߤ��ɤ����ˤ�ä��طʿ����Ѥ��ӥåȥޥåפ�ɽ����������å��ܥ���Ǥ���"
}
msg.pack('side'=>'top')

# frame ����
TkFrame.new(base_frame) {|frame|
  TkButton.new(frame) {
    #text 'λ��'
    text '�Ĥ���'
    command proc{
      tmppath = $icon_demo
      $icon_demo = nil
      tmppath.destroy
    }
  }.pack('side'=>'left', 'expand'=>'yes')

  TkButton.new(frame) {
    text '�����ɻ���'
    command proc{showCode 'icon'}
  }.pack('side'=>'left', 'expand'=>'yes')

}.pack('side'=>'bottom', 'fill'=>'x', 'pady'=>'2m')

# image ����
flagup = \
TkBitmapImage.new('file'=>[$demo_dir,'..',
                           'images','flagup.xbm'].join(File::Separator),
                  'maskfile'=>\
                  [$demo_dir,'..','images','flagup.xbm'].join(File::Separator))
flagdown = \
TkBitmapImage.new('file'=>[$demo_dir,'..',
                           'images','flagdown.xbm'].join(File::Separator),
                  'maskfile'=>\
                  [$demo_dir,'..',
                    'images','flagdown.xbm'].join(File::Separator))

# �ѿ�����
letters = TkVariable.new

# frame ����
TkFrame.new(base_frame, 'borderwidth'=>10){|w|
  TkFrame.new(w) {|f|
    # TkRadioButton.new(f){
    Tk::RadioButton.new(f){
      bitmap '@' + [$demo_dir,'..',
                     'images','letters.xbm'].join(File::Separator)
      variable letters
      value 'full'
    }.pack('side'=>'top', 'expand'=>'yes')

    # TkRadioButton.new(f){
    Tk::RadioButton.new(f){
      bitmap '@' + [$demo_dir,'..',
                     'images','noletter.xbm'].join(File::Separator)
      variable letters
      value 'empty'
    }.pack('side'=>'top', 'expand'=>'yes')

  }.pack('side'=>'left', 'expand'=>'yes', 'padx'=>'5m')

  # TkCheckButton.new(w) {
  Tk::CheckButton.new(w) {
    image flagdown
    selectimage flagup
    indicatoron 0
    selectcolor self['background']
  }.pack('side'=>'left', 'expand'=>'yes', 'padx'=>'5m')

  # TkCheckButton.new(w) {
  Tk::CheckButton.new(w) {
    bitmap '@' + [$demo_dir,'..',
                   'images','letters.xbm'].join(File::Separator)
    indicatoron 0
    selectcolor 'SeaGreen1'
  }.pack('side'=>'left', 'expand'=>'yes', 'padx'=>'5m')

}.pack('side'=>'top')


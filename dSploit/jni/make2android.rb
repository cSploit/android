#!/usr/bin/env ruby

# This file is part of the dSploit.
# 
# Copyleft of Dragano Massimo aka tux_mind <massimo.dragano@gmail.com>
# 
# dSploit is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# dSploit is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with dSploit.  If not, see <http://www.gnu.org/licenses/>.

# this script take a make output and convert it into the android
# makefiles standard.

class ObjectTarget
	attr_accessor :name,:objs,:srcs,:libs,:cflags,:cxxflags,:includes
	
	def initialize()
		@objs=[]
		@srcs=[]
		@libs=[]
		@cflags=[]
		@cxxflags=[]
		@includes=[]
	end
	
	def to_s
		@name
	end
	
	def ==(o)
		o.class == self.class and o.name == self.name
	end
end

class Library < ObjectTarget
	attr_accessor :unknown,:warned,:shared
	
	def initialize()
		super
		@unknown=[]
		@warned=false
		@shared=false
		@srcs=nil
	end
	
	def to_s
		libname
	end
	
	def libname
		@name.sub(/.*\//,'').sub(/\..*$/,'')
	end
end

class Executable < Library

    def exename
        libname
    end

end

##static variables...
def cflags_re
	/ (-[Df] ?[^ ]+)/
end
def include_re
	/ -I([^ ]+)/
end
def topwd_re
	/#{File.absolute_path(Dir.pwd).sub(/\/[^\/]+$/,'')+'/'}/
end
def libs_re
	/ -l([^\s]+)/
end
def libsearch_re
	/ -L([^\s]+)/
end
def abslib_re
	/\/[^\s]+\.(l?a|so(\.[0-9]+)*)/
end
def system_libs
	["m","z","c","dl","stc++","gcc","gnustl_static","stlport_static","gnustl_shared","stlport_shared"]
end
def system_includes
	/sources\/cxx-stl\//
end
def extra_cflags
    ["-static","-ffunction-sections", "-fdata-sections"]
end
def extra_ldflags
    [" -Wl,--gc-sections"]
end
def cd_re
	/^cd\s+(?<dir>[^; ]+)/
end
def pwd_re
	/#{Dir.pwd}\//
end
def cmake_cd_re
	/^cd\s+[^\s]+\s+&&\s+[^ ]+cmake\s/
end

# directory object
class Directory
    attr_accessor :absolute_path
    attr_reader :relative_path,:top_relative_path

    def initialize( abs )
        @absolute_path=abs
        @absolute_path+='/' if absolute_path[-2] != '/'
        @relative_path=absolute_path.sub(pwd_re,'')
        @top_relative_path=absolute_path.sub(topwd_re,'')
    end

    def ==(o)
        o.class == self.class && o.absolute_path == this.absolute_path
    end

    def to_s
        absolute_path
    end
end

def get_params(line,t)
	if line =~ /\.(cc|cpp)( |$)/ then
		dst = t.cxxflags
	else
		dst = t.cflags
	end
	line.scan(cflags_re).each do |flag|
		dst << flag[0] if !(dst.include? flag[0])
	end
	line.scan(include_re).each do |inc|
		next if inc[0] =~ system_includes
		dir=get_absolute_path(inc[0])
		while File.symlink? dir do
			dir=File.expand_path(File.readlink(dir),File.dirname(dir))
		end
		next if !(File.exists? dir)
		rel=dir.sub(topwd_re,'')
		t.includes << rel if !(t.includes.include? rel)
	end
	line.scan(libs_re).each do |lib|
		next if system_libs.include? lib[0]
		l=Library.new
		l.name="lib#{lib[0]}"
		t.libs << l
	end
end

def bash_globbing(t,inputs)
	globbed=inputs.find_all{|i| i =~ /\*.*\.[a-z]?[oa]/}
	# cannot use -=, must work on the same object
	inputs.delete_if{|i| globbed.include? i}
	globbed.each do |g|
		#transalte bash globbing into ruby regex and
		#add current make working dir to gobbing
		g=($dir_stack[-1].relative_path+g).gsub(".","\.").gsub("*",".*").gsub("/","\/")
		found = $list.find_all{|item| item.name =~ /#{g}/}
		if found.size == 0 then
			t.unknown << g
		else
			found.each {|item| inputs << File.absolute_path($dir_stack[0].absolute_path + item.name)}
		end
	end
end

def parse_inputs(t,inputs)
	inputs.each do |input|
	    input=get_absolute_path(input)
	    input=input.sub(pwd_re,'')
		case input
		when /\.(c|cpp|cc|s|S)$/
			abort("making library `#{t.name}' from source `#{input}'") if t.is_a?(Library)
			# i known that you CAN build an executable from sources,
			# but this script it's wrote for solving bug projects porting trouble,
			# not simple printf("hello world!\n");
			abort("making executable `#{t.name}' from source `#{input}'") if t.is_a?(Executable)
			t.srcs << input
		when /\.[la]?[oa]$/
			f=$list.find_all{|item| !(item.is_a?(Executable)) and item.name == input}
			if f.size > 1
				abort("#{t.name}: more targets for `#{input}' => #{f}")
			elsif f.size == 0
				abort("#{t.name}: cannot find `#{input}' in #{$list}")
			end
			i=f[0]
			if i.is_a?(Library)
				t.libs << i
			else
				t.objs << i
			end
		when /([^\/.]+)\.so(\.[0-9]+)*/
			fullname=$~[0]
			name=$1.sub(/^lib/,'')
			next if system_libs.include? name
			f=$list.find_all{|item| item.class != Executable and item.name == fullname}
			if f.size > 1
				abort("#{t.name}: more targets for `#{input}' => #{f}")
			elsif f.size == 0
				abort("#{t.name}: cannot find `#{input}' in #{$list}")
			end
			abort("#{t.name}: executable as input `#{f[0]}'") if f[0].is_a?(Executable)
			t.libs << f[0]
		else
			abort("unrecognized input: #{input}")
		end
	end
end

def get_absolute_path(file)
	if file[0] == '/' then
		File.absolute_path(file)
	else
		File.absolute_path($dir_stack[-1].absolute_path + file)
	end
end

# check if we are changing directory for the current command only
def check_cd(line)
    res=false
	if cd_re =~ line then
		$dir_stack.push Directory.new get_absolute_path($~[:dir])
		# cd /path && /usr/bin/cmake ... => we will remain in that directory
        if !(cmake_cd_re =~ line) then
            res=true
        end
	end
	res
end
	

$list=[]
$dir_stack=[]
dir_re=/(?<action>(Entering|Leaving)) directory `(?<path>[^']+)'/
lt_check=/(--mode=(?<mode>(link|compile))).*?-o (?<target>[^ ]+(\.([al]?o|l?a))?)/
lt_get_inputs=/^[^-].*\.(c|cpp|cc|S|s|[la]?o|l?a|so(\.[0-9]+)*)$/
# libtool compile some extra objects when making static libs
# end it change object/source order upon libtool version...
lt_extra_check=/libtool: compile:.*?-o (?<target>[^ ]+\.o)/
lt_extra_get_inputs=/(?<inputs>( +([^ ]+\.c))+)/
# generic compilation logs ( i use it for cmake )
generic_check=/(^| )(\/[^ ]+)?(?<prog>(gcc|ld|ar|g\+\+)) .*?[^ ]+\.(c|cpp|cc|S|s|[al]?o|l?a)( |$)/
generic_get_inputs=lt_get_inputs
generic_get_target=/-o (?<target>[^ ]+\.([al]?o|l?a))( |$)/
generic_check_executable=/-o [^ ]+($| )/

in_temporary_dir=false
n=0
text=File.open(ARGV[0]).read
text.gsub!(/\r\n?/, "\n")
text.each_line do |line|
  line.gsub!(/`.*?`/,'') # remove bash subshells
  line.sub!(/^\[ *[0-9]+%\]\s+/,'') # remove cmake percentage status
  n+=1
  if in_temporary_dir then
    $dir_stack.pop
    in_temporary_dir=false
  end
  if dir_re =~ line then
      case $~[:action]
      when "Entering"
        dir=Directory.new $~[:path]
        $dir_stack.push(dir)
      when "Leaving"
        $dir_stack.pop
      end
  elsif check_cd(line) then
      in_temporary_dir=true
  end
#  if false and lt_check =~ line then
#      re_result=$~
#	  name=$dir_stack[-1].relative_path+re_result[:target]
#	  next if $list.find_all{|item| item.name == name}.size > 0
#	  inputs=(line.split.grep lt_get_inputs).delete_if{|item| item == re_result[:target]}
#	  case name
#		when /\.[a-z]?o$/
#			t=ObjectTarget.new
#		when /\.[a-z]?a$/
#			t=Library.new
#		else
#			t=Executable.new
#		end
#	  t.name = name
#		#check for bash globbing
#		bash_globbing(t,inputs) if t.is_a?(Library) or t.is_a?(Executable)
#	  get_params(line,t)
#		parse_inputs(t,inputs)
#	  $list << t
#  elsif false and lt_extra_check =~ line then
#		lt_found=true
#	  t = ObjectTarget.new
#	  t.name = $dir_stack[-1].relative_path+($~[:target].sub(".o",".lo"))
#	  #remove .libs
#	  t.name.sub!('.libs/','')
#	  next if $list.find_all{|item| item.name == t.name}.size > 0
#	  (lt_extra_get_inputs.match line)[:inputs].split.each do |s|
#		  t.srcs << get_absolute_path(s).sub(pwd_re,'')
#	  end
#	  get_params(line, t)
#	  $list << t
#	els
	if generic_check =~ line then
		prog=$~[:prog]
		inputs=(line.split.grep generic_get_inputs)
		res=[]
		# probably we are building an object if we have sources in input
		if (res=(inputs.grep /\.(c|cpp|cc|S|s)$/).uniq).size > 0 then
			objs=(inputs.grep /\.[al]?o$/)
			abort("more sources in input: #{res}") if res.size > 1
			if objs.size == 0 then #guess object name ( source basename in CWD with .o extension)
				objs=[res[0].sub(/\.[a-zS]+$/,'.o').sub(/^.*\//,'')]
			elsif objs.size > 1 then
				abort("cannot find object target for `#{line}' ( found #{objs.size} )") if !(generic_get_target =~ line)
				objs=[$~[:target]]
			end
			t = ObjectTarget.new
			name = objs[0]
			t.name = get_absolute_path(name).sub(pwd_re,'')
			next if ($list.include? t)
			inputs.delete_if{|item| item == name}
			get_params(line,t)
			parse_inputs(t,inputs)
			$list << t
		# probably we are building a library if no sources in input 
		elsif (res=(inputs.grep /\.(l?a|so(\.[0-9]+)*)$/)).size > 0 then
			# try to find the target library
			if prog=="ar" then # if using ar, the target is the first one
				name=res[0]
				t=Library.new
			elsif line =~ /-o (?<target>[^ ]+\.l?a)/ then
				name=$~[:target]
				t=Library.new
			elsif line =~ /-o (?<target>[^ ]+\.so(\.[0-9]+)*)/ then
				name=$~[:target]
				t=Library.new
				t.shared=true
			elsif line =~ /-o (?<target>[^ ]+)/ then
				name=$~[:target]
				t=Executable.new
			else
				abort("cannot find library target for `#{line}'")
			end
			t.name = get_absolute_path(name).sub(pwd_re,'')
			next if ($list.include? t)
			#next if (t.is_a?(Library) and $list.select{|item| item.is_a?(Library) and item.libname == t.libname}.size > 0)
			inputs.delete_if{|item| item == name}
			#check for bash globbing
			bash_globbing(t,inputs)
			get_params(line,t)
			parse_inputs(t,inputs)
			$list << t
		else
			warn "unknown command: `#{line}'" if !(generic_check_executable =~ line)
		end
  end
end
$list.find_all{|item| item.is_a?(Library) or item.is_a?(Executable)}.each do |t|
	t.unknown.each do |u|
		found=$list.find_all{|item| !(item.is_a?(Executable)) and item.name =~ /#{u}/}
		abort("cannot find `#{u}' in `#{$list}'") if found.size == 0
		found.each do |f|
			if f.is_a?(Library)
				t.libs << f
			else
				t.objs << f
			end
		end
	end
	next if t.warned
	t.warned = true
	libs=$list.find_all{|item| t.class == item.class and t.libname == item.libname and !(item.warned)}
	warn "more libraries has the same name:\n#{t.libname} from `#{t.name}'" if libs.size > 0
	libs.each do |l|
		warn "#{l.libname} from `#{l.name}'"
		l.warned=true
	end
end

print "LOCAL_PATH := $(call my-dir)\n"

$list.find_all{|item| item.is_a?(Library) or item.is_a?(Executable)}.each do |lib|
	print "\n#original path: #{lib.name}\n"
	print "include $(CLEAR_VARS)\n"
	includes=[]
	cflags=[]
	cxxflags=[]
	srcs=[]
	libs=[]
	lib.objs.each do |o|
		o.includes.each do |inc|
			includes << inc if !(includes.include? inc)
		end
		o.cflags.each do |flag|
			cflags << flag if !(cflags.include? flag)
		end
		o.cxxflags.each do |flag|
			cxxflags << flag if !(cxxflags.include? flag)
		end
		o.srcs.each do |s|
			srcs << s if !(srcs.include? s)
		end
		o.libs.each do |l|
			libs << l if !(libs.include? l)
		end
	end
	#remove duplicate cflags ( C++ uses CFLAGS too )
	cxxflags.delete_if{|flag| cflags.include? flag}
	lib.libs.each do |l|
		libs << l if !(libs.include? l)
	end
	print "\nLOCAL_CFLAGS:= " if cflags.size > 0
	i=15
	cflags.each do |flag|
		if (i+flag.size) > 80 && i > 0 then
			print "\\\n"
			i=0
		end
		i+=flag.size+3
		print "#{flag} "
	end
    # fixed stuff ( you can change it from the head of this script )
	print "\n# fixed flags\nLOCAL_CFLAGS+= #{extra_cflags.join(" ")}" if extra_cflags.size > 0

	# the whole world call it CXXFLAGS, but google decided that in their makefiles
	# CXXFLAGS is CPPFLAGS ( that is usually the C/C++ preprocessor flags )
	# /me FACEPALM
	print "\n\nLOCAL_CPPFLAGS:= " if cxxflags.size > 0
	i=17
	cxxflags.each do |flag|
		if (i+flag.size) > 80 && i > 0 then
			print "\\\n"
			i=0
		end
		i+=flag.size+3
		print "#{flag} "
	end
	print "\n\nLOCAL_LDFLAGS:= #{extra_ldflags.join(" ")}" if lib.is_a?(Executable) and extra_ldflags.size > 0
	print "\n\n"
	print "LOCAL_C_INCLUDES:= " if includes.size > 0
	includes.each do |inc|
		print "\\\n\t#{inc}"
	end
	print "\nLOCAL_SRC_FILES:= " if srcs.size > 0
	srcs.each do |s|
		print "\\\n\t#{s}"
	end

	print "\nLOCAL_STATIC_LIBRARIES:= " if libs.size > 0
	libs.each do |l|
		print "\\\n\t#{l.libname}"
	end
	print "\nLOCAL_MODULE := #{lib.libname}\n\n"
	#TODO: refactoring
	if lib.is_a?(Executable)
	    print "include $(BUILD_EXECUTABLE)\n\n"
	elsif lib.shared
		print "include $(BUILD_SHARED_LIBRARY)\n\n"
	else
		print "include $(BUILD_STATIC_LIBRARY)\n\n"
	end
end

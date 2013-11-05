#!/bin/env ruby

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
		o.class == self.class && o.name == self.name
	end
end

class Library < ObjectTarget
	attr_accessor :unknown,:warned,:shared
	
	def initialize()
		super
		@unknown=[]
		@includes=[]
		@cflags=[]
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
def cd_re
	/^cd (?<dir>[^; ]+)/
end
def pwd_re
	/#{Dir.pwd}\//
end
def cmake_cd_re
	/^cd [^ ]+ +&& +[^ ]+cmake /
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
		if(inc[0][0] == '/')
			next if inc[0] =~ system_includes
			abs=File.absolute_path(inc[0])
		else
			abs=File.absolute_path($make_abs_dir + inc[0])
		end
		while File.symlink? abs do
			abs=File.expand_path(File.readlink(abs),File.dirname(abs))
		end
		next if !(File.exists? abs)
		rel=abs.sub(topwd_re,'')
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
	inputs= inputs - globbed
	globbed.each do |g|
		#transalte bash globbing into ruby regex and
		#add current make working dir to gobbing
		g=$make_dir+g.gsub(".","\.").gsub("*",".*").gsub("/","\/")
		found = $list.find_all{|item| item.name =~ /#{g}/}
		if found.size == 0 then
			t.unknown << g
		else
			found.each {|item| inputs << item.name}
		end
	end
end

def parse_inputs(t,inputs)
	inputs.each do |input|
		if input[0] == '/'
			abs=input
		else
			abs=File.absolute_path($make_abs_dir + input)
		end
		input=abs.sub(pwd_re,'')
		case input
		when /\.(c|cpp|cc|s|S)$/
			abort("making library `#{t.name}' from source `#{input}'") if t.is_a?(Library)
			t.srcs << input
		when /\.[la]?[oa]$/
			f=$list.find_all{|item| item.name == input}
			if f.size > 1
				abort("#{t.name}: more targets for `#{input}' => #{f}")
			elsif f.size == 0
				abort("#{t.name}: cannot find `#{input}' in #{$list}")
			end
			i=f[0]
			
			if i.instance_of?(Library)
				t.libs << i
			else
				t.objs << i
			end
		when /([^\/.]+)\.so(\.[0-9]+)*/
			fullname=$~[0]
			name=$1.sub(/^lib/,'')
			next if system_libs.include? name
			f=$list.find_all{|item| item.name == fullname}
			if f.size > 1
				abort("#{t.name}: more targets for `#{input}' => #{f}")
			elsif f.size == 0
				abort("#{t.name}: cannot find `#{input}' in #{$list}")
			end
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
		File.absolute_path($make_abs_dir + file)
	end
end

def check_cd(line)
	if cd_re =~ line then
		re_result = $~
		# cd /path && /usr/bin/cmake ... => we will remain in that directory
		$old_make_abs=$make_abs_dir if !(cmake_cd_re =~ line)
		$make_abs_dir=get_absolute_path(re_result[:dir])+'/'
		$make_dir=$make_abs_dir.sub(pwd_re,'')
	end
end

def restore_dir
	if $old_make_abs.size > 0 then
		$make_abs_dir=$old_make_abs
		$make_dir=$make_abs_dir.sub(pwd_re,'')
		$old_make_abs=""
	end
end
	

$make_dir=""
$make_abs_dir=""
$old_make_abs=""
$list=[]
dir_re=/Entering directory `([^']+)'/
lt_check=/(--mode=(?<mode>(link|compile))).*?(-o (?<target>[^ ]+\.([al]?o|l?a)))/
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

lt_found=false

text=File.open(ARGV[0]).read
text.gsub!(/\r\n?/, "\n")
text.each_line do |line|
	line.gsub!(/`.*?`/,'') # remove bash subshells
	line.sub!(/^\[ *[0-9]+%\]\s+/,'') # remove cmake percentage status
	restore_dir()
  if dir_re =~ line then
	  $make_abs_dir=$1+'/'
	  $make_dir=$make_abs_dir.sub(pwd_re,'')
		next
	else
		check_cd(line)
	end
  if lt_check =~ line then
		lt_found=true
	  isLibrary=false
		re_result=$~
	  name=$make_dir+re_result[:target]
	  next if $list.find_all{|item| item.name == name}.size > 0
	  inputs=(line.split.grep lt_get_inputs).delete_if{|item| item == re_result[:target]}
	  case name
		when /\.[a-z]?o$/
			t=ObjectTarget.new
		when /\.[a-z]?a$/
			t=Library.new
			isLibrary=true
		else
			abort("unrecognized target `#{name}'")
		end
	  t.name = name
		#check for bash globbing
		bash_globbing(t,inputs) if isLibrary
	  get_params(line,t)
		parse_inputs(t,inputs)
	  $list << t
  elsif lt_extra_check =~ line then
		lt_found=true
	  t = ObjectTarget.new
	  t.name = $make_dir+($~[:target].sub(".o",".lo"))
	  #remove .libs
	  t.name.sub!('.libs/','')
	  next if $list.find_all{|item| item.name == t.name}.size > 0
	  (lt_extra_get_inputs.match line)[:inputs].split.each do |s|
		  t.srcs << File.absolute_path($make_abs_dir + s).sub(pwd_re,'')
	  end
	  get_params(line, t)
	  $list << t
	elsif !lt_found && generic_check =~ line then
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
			#abort("make_abs_dir: #{$make_abs_dir}\nmake_dir: #{$make_dir}\nname: #{name}\nt.name: #{t.name}")
			inputs.delete_if{|item| item == name}
			get_params(line,t)
			parse_inputs(t,inputs)
			$list << t
		# probably we are building a library if no sources in input 
		elsif (res=(inputs.grep /\.(l?a|so(\.[0-9]+)*)$/)).size > 0 then
			# try to find the target library
			shared=false
			if prog=="ar" then # if using ar, the target is the first one
				name=res[0]
			elsif line =~ /-o (?<target>[^ ]+\.l?a)/ then
				name=$~[:target]
			elsif line =~ /-o (?<target>[^ ]+\.so(\.[0-9]+)*)/ then
				name=$~[:target]
				shared=true
			elsif line =~ /-o [^ ]+/ then # executable
				next
			else
				abort("cannot find library target for `#{line}'")
			end
			t = Library.new
			t.shared=shared
			t.name = get_absolute_path(name).sub(pwd_re,'')
			next if ($list.include? t)
			inputs=inputs.delete_if{|item| item == name}
			get_params(line,t)
			parse_inputs(t,inputs)
			$list << t
		else
			warn "unknown command: `#{line}'" if !(generic_check_executable =~ line)
		end
  end
end
$list.find_all{|item| item.instance_of?(Library)}.each do |t|
	t.unknown.each do |u|
		found=$list.find_all{|item| item.name =~ /#{u}/}
		abort("cannot find `#{u}' in `#{$list}'") if found.size == 0
		found.each do |f|
			if f.instance_of?(Library)
				t.libs << f
			else
				t.objs << f
			end
		end
	end
	next if t.warned
	libs=$list.find_all{|item| item != t && item.instance_of?(Library) && !(item.warned) && item.libname == t.libname}
	warn "more libraries has the same name:" if libs.size > 0
	warn "#{t.libname} from `#{t.name}'" if libs.size > 0
	libs.each do |l|
		warn "#{l.libname} from `#{l.name}'"
		l.warned=true
	end
	t.warned = true
end

print "LOCAL_PATH := $(call my-dir)\n"

$list.find_all{|item| item.instance_of?(Library)}.each do |lib|
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
	# the whole world call it CXXFLAGS, but google decided that in their makefiles
	# CXXFLAGS is CPPFLAGS ( that is usually the C/C++ preprocessor flags )
	# /me FACEPALM
	print "\nLOCAL_CPPFLAGS:= " if cxxflags.size > 0
	i=17
	cxxflags.each do |flag|
		if (i+flag.size) > 80 && i > 0 then
			print "\\\n"
			i=0
		end
		i+=flag.size+3
		print "#{flag} "
	end
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
	if lib.shared
		print "include $(BUILD_SHARED_LIBRARY)\n\n"
	else
		print "include $(BUILD_STATIC_LIBRARY)\n\n"
	end
end

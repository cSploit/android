#!/usr/bin/env ruby

#find all files in a directory ( recursively )
def find_files(dir)
	list=[]
	dir=dir[0..-2] if dir[-1] == '/'
	Dir.entries(dir).each do |ent|
		next if ent == '.' or ent == '..'
		path="#{dir}/#{ent}"
		next if File.symlink? path
		if Dir.exists? path then
			list.concat(find_files(path))
		else
			list << path
		end
	end
	list
end

#find all empty directories in a directory ( recursively )
def find_empty_dirs(dir)
	list=[]
	dir=dir[0..-2] if dir[-1] == '/'
	Dir.entries(dir).each do |ent|
		next if ent == '.' or ent == '..'
		path="#{dir}/#{ent}"
		next if File.symlink? path
		if Dir.exists? path then
			if (Dir.entries(path) - %w[ . .. ]).size == 0 then
				list << path
			else
				list.concat(find_empty_dirs(path))
			end
		end
	end
	list
end

#find all symlinks in a directory ( recursively )
def find_symlinks(dir)
	list=[]
	dir=dir[0..-2] if dir[-1] == '/'
	Dir.entries(dir).each do |ent|
		next if ent == '.' or ent == '..'
		path="#{dir}/#{ent}"
		if File.symlink? path then
			list << path
		elsif Dir.exists? path then
			list.concat(find_symlinks(path))
		end
	end
	list
end

def find_header_parse_one(line,file)
	case line
	when /#\s*include\s+"([^"]+)"/
		name= File.expand_path($1,File.dirname(file)+'/')
		if File.exists? name then
			while File.symlink? name do
				$headers_symlinks << name
				name=File.expand_path(File.readlink(name),File.dirname(name))
				abort("broken link: #{$headers_symlinks[-1]} -> #{name}") if !(File.exists? name)
			end
			# warn "#{file.sub($topdir+$subdir,'')} => #{name.sub($topdir+$subdir,'')}"
			if !($find_list.include? name) and name != file then
				$find_list << name
				find_headers(name)
			end
		else
			$not_relative_headers << $1 if !($not_relative_headers.include? $1)
		end
	when /#\s*include\s+<([^>]+)>/
		$not_relative_headers << $1 if !($not_relative_headers.include? $1)
	when /#\s*include\s+([^\s"<]+)\s*$/
		# check if are including a define
		# it's not so clean, but rarely we have it
		missing=$1
		res=`grep -rhIP "#\s*define\s+#{missing}\s+[^\s]+" "#{$topdir + $subdir}"`
		res.scan(/define\s+#{missing}\s+([^\s]+)/).map{|i| i[0]}.each do |value|
			find_header_parse_one(line.sub(missing,value),file)
		end
	end
end

#find headers used by file ( recursively )
def find_headers(file)
	in_comments = false
	open(file).read.encode!('UTF-16', 'UTF-8', :invalid => :replace , :universal_newline => true).encode!('UTF-8', 'UTF-16').each_line do |line|
		# remove newline
		line.sub!(/\n$/,'')
		# remove inline comments
		line.gsub!(/\/\*.*?\*\//,'')
		# detect if we are in a comment block
		if !in_comments then
			in_comments = line.include? "/*"
			if in_comments then
				line.sub!(/\/\*.*$/,'')
			end
		else
			in_comments = !(line.include? "*/")
			if !in_comments then
				line.sub!(/^.*?\*\//,'')
			end
		end
		next if in_comments
		line.sub!(/\/\/.*$/,'') #TODO: check that "//" is out of any string
		find_header_parse_one(line,file)
	end
end

## MAIN ##

keep_files=["COPYING","Android.mk","LICENSE","android-config.mk"]
topdir=File.dirname(File.absolute_path(__FILE__)) + '/'
subdir=File.absolute_path(ARGV[0]).sub(topdir,'')
$topdir=topdir
$subdir=subdir
subdir+='/' if subdir[-1] != '/'
srcs=[]
inc=[]
target=nil
lastline=false
text=File.open(topdir + subdir + "/Android.mk").read
text.gsub!(/\r\n?/, "\n")
text.each_line do |line|
	lastline=false
	case line
	when /^#/
		next
	when /LOCAL_SRC_FILES\s*[:+]?=/
		target=srcs
	when /LOCAL_C_INCLUDES\s*[:+]?=/
		target=inc
	end
	if !(line =~ /\\$/) then
		lastline=true if target != nil
	end
	next if target == nil
	line.sub(/^[^=]+=/,'').sub(/\\$/,'').split.grep(/[^\s]+/).each{|i| target << i if !(target.include? i)}
	target = nil if lastline
end

# android build system add project dir to the included ones
inc << subdir if !(inc.include? subdir)

# fix paths
srcs.map! do |s|
	File.absolute_path(s.sub(/^/,topdir + subdir + '/'))
end
inc.map! do |i|
	File.absolute_path(i.sub(/^/,topdir))
end

# check file existance
srcs.each do |s|
	abort("cannot find `#{s}'") if !(File.exists? s)
end
inc.each do |i|
	abort("cannot find `#{i}'") if !(Dir.exists? i)
end


# find all headers in include dirs
headers=[]
inc.each do |dir|
	next if !(dir =~ /^#{topdir}/)
	find_files(dir).grep(/\.(h|hpp)$/).each do |h|
		h.gsub!(/\/+/,'/')
		headers << h if !(headers.include? h)
	end
end

$not_relative_headers=[]
$find_list=[]
$headers_symlinks=[]
used_headers=[]

# find headers included by sources
srcs.each{|s| find_headers(s)}
used_headers.concat($find_list).uniq!
begin
	tmp=[]
	inc.each do |incdir|
		incdir+='/' if incdir[-1] != '/'
		$not_relative_headers.each do |h|
			next if !(File.exists? (incdir + h))
			tmp << File.absolute_path(incdir + h)
		end
	end
	tmp.uniq!
	$not_relative_headers=[]
	$find_list=[]
	tmp.each{|h| find_headers(h)}
	used_headers.concat($find_list).concat(tmp).uniq!
	#remove the one that we have already found ( avoid circular dependencies )
	used_headers.each do |f|
		$not_relative_headers.delete_if{|h| /\/#{h}$/ =~ f}
	end
end while $not_relative_headers.size > 0

# we are *sure* that $used_headers are used ( many depends on CPP macros...)
headers=used_headers
probably_unused_files=[]

find_files(topdir + subdir)\
		.select{|f| 
            !(\
							(keep_files.include? File.basename(f)) or \
							srcs.include? f or\
							headers.include? f\
             )}\
.each do |file|
	if file =~ /\.(c|cc|S|s|cpp|h|hpp)$/
		probably_unused_files << file
	else
		File.delete file
	end
end

if probably_unused_files.size > 0 then
	print "probably unused but kept files:\n"
	probably_unused_files.each{|f| print "#{f.sub(topdir + subdir,'')}\n"}
	print "\ndo you want to delete them? [y/N] "
	c=STDIN.gets.split[0][0]
	probably_unused_files.each{|f| File.delete f} if (c == 'Y' or c == 'y')
end

#remove empty dirs
begin
	empty=find_empty_dirs(topdir + subdir)
	empty.each{|d| Dir.rmdir d}
end while empty.size > 0

#remove symlinks only after we clean it all
find_symlinks(topdir + subdir).each do |l|
	next if $headers_symlinks.include? l
	File.delete l
end

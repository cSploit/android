How that works?
--------

This is a quick howto for use our scripts to port linux software into your
android ndk project.

you should have your project directory under the jni one, 
no matters how deep, it must be a subdirectory of the jni folder.

for example:
```sh
$ cd /path/to/jni
$ wget -O - "http://host.com/source-tarball.tar.gz" | tar -xzf -
```
will extract the tarball into the jni directory.

> **NOTE:** most open source projects will create their own dir on extract, check that everything will extract to a single directory

we have 3 scripts:

 - [ndk-configure]: run configure scripts using the NDK stuff
 - [make2android]: convert make output into an Android.mk
 - [cleanup]: delete everything that is not essential
 

Generating Makefiles
--------------------

### Configure

Most of the Open Source projects use auto-tools for get the envinroment
where they will be built for.

I created the ndk-configure script which will start the real configure
script with options and environment variables to make it uses the
android ndk toolchain.

```sh
Usage: ndk-configure <ndk_path> <api_level> [configure_script] [stl_provider] [update_files]
```
where:

 - **ndk_path** is the install directory of your ndk ( usually /opt/android-ndk )
 - **api_level** is the target api level of your application
 - **configure_script** is the real configure script [default=./configure]
 - **stl_provider** is the c++ stl provider [default=system]
 - if **update_files** option is given will update machine detecting scripts

for more info run it without arguments.

#### Configure hacking

sometimes configure script will try to run compiled code, which is not possible while cross-compiling.

so, **if** you get something like that:
```
checking for something ... ERROR: cannot check for something while cross compiling
```
_you have to edit the configure script._

the main purpose of this hack is to run compiled stuff on your android phone though ADB.

copy and paste the part containing all functions from  [configure_hacks] to the top of your project configure script.

edit the `ac_c_fn_try` function as described in [configure_hacks] comments.

now you have to find out where is the first project-specific check.
run the configure script using ndk-configure and search for the first check of the long list.

copy and paste the last 2 lines of [configure_hacks] just before that check start ( before the `{ echo "checking for something ..."` ).

> **NOTE:** ensure to put them in an always reachable position

run again the ndk-configure, you should now have all Makefiles generated correctly

### CMake
for got CMake works with the android toolchain you have to use the [android.cmake] toolchain script.
for more info visit the [android cmake website]

________

Compiling
---------

### Make
it's now time to compile! :smile

first of all you should check that everything works by running `make`.


if you have some trouble here take a look to [bionic_workarounds] file, which have all my workarounds to solve bionic libc troubles.

### Cross compilation hacks
***maybe*** the project you're porting will run compiled code for some purpose.
this will fail since you cannot run cross-compiled code on your host.
you should see something like:

```
Make[2]: Entering directory 'dir'
bin/foo arg1 arg2 arg3 ...
Make[2]: Failed to run bin/foo [126]
Cannot execute binary file
Make[2]: Leaving directory 'dir'
Make[1]: Leaving directory 'dir'
```

the solution it's quite simple, we have to build these binaries for our host.

```sh
# on a new shell
# go into a temporary location
cd /tmp
# extract the project tarball ( ensure to have enoght space )
tar -xzf /path/to/project-tarball.tar.gz
# chdir into the project
cd project
# configure the project for your host
# NOTE: use cmake instead of configure if this project does
./configure
# compile for your host
make
```

replace the "missing" binary with the one you built for your host.

```sh
# from the porting project directory
cp /tmp/project/bin/foo bin/foo
```

you can now resume the make process by run ` make ` again.
______

Generate the Android.mk
-----------------------

### Capture make output
we have to create a file containing everything make does.
to do this run:
```sh
make clean
make -w > /tmp/project_make.log
```
if you did the [cross-compilation hack](#cross-compilation-hacks) described before your build will fail as it try to run the compiled executable. in this case you have to copy the "missing" executable as you did before ( you have already compiled it, so just copy it again ). once you copied it, run the previous command with an extra `>`, thus to preserve previous output.

### Convert make output to Android.mk {#convert-make-android}
to use [make2android] correctly you have to put it in the `jni` folder, above all other projects.
and then:
```sh
../make2adroid.rb /tmp/project_make.log > Android.mk
```
from your project directory

***DONE!***

Cleaning up
-----------
### Create a .gitignore
before you push everything over your git repo it's very important that you not push any blob file. they are useless and require a lot of space.

[.gitignore] it's exactly what we need!

ensure to not upload compiled objects and libraries:
```
*.[oa]
*.l[oa]
*.ao
*.so
```

### Remove everything else

if you wish to use as less space as possible you can use the [cleanup] script, which will track every used source and header and remove everything else.

> **NOTE:** use [cleanup] **only after porting everything you need**.
I strongly suggest you to make a whole backup of your jni folder by running ` tar -czf /home/user/jni_backup.tar.gz .` from the jni folder.

after you did a backup ( please do it! ) you can start clearing all unused files.
go to the project directory and run the [cleanup] script.
```sh
cd project
../cleanup.rb .
```


  [bionic_workarounds]: bionic_workarounds.md
  [configure_hacks]: configure_hacks
  [ndk-configure]: ndk-configure
  [make2android]: make2android.rb
  [.gitignore]: https://help.github.com/articles/ignoring-files
  [cleanup]: cleanup.rb

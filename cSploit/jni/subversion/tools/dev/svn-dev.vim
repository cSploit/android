" This file sets vim up to use subversion's coding style.  It can be applied on
" a per buffer basis with :source svn-dev.vim, or can be source from ~/.vimrc to
" apply settings to all files vim uses.  For other variation try :help autocmd.
"
" Licensed to the Apache Software Foundation (ASF) under one
" or more contributor license agreements.  See the NOTICE file
" distributed with this work for additional information
" regarding copyright ownership.  The ASF licenses this file
" to you under the Apache License, Version 2.0 (the
" "License"); you may not use this file except in compliance
" with the License.  You may obtain a copy of the License at
"
"   http://www.apache.org/licenses/LICENSE-2.0
"
" Unless required by applicable law or agreed to in writing,
" software distributed under the License is distributed on an
" "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
" KIND, either express or implied.  See the License for the
" specific language governing permissions and limitations
" under the License.
"
" TODO: Try to find a way to wrap comments without putting a * on the next line,
" since most of subversion doesn't use that style.  (Note that taking cro out of
" formatoptions won't quite work, because then comments won't be wrapped by
" default).
"
" Expand tab characters to spaces
set expandtab

" Tab key moves 8 spaces
set tabstop=8 

" '>>' moves 4 spaces
set shiftwidth=4

" Wrap lines at 78 columns.
"   78 so that vim won't swap over to the right before it wraps a line.
set textwidth=78

" What counts as part of a word (used for tag matching, and motion commands)
set iskeyword=a-z,A-Z,48-57,_,.,-,>

" How to wrap lines
"   t=wrap lines, c=wrap comments, inserting comment leader, r=insert comment
"   leader after an <ENTER>, o=Insert comment leader after an 'o', q=Allow
"   formatting of comments with 'gq'
set formatoptions=tcroq

" Use C style indenting
set cindent

" Use the following rules to do C style indenting
"   (Note that an s mean number*shiftwidth)
"   >=normal indent,
"   e=indent inside braces(brace at end of line),
"   n=Added to normal indent if no braces,
"   f=opening brace of function,
"   {=opening braces,
"   }=close braces (from opening),
"   ^s=indent after brace, if brace is on column 0,
"   := case labels from switch, ==statements after case,
"   t=function return type,
"   +=continuation line,
"   c=comment lines from opener,
"   (=unclosed parens (0 means match),
"   u=same as ( but for second set of parens
"   
"   Try :help cinoptions-values
set cinoptions=>1s,e0,n-2,f0,{.5s,}0,^-.5s,=.5s,t0,+1s,c3,(0,u0,\:2

" The following modelines can also be used to set the same options.
"/*
" * vim:ts=8:sw=4:expandtab:tw=78:fo=tcroq cindent
" * vim:isk=a-z,A-Z,48-57,_,.,-,>
" * vim:cino=>1s,e0,n-2,f0,{.5s,}0,^-.5s,=.5s,t0,+1s,c3,(0,u0,\:2
" */

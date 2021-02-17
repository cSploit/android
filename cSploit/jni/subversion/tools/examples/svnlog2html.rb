#!/usr/bin/env ruby

#
######################################################################
#    Licensed to the Apache Software Foundation (ASF) under one
#    or more contributor license agreements.  See the NOTICE file
#    distributed with this work for additional information
#    regarding copyright ownership.  The ASF licenses this file
#    to you under the Apache License, Version 2.0 (the
#    "License"); you may not use this file except in compliance
#    with the License.  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing,
#    software distributed under the License is distributed on an
#    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#    KIND, either express or implied.  See the License for the
#    specific language governing permissions and limitations
#    under the License.
######################################################################
#

require "erb"
require "svn/client"

include ERB::Util

path = File.expand_path(ARGV.shift || Dir.pwd)

html = <<-HEADER
<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE html
    PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
    "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
  <style type="text/css">
div.entry
{
  border: 1px solid red;
  border-width: 1px 0 0 1px;
  margin: 2em 2em 2em 3em;
  padding: 0 2em;
}

pre.message
{
  border-left: 1px solid red;
  margin: 1em 2em;
  padding-left: 1em;
}

div.info
{
  text-align: right;
}

span.info
{
  border-bottom: 1px solid red;
  padding: 0 5px 1px 1em;
}

span.author
{
  font-style: italic;
}

span.date
{
  color: #999;
}

li.action-A
{
  color: blue;
}

li.action-M
{
  color: green;
}

li.action-D
{
  color: red;
  text-decoration: line-through;
}
  </style>
  <title>#{h path}</title>
</head>
<body>
<h1>#{h path}</h1>
HEADER

ctx = Svn::Client::Context.new
ctx.log(path, "HEAD", 0, 40, true, true) do
  |changed_paths, rev, author, date, message|

  html << <<-ENTRY_HEADER

<div class="entry">
  <h2>r#{h rev}</h2>
  <pre class="message">#{h message}</pre>
  <div class="info">
    <span class="info">
      by <span class="author">#{h author}</span>
      at <span class="date">#{date}</span>
    </span>
  </div>
  <div class="changed-path">
ENTRY_HEADER

  changed_paths.sort.each do |path, changed_path|
    action = changed_path.action
    html << <<-ENTRY_PATH
    <ul>
      <li class="action-#{h action}">
        <span class="action">#{h action}</span>:
        <span class="changed-path">#{h path}</span>
      </li>
    </ul>
ENTRY_PATH
  end

  html << <<-ENTRY_FOOTER
  </div>
</div>

ENTRY_FOOTER
end

html << <<-FOOTER
</body>
</html>
FOOTER

puts html

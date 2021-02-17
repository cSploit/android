description = [[
Crawls webservers in search of RFI (remote file inclusion) vulnerabilities. It tests every form field it finds and every parameter of a URL containing a query.
]]

---
-- @usage
-- nmap --script http-rfi-spider -p80 <host>
--
--
-- @output
-- PORT   STATE SERVICE REASON
-- 80/tcp open  http    syn-ack
-- | http-rfi-spider:
-- |   Possible RFI in form at path: /pio/rfi_test2.php, action: /rfi_test2.php for fields:
-- |     color
-- |_    inc
--
-- @args http-rfi-spider.inclusionurl the url we will try to include, defaults
--       to <code>http://www.yahoo.com/search?p=rfi</code>
-- @args http-rfi-spider.pattern the pattern to search for in <code>response.body</code>
--       to determine if the inclusion was successful, defaults to
--       <code>'<a href="http://search.yahoo.com/info/submit.html">Submit Your Site</a>'</code>
-- @args http-rfi-spider.maxdepth the maximum amount of directories beneath
--       the initial url to spider. A negative value disables the limit.
--       (default: 3)
-- @args http-rfi-spider.maxpagecount the maximum amount of pages to visit.
--       A negative value disables the limit (default: 20)
-- @args http-rfi-spider.url the url to start spidering. This is a URL
--       relative to the scanned host eg. /default.html (default: /)
-- @args http-rfi-spider.withinhost only spider URLs within the same host.
--       (default: true)
-- @args http-rfi-spider.withindomain only spider URLs within the same
--       domain. This widens the scope from <code>withinhost</code> and can
--       not be used in combination. (default: false)
--

author = "Piotr Olma"
license = "Same as Nmap--See http://nmap.org/book/man-legal.html"
categories = {"intrusive"}

local shortport = require 'shortport'
local http = require 'http'
local stdnse = require 'stdnse'
local url = require 'url'
local httpspider = require 'httpspider'
local string = require 'string'
local table = require 'table'

-- this is a variable that will hold the function that checks if a pattern we are searching for is in
-- response's body
local check_response

-- this variable will hold the injection url
local inclusion_url

-- checks if a field is of type we want to check for rfi
local function rfi_field(field_type)
  return field_type=="text" or field_type=="radio" or field_type=="checkbox" or field_type=="textarea"
end

-- generates postdata with value of "sampleString" for every field (that satisfies rfi_field()) of a form
local function generate_safe_postdata(form)
  local postdata = {}
  for _,field in ipairs(form["fields"]) do
    if rfi_field(field["type"]) then
      postdata[field["name"]] = "sampleString"
    end
  end
  return postdata
end

local function generate_get_string(data)
  local get_str = {"?"}
  for name,value in pairs(data) do
    get_str[#get_str+1]=url.escape(name).."="..url.escape(value).."&"
  end
  return table.concat(get_str)
end

-- checks each field of a form to see if it's vulnerable to rfi
local function check_form(form, host, port, path)
  local vulnerable_fields = {}
  local postdata = generate_safe_postdata(form)
  local sending_function, response

  local action_absolute = string.find(form["action"], "^https?://")
  -- determine the path where the form needs to be submitted
  local form_submission_path
  if action_absolute then
    form_submission_path = form["action"]
  else
    local path_cropped = string.match(path, "(.*/).*")
    path_cropped = path_cropped and path_cropped or ""
    form_submission_path = path_cropped..form["action"]
  end
  if form["method"]=="post" then
    sending_function = function(data) return http.post(host, port, form_submission_path, nil, nil, data) end
  else
    sending_function = function(data) return http.get(host, port, form_submission_path..generate_get_string(data), nil) end
  end

  for _,field in ipairs(form["fields"]) do
    if rfi_field(field["type"]) then
      stdnse.print_debug(2, "http-rfi-spider: checking field %s", field["name"])
      postdata[field["name"]] = inclusion_url
      response = sending_function(postdata)
      if response and response.body and response.status==200 then
        if check_response(response.body) then
          vulnerable_fields[#vulnerable_fields+1] = field["name"]
        end
      end
      postdata[field["name"]] = "sampleString"
    end
  end
  return vulnerable_fields
end

-- builds urls with a query that would let us decide if a parameter is rfi vulnerable
local function build_urls(injectable)
  local new_urls = {}
  for _,u in ipairs(injectable) do
    if type(u) == "string" then
      local parsed_url = url.parse(u)
      local old_query = url.parse_query(parsed_url.query)
      for f,v in pairs(old_query) do
        old_query[f] = inclusion_url
        parsed_url.query = url.build_query(old_query)
        table.insert(new_urls, url.build(parsed_url))
        old_query[f] = v
      end
    end
  end
  return new_urls
end

-- as in sql-injection.nse
local function inject(host, port, injectable)
  local all = nil
  for k, v in pairs(injectable) do
    all = http.pipeline_add(v, nil, all, 'GET')
  end
  return http.pipeline_go(host, port, all)
end

local function check_responses(urls, responses)
  if responses == nil or #responses==0 then
    return {}
  end
  local suspects = {}
  for i,r in ipairs(responses) do
    if r.body then
      if check_response(r.body) then
        local parsed = url.parse(urls[i])
        if suspects[parsed.path] then
          table.insert(suspects[parsed.path], parsed.query)
        else
          suspects[parsed.path] = {}
          table.insert(suspects[parsed.path], parsed.query)
        end
      end
    end
  end
  return suspects
end

portrule = shortport.port_or_service( {80, 443}, {"http", "https"}, "tcp", "open")

function action(host, port)
  inclusion_url = stdnse.get_script_args('http-rfi-spider.inclusionurl') or 'http://www.yahoo.com/search?p=rfi'
  local pattern_to_search = stdnse.get_script_args('http-rfi-spider.pattern') or '<a href="http://search%.yahoo%.com/info/submit%.html">Submit Your Site</a>'

  -- once we know the pattern we'll be searching for, we can set up the function
  check_response = function(body) return string.find(body, pattern_to_search) end

  -- create a new crawler instance
  local crawler = httpspider.Crawler:new(  host, port, nil, { scriptname = SCRIPT_NAME} )

  if ( not(crawler) ) then
    return
  end

  local return_table = {}

  while(true) do
    local status, r = crawler:crawl()

    if ( not(status) ) then
      if ( r.err ) then
        return stdnse.format_output(true, ("ERROR: %s"):format(r.reason))
      else
        break
      end
    end

    -- first we try rfi on forms
    if r.response and r.response.body and r.response.status==200 then
      local all_forms = http.grab_forms(r.response.body)
      for _,form_plain in ipairs(all_forms) do
        local form = http.parse_form(form_plain)
        local path = r.url.path
        if form then
          local vulnerable_fields = check_form(form, host, port, path)
          if #vulnerable_fields > 0 then
            vulnerable_fields["name"] = "Possible RFI in form at path: "..path..", action: "..form["action"].." for fields:"
            table.insert(return_table, vulnerable_fields)
          end
        end
      end --for
    end --if

    -- now try inclusion by parameters
    local injectable = {}
    -- search for injectable links (as in sql-injection.nse)
    if r.response.status and r.response.body then
      local links = httpspider.LinkExtractor:new(r.url, r.response.body, crawler.options):getLinks()
      for _,u in ipairs(links) do
        local url_parsed = url.parse(u)
        if url_parsed.query then
          table.insert(injectable, u)
        end
      end
    end
    if #injectable > 0 then
      local new_urls = build_urls(injectable)
      local responses = inject(host, port, new_urls)
      local suspects = check_responses(new_urls, responses)
      if #suspects > 0 then
        for p,q in pairs(suspects) do
          local vulnerable_fields = q
          vulnerable_fields["name"] = "Possible RFI in parameters at path: "..p.." for queries:"
          table.insert(return_table, vulnerable_fields)
        end
      end
    end
  end
  return stdnse.format_output(true, return_table)
end


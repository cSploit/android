/*
 * This file is part of the dSploit.
 *
 * Copyleft of Simone Margaritelli aka evilsocket <evilsocket@gmail.com>
 *             Massimo Dragano aka tux_mind <massimo.dragano@gmail.com>
 *
 * dSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * dSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dSploit.  If not, see <http://www.gnu.org/licenses/>.
 */
package it.evilsocket.dsploit.plugins;

import it.evilsocket.dsploit.net.Target.Vulnerability;
import it.evilsocket.dsploit.core.Logger;
import it.evilsocket.dsploit.core.System;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;
import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.util.Log;

public class OSVDatabase
{
  private final static Pattern ID_PATTERN       = Pattern.compile( "<a href=\"/show/osvdb/([0-9]+)",		Pattern.MULTILINE | Pattern.DOTALL );
  private final static Pattern SUMMARY_PATTERN  	  = Pattern.compile( "<title>[0-9]+: ([^<]+)",	Pattern.MULTILINE | Pattern.DOTALL );
  private final static Pattern SEVERITY_PATTERN = Pattern.compile( "CVSSv2 Base Score = ([0-9]{1,2}\\.[0-9])",	Pattern.MULTILINE | Pattern.DOTALL );
  private final static Pattern MSF_PATTERN	  = Pattern.compile("href=\"http://metasploit.org/modules/framework/search?", Pattern.MULTILINE | Pattern.DOTALL);
  private final static Pattern PAGES_CHECK	  = Pattern.compile("<div class=\"pagination\">", Pattern.MULTILINE | Pattern.DOTALL);
  private final static Pattern PAGES_PATTERN	  = Pattern.compile("<div class=\"pagination\">((?!</div>).)+", Pattern.MULTILINE | Pattern.DOTALL);
  private final static Pattern PAGE_NUMS		  = Pattern.compile("page=([0-9]+)", Pattern.MULTILINE | Pattern.DOTALL);
  private final static String  APPEND_REQUEST   = "&search[text_type]=alltext&search[s_date]=&search[e_date]=&search[refid]=&search[referencetypes]=&search[vendors]=&search[cvss_score_from]=&search[cvss_score_to]=&search[cvss_av]=*&search[cvss_ac]=*&search[cvss_a]=*&search[cvss_ci]=*&search[cvss_ii]=*&search[cvss_ai]=*&kthx=search";

  private static ArrayList<Integer> parse_response(String body)
  {
    ArrayList<Integer> identifiers = new ArrayList<Integer>();
    Matcher matcher = null;
    if((matcher = ID_PATTERN.matcher(body)) != null) {
      while( matcher.find()) {
        identifiers.add(Integer.parseInt(matcher.group(1)));
      }
    }
    return identifiers;
  }

  private static Vulnerability parse_vuln(String body, Integer id)
  {
    Matcher matcher;
    Vulnerability osv = new Vulnerability();
    osv.osvdb_id = id;

    if((matcher = SUMMARY_PATTERN.matcher(body)) != null && matcher.find())
      osv.summary = matcher.group(1);
    if((matcher = SEVERITY_PATTERN.matcher(body)) != null && matcher.find())
      osv.severity = Double.parseDouble(matcher.group(1));
    if((matcher = MSF_PATTERN.matcher(body))!=null && matcher.find())
      osv.has_msf_exploit = true;
    return osv;
  }

  private static int get_lastpage_index(String body)
  {
    Matcher matcher = null;
    String tmp;
    int i,j;

    // this is a quick way to check ( no negative look-ahead )
    if((matcher = PAGES_CHECK.matcher(body)) == null || !matcher.find())
      return 0;
    // this should never happens
    if((matcher = PAGES_PATTERN.matcher(body)) == null || !matcher.matches())
      return 0;
    tmp = matcher.group(1);
    if((matcher = PAGE_NUMS.matcher(tmp)) == null)
      return 0;
    i=0;
    while(matcher.find())
      if((j=Integer.parseInt(matcher.group(1))) > i)
        i=j;
    return i;
  }

  private static String get_response(String query, int page) throws IOException
  {
    String line,body;
    URLConnection connection;
    BufferedReader reader;
    body = "";

    if(page>0)
      connection = new URL( "http://osvdb.org/search/search?" + query + "&page=" + page ).openConnection();
    else
      connection = new URL( "http://osvdb.org/search/search?" + query ).openConnection();

    reader = new BufferedReader( new InputStreamReader( connection.getInputStream() ) );
    while( ( line = reader.readLine() ) != null )
      body += line;

    reader.close();

    return body;
  }

  private static String read_vuln(int id) throws IOException
  {
    URLConnection connection;
    BufferedReader reader;
    String line,body;

    body = "";
    connection = new URL( "http://osvdb.org/show/osvdb/"+ Integer.toString(id)).openConnection();
    reader = new BufferedReader( new InputStreamReader(connection.getInputStream()));
    while((line = reader.readLine()) != null)
      body+= line;
    reader.close();
    return body;
  }

  public static ArrayList<Vulnerability> search( String query )
  {
    ArrayList<Vulnerability> results = new ArrayList<Vulnerability>();
    String		   body		  = "";
    int cur_page,last_page;

    try
    {
      query = "search[vuln_title]=" + URLEncoder.encode( query, "UTF-8" ) + APPEND_REQUEST;
    }
    catch( UnsupportedEncodingException e )
    {
      query = "search[vuln_title]=" + URLEncoder.encode( query ) + APPEND_REQUEST;
    }
    Logger.debug("OSVDatabase.query = \""+query+"\"");
    try
    {
      cur_page=0;
      body = get_response(query, cur_page);
      last_page = get_lastpage_index(body);
      for(int id : parse_response(body)) {
        body = read_vuln(id);
        results.add(parse_vuln(body, id));
      }
      Logger.debug("last_index = "+last_page);
      cur_page++;
      while(cur_page<last_page)
      {
        body=get_response(query, cur_page++);
        for(int id : parse_response(body)) {
          body = read_vuln(id);
          results.add(parse_vuln(body, id));
        }
      }
    }
    catch( MalformedURLException mue )
    {
      System.errorLogging(mue);
    }
    catch( IOException ioe )
    {
      System.errorLogging(ioe);
    }

    return results;
  }
}
/*
 * This file is part of the cSploit.
 *
 * Copyleft of Massimo Dragano aka tux_mind <tux_mind@csploit.org>
 *
 * cSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cSploit.  If not, see <http://www.gnu.org/licenses/>.
 */
package org.csploit.android.net.datasource;

import org.csploit.android.core.Logger;
import org.csploit.android.core.System;
import org.csploit.android.net.RemoteReader;
import org.csploit.android.net.Target;
import org.csploit.android.net.metasploit.Author;
import org.csploit.android.net.metasploit.MsfExploit;
import org.csploit.android.net.reference.CVE;
import org.csploit.android.net.reference.Link;
import org.csploit.android.net.reference.OSVDB;
import org.csploit.android.net.reference.Reference;
import org.unbescape.html.HtmlEscape;

import java.io.UnsupportedEncodingException;
import java.net.MalformedURLException;
import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.Collection;
import java.util.LinkedList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

class Rapid7
{
  private static class ExploitReceiver implements RemoteReader.Receiver {

    private static final Pattern SECTION = Pattern.compile("<section[^>]*>(.*?)</section>", Pattern.MULTILINE | Pattern.DOTALL);
    private static final Pattern TITLE = Pattern.compile("<h1>(.+?)</h1>");
    private static final Pattern PARAGRAPH = Pattern.compile("<p>(.+?)</p>", Pattern.MULTILINE | Pattern.DOTALL);
    private static final Pattern ITEM = Pattern.compile("<li>(.*?)</li>", Pattern.MULTILINE | Pattern.DOTALL);
    private static final Pattern LINK = Pattern.compile("<a +href=['\"]([^\"]+?)['\"][^>]*>([^<]+)</a>");

    private final Search.Receiver<Target.Exploit> receiver;
    private final MsfExploit exploit;
    private final RemoteReader.Job job;

    public ExploitReceiver(RemoteReader.Job job, MsfExploit exploit, Search.Receiver<Target.Exploit> receiver) {
      this.job = job;
      this.exploit = exploit;
      this.receiver = receiver;
    }

    private static String getTitle(String section) {
      Matcher matcher = TITLE.matcher(section);

      if(!matcher.find())
        return null;
      return new String(matcher.group(1).toCharArray());
    }

    private static String getParagraph(String section) {
      Matcher matcher = PARAGRAPH.matcher(section);

      if(!matcher.find())
        return null;
      return HtmlEscape.unescapeHtml(matcher.group(1)).replaceAll("(?s)[\\r\\n\\s]+", " ");
    }

    private static ArrayList<String> getItems(String section) {
      Matcher matcher = ITEM.matcher(section);
      ArrayList<String> list = new ArrayList<String>();

      while(matcher.find()) {
        list.add(new String(matcher.group(1).toCharArray()));
      }

      return list;
    }

    private static Collection<Reference> getReferences(String section) {
      Matcher matcher = LINK.matcher(section);
      LinkedList<Reference> list = new LinkedList<Reference>();

      while(matcher.find()) {
        String link = matcher.group(1);
        String label = matcher.group(2);
        Reference ref;

        if(label.startsWith("CVE-")) {
          ref = new CVE(new String(label.substring(4).toCharArray()));
        } else if(label.startsWith("OSVDB-")) {
          ref = new OSVDB(Integer.parseInt(label.substring(6)));
        } else {
          ref = new Link(new String(label.toCharArray()),
                  new String(link.toCharArray()));
        }

        list.add(ref);
      }

      return list;
    }

    private static Collection<Author> getAuthors(String section) {
      Matcher matcher = ITEM.matcher(section);
      LinkedList<Author> list = new LinkedList<Author>();

      while(matcher.find()) {
        list.add(Author.fromString(HtmlEscape.unescapeHtml(matcher.group(1))));
      }

      return list;
    }

    private static MsfExploit.Ranking getRanking(String section) {
      Matcher matcher = LINK.matcher(section);

      if(!matcher.find())
        return null;

      return MsfExploit.Ranking.valueOf(matcher.group(2));
    }

    public static MsfExploit parsePage(String html) {
      Matcher matcher;
      String name, summary, description;
      ArrayList<String> targets, platforms, architectures;
      Collection<Author> authors;
      Collection<Reference> references;
      MsfExploit.Ranking ranking;

      name = summary = description = null;
      targets = platforms = architectures = null;
      authors = null;
      references = null;
      ranking = MsfExploit.Ranking.Normal;

      matcher = TITLE.matcher(html);

      if(matcher.find()) {
        summary = new String(matcher.group(1).toCharArray());
      }

      matcher = SECTION.matcher(html);

      while(matcher.find()) {
        String section = matcher.group(1);
        String title = getTitle(section);

        if(title == null) {
          if(description == null)
            description = getParagraph(section);
          continue;
        }

        if(title.equals("Module Name")) {
          name = getParagraph(section);
        } else if(title.equals("Authors")) {
          authors = getAuthors(section);
        } else if(title.equals("References")) {
          references = getReferences(section);
        } else if(title.equals("Targets")) {
          targets = getItems(section);
        } else if(title.equals("Platforms")) {
          platforms = getItems(section);
        } else if(title.equals("Architectures")) {
          architectures = getItems(section);
        } else if(title.equals("Reliability")) {
          ranking = getRanking(section);
        }
      }

      // name is a required field
      if(name == null)
        return null;

      return new MsfExploit(name, summary, description, ranking, targets, authors, platforms, architectures, references);
    }

    public static void beginFetchReferences(RemoteReader.Job job, Target.Exploit exploit, Search.Receiver<Target.Exploit> receiver) {
      for(Reference ref : exploit.getReferences()) {
        try {
          if(ref instanceof CVE) {
            job.add(((CVE)ref).getUrl(),
                    new CVEDetails.Receiver(exploit, (CVE) ref, receiver));
          } else if (ref instanceof Link) {
            if(!ref.getName().startsWith("http"))
              continue;

            job.add(((Link)ref).getUrl(),
                    new Generic.Receiver((Link) ref));
          }
        } catch (MalformedURLException e) {
          Logger.error("Bad URL: " + ref);
        } catch (IllegalStateException e) {
          Logger.warning(e.getMessage());
        }
      }
    }

    @Override
    public void onContentFetched(byte[] content) {

      MsfExploit result = parsePage(new String(content));
      result.copyTo(exploit);

      receiver.onFoundItemChanged(exploit);

      beginFetchReferences(job, exploit, receiver);
    }

    @Override
    public void onError(byte[] description) {
      Logger.error(exploit.getId() + ": " + new String(description));
    }
  }

  private static class ResultsReceiver implements RemoteReader.Receiver {

    private static final String  SEARCHFORMID = "search_form";
    private static final Pattern PAGES = Pattern.compile("[&?]page=([0-9]+)");
    private static final Pattern RESULT = Pattern.compile("<a +href=['\"]/db/modules/(exploit/[^\"]+)['\"] *>([^<]+)</a>");

    private final Search.Receiver<Target.Exploit> receiver;
    private final RemoteReader.Job job;
    private final String originalUrl;
    private final Target.Port port;

    private boolean analyzePagination = true;

    public ResultsReceiver(RemoteReader.Job job, String url, Target.Port port, Search.Receiver<Target.Exploit> receiver) {
      this.job = job;
      this.receiver = receiver;
      this.originalUrl = url;
      this.port = port;
    }

    public ResultsReceiver(RemoteReader.Job job, String url, Search.Receiver<Target.Exploit> receiver) {
      this(job, url, null, receiver);
    }

    private void parsePagination(String html) {
      Matcher matcher = PAGES.matcher(html);
      ArrayList<Integer> pages = new ArrayList<Integer>();

      while(matcher.find()) {
        Integer p = Integer.parseInt(matcher.group(1));
        if(pages.contains(p))
          continue;
        pages.add(p);

        try {
          job.add(originalUrl + "&page=" + p, this);
        } catch (MalformedURLException e) {
          Logger.error("Bad URL: " + originalUrl + "&page=" + p);
        } catch (IllegalStateException e) {
          Logger.warning(e.getMessage());
        }
      }
    }

    private void parseSearchResults(String html) {
      Matcher matcher = RESULT.matcher(html);
      MsfExploit ex;

      while(matcher.find()) {
        ex = new MsfExploit(new String(matcher.group(1).toCharArray()), port);

        receiver.onItemFound(ex);

        if(ex.isSynced()) {
          ExploitReceiver.beginFetchReferences(job, ex, receiver);
          continue;
        }

        // not connected to MSF, retrieve exploit info from web

        ExploitReceiver r = new ExploitReceiver(job, ex, receiver);

        try {
          job.add(ex.getUrl(), r);
        } catch (MalformedURLException e) {
          Logger.error("Bad URL: " + ex.getUrl());
        } catch (IllegalStateException e) {
          Logger.warning(e.getMessage());
        }
      }

      synchronized (this) {
        if(!analyzePagination) {
          return;
        }
        analyzePagination = false;
      }

      parsePagination(html);
    }

    private void parseExploit(String html) {
      MsfExploit exploit = ExploitReceiver.parsePage(html);

      exploit.setPort(port);

      receiver.onItemFound(exploit);
    }

    @Override
    public void onContentFetched(byte[] content) {
      String html = new String(content);

      if(html.contains(SEARCHFORMID)) {
        parseSearchResults(html);
      } else {
        parseExploit(html);
      }
    }

    @Override
    public void onError(byte[] description) {
      Logger.error(this + ": " + new String(description));
    }

    @Override
    public String toString() {
      return this.getClass().getName() + "[" + originalUrl + "]";
    }
  }

  public void beginSearch(RemoteReader.Job job, String query, Target.Port port, Search.Receiver<Target.Exploit> receiver) {
    String url;

    url = "http://www.rapid7.com/db/search?q=";

    try {
      url += URLEncoder.encode(query, "UTF-8");
    } catch (UnsupportedEncodingException e) {
      url += URLEncoder.encode(query);
    }

    url += "&t=m";

    Logger.debug("url = '" + url + "'");

    ResultsReceiver r = new ResultsReceiver(job, url, port, receiver);

    try {
      job.add(url, r);
    } catch (MalformedURLException e) {
      Logger.error("Bad URL: " + url);
    } catch (IllegalStateException e) {
      System.errorLogging(e);
    }
  }

  public void beginSearch(RemoteReader.Job job, String query, Search.Receiver<Target.Exploit> receiver) {
    beginSearch(job, query, null, receiver);
  }
}
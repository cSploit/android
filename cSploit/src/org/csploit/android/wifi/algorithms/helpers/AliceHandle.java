/*
 * This file is part of the dSploit.
 *
 * Copyleft of Simone Margaritelli aka evilsocket <evilsocket@gmail.com>
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
package org.csploit.android.wifi.algorithms.helpers;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public class AliceHandle extends DefaultHandler{
  private final Map<String, ArrayList<AliceMagicInfo>> supportedAlices;

  public AliceHandle(){
    supportedAlices = new HashMap<String, ArrayList<AliceMagicInfo>>();
  }

  public void startElement(String uri, String localName, String qName,
                           Attributes attributes){
    int[] magic = new int[2];
    String serial;
    String mac;
    if(attributes.getLength() == 0)
      return;
    ArrayList<AliceMagicInfo> supported = supportedAlices.get(qName);
    if(supported == null){
      supported = new ArrayList<AliceMagicInfo>(5);
      supportedAlices.put(qName, supported);
    }
    serial = attributes.getValue("sn");
    mac = attributes.getValue("mac");
    magic[0] = Integer.parseInt(attributes.getValue("q"));
    magic[1] = Integer.parseInt(attributes.getValue("k"));
    supported.add(new AliceMagicInfo(qName, magic, serial, mac));

  }

  public void endElement(String namespaceURI, String localName, String qName)
    throws SAXException{
  }

  public void characters(char[] ch, int start, int length)
    throws SAXException{
  }

  public Map<String, ArrayList<AliceMagicInfo>> getSupportedAlices(){
    return supportedAlices;
  }
}

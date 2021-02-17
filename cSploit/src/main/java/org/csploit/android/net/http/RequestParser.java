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
package org.csploit.android.net.http;

import android.util.Patterns;

import org.csploit.android.core.Logger;
import org.csploit.android.net.ByteBuffer;
import org.csploit.android.net.http.proxy.DNSCache;

import java.net.HttpCookie;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class RequestParser
{
  public final static String HOST_HEADER = "Host";
  public final static String ACCEPT_ENCODING_HEADER = "Accept-Encoding";
  public final static String CONNECTION_HEADER = "Connection";
  public final static String IF_MODIFIED_SINCE_HEADER = "If-Modified-Since";
  public final static String CACHE_CONTROL_HEADER = "Cache-Control";
  public final static String LOCATION_HEADER = "Location";
  public final static String CONTENT_TYPE_HEADER = "Content-Type";

  private final static byte[] CARRIAGE_RETURN = "\r".getBytes();
  private final static byte[] LINE_FEED = "\n".getBytes();
  private final static byte[] SPACE = " ".getBytes();

  private static final String[] TLD =
    {
      ".com.ac", ".edu.ac", ".gov.ac",
      ".net.ac", ".mil.ac", ".org.ac", ".nom.ad", ".ad",
      ".net.ae", ".gov.ae", ".org.ae", ".mil.ae", ".sch.ae", ".ac.ae",
      ".pro.ae", ".name.ae", ".ae", ".aero", ".gov.af", ".edu.af",
      ".net.af", ".com.af", ".af", ".com.ag", ".org.ag", ".net.ag",
      ".co.ag", ".nom.ag", ".ag", ".off.ai", ".com.ai", ".net.ai",
      ".org.ai", ".ai", ".gov.al", ".edu.al", ".org.al", ".com.al", ".net.al",
      ".tirana.al", ".soros.al", ".upt.al", ".am", ".com.an",
      ".net.an", ".org.an", ".edu.an", ".an", ".co.ao", ".ed.ao", ".gv.ao",
      ".it.ao", ".og.ao", ".pb.ao", ".com.ar", ".gov.ar", ".int.ar",
      ".mil.ar", ".net.ar", ".org.ar", ".iris.arpa", ".uri.arpa",
      ".urn.arpa", ".as", ".gv.at", ".ac.at", ".co.at", ".or.at",
      ".priv.at", ".at", ".asn.au", ".com.au", ".net.au", ".id.au", ".org.au",
      ".csiro.au", ".oz.au", ".info.au", ".conf.au", ".act.au",
      ".nsw.au", ".nt.au", ".qld.au", ".sa.au", ".tas.au", ".vic.au",
      ".wa.auFor", ".gov.au", ".and", ".act", ".nsw", ".nt", ".qld",
      ".sa", ".tas", ".vic", ".wa", ".com.aw", ".aw", ".ax",
      ".com.az", ".net.az", ".int.az", ".gov.az", ".biz.az", ".org.az",
      ".edu.az", ".mil.az", ".pp.az", ".name.az", ".info.az", ".az", ".com.bb",
      ".edu.bb", ".gov.bb", ".net.bb", ".org.bb", ".com.bd", ".edu.bd",
      ".net.bd", ".gov.bd", ".org.bd", ".mil.bd", ".ac.be", ".gov.bf",
      ".possibly", ".com.bm", ".edu.bm", ".org.bm", ".gov.bm", ".net.bm",
      ".com.bn", ".edu.bn", ".org.bn", ".net.bn", ".com.bo",
      ".org.bo", ".net.bo", ".gov.bo", ".gob.bo", ".edu.bo", ".tv.bo",
      ".mil.bo", ".int.bo", ".bo", ".agr.br", ".am.br", ".art.br", ".edu.br",
      ".com.br", ".coop.br", ".esp.br", ".far.br", ".fm.br", ".gov.br",
      ".imb.br", ".ind.br", ".inf.br", ".mil.br", ".net.br", ".org.br",
      ".psi.br", ".rec.br", ".srv.br", ".tmp.br", ".tur.br", ".tv.br",
      ".etc.br", ".adm.br", ".adv.br", ".arq.br", ".ato.br", ".bio.br",
      ".bmd.br", ".cim.br", ".cng.br", ".cnt.br", ".ecn.br", ".eng.br",
      ".eti.br", ".fnd.br", ".fot.br", ".fst.br", ".ggf.br", ".jor.br",
      ".lel.br", ".mat.br", ".med.br", ".mus.br", ".not.br", ".ntr.br",
      ".odo.br", ".ppg.br", ".pro.br", ".psc.br", ".qsl.br", ".slg.br",
      ".trd.br", ".vet.br", ".zlg.br", ".dpn.br", ".nom.br",
      ".com.bs", ".net.bs", ".org.bs", ".bs", ".com.bt", ".edu.bt",
      ".gov.bt", ".net.bt", ".org.bt", ".bt", ".co.bw", ".org.bw", ".bw",
      ".gov.by", ".mil.by", ".ab.ca", ".bc.ca", ".mb.ca",
      ".nb.ca", ".nf.ca", ".nl.ca", ".ns.ca", ".nt.ca", ".nu.ca",
      ".on.ca", ".pe.ca", ".qc.ca", ".sk.ca", ".yk.ca", ".ca", ".co.cc", ".cc",
      ".com.cd", ".net.cd", ".org.cd", ".cd", ".com.ch",
      ".net.ch", ".org.ch", ".gov.ch", ".ch", ".co.ck", ".others",
      ".ac.cn", ".com.cn", ".edu.cn", ".gov.cn", ".net.cn", ".org.cn",
      ".ah.cn", ".bj.cn", ".cq.cn", ".fj.cn", ".gd.cn", ".gs.cn",
      ".gz.cn", ".gx.cn", ".ha.cn", ".hb.cn", ".he.cn", ".hi.cn",
      ".hl.cn", ".hn.cn", ".jl.cn", ".js.cn", ".jx.cn", ".ln.cn",
      ".nm.cn", ".nx.cn", ".qh.cn", ".sc.cn", ".sd.cn", ".sh.cn",
      ".sn.cn", ".sx.cn", ".tj.cn", ".xj.cn", ".xz.cn", ".yn.cn",
      ".zj.cn", ".cn", ".com.co", ".edu.co", ".org.co", ".gov.co", ".mil.co",
      ".net.co", ".nom.co", ".ac.cr", ".co.cr", ".ed.cr", ".fi.cr",
      ".go.cr", ".or.cr", ".sa.cr", ".com.cu", ".edu.cu",
      ".org.cu", ".net.cu", ".gov.cu", ".inf.cu", ".cu", ".gov.cx", ".cx",
      ".com.cy", ".biz.cy", ".info.cy", ".ltd.cy", ".pro.cy", ".net.cy",
      ".org.cy", ".name.cy", ".tm.cy", ".ac.cy", ".ekloges.cy",
      ".press.cy", ".parliament.cy", ".com.dm", ".net.dm",
      ".org.dm", ".edu.dm", ".gov.dm", ".dm", ".edu.do", ".gov.do", ".gob.do",
      ".com.do", ".org.do", ".sld.do", ".web.do", ".net.do", ".mil.do",
      ".art.do", ".com.dz", ".org.dz", ".net.dz", ".gov.dz",
      ".edu.dz", ".asso.dz", ".pol.dz", ".art.dz", ".dz", ".com.ec",
      ".info.ec", ".net.ec", ".fin.ec", ".med.ec", ".pro.ec", ".org.ec",
      ".edu.ec", ".gov.ec", ".mil.ec", ".ec",".com.ee", ".org.ee",
      ".fie.ee", ".pri.ee", ".ee", ".eun.eg", ".edu.eg", ".sci.eg", ".gov.eg",
      ".com.eg", ".org.eg", ".net.eg", ".mil.eg", ".com.es",
      ".nom.es", ".org.es", ".gob.es", ".edu.es", ".es", ".com.et", ".gov.et",
      ".org.et", ".edu.et", ".net.et", ".biz.et", ".name.et", ".info.et",
      ".aland.fi", ".fi", ".biz.fj", ".com.fj", ".info.fj", ".name.fj",
      ".net.fj", ".org.fj", ".pro.fj", ".ac.fj", ".gov.fj", ".mil.fj",
      ".school.fj", ".co.fk", ".org.fk", ".gov.fk", ".ac.fk", ".nom.fk",
      ".net.fk", ".tm.fr", ".asso.fr", ".nom.fr", ".prd.fr",
      ".presse.fr", ".com.fr", ".gouv.fr", ".fr", ".com.ge", ".edu.ge",
      ".gov.ge", ".org.ge", ".mil.ge", ".net.ge", ".pvt.ge", ".ge",
      ".co.gg", ".net.gg", ".org.gg", ".gg", ".com.gh", ".edu.gh", ".gov.gh",
      ".org.gh", ".mil.gh", ".gh", ".com.gi", ".ltd.gi", ".gov.gi",
      ".mod.gi", ".edu.gi", ".org.gi", ".com.gn", ".ac.gn", ".gov.gn",
      ".org.gn", ".net.gn", ".gn", ".or", ".org.gp", ".gp", ".com.gr",
      ".edu.gr", ".net.gr", ".org.gr", ".gov.gr", ".gr", ".com.hk",
      ".edu.hk", ".gov.hk", ".idv.hk", ".net.hk", ".org.hk", ".hk",
      ".com.hn", ".edu.hn", ".org.hn", ".net.hn", ".mil.hn", ".gob.hn",
      ".hn", ".iz.hr", ".from.hr", ".name.hr", ".com.hr", ".hr",
      ".com.ht", ".net.ht", ".firm.ht", ".shop.ht", ".info.ht",
      ".pro.ht", ".adult.ht", ".org.ht", ".art.ht", ".pol.ht", ".rel.ht",
      ".asso.ht", ".perso.ht", ".coop.ht", ".med.ht", ".edu.ht",
      ".gouv.ht", ".ht", ".co.hu", ".info.hu", ".org.hu", ".priv.hu",
      ".sport.hu", ".tm.hu", ".agrar.hu", ".bolt.hu", ".casino.hu",
      ".city.hu", ".erotica.hu", ".erotika.hu", ".film.hu", ".forum.hu",
      ".games.hu", ".hotel.hu", ".ingatlan.hu", ".jogasz.hu",
      ".konyvelo.hu", ".lakas.hu", ".media.hu", ".news.hu", ".reklam.hu",
      ".sex.hu", ".shop.hu", ".suli.hu", ".szex.hu", ".tozsde.hu",
      ".utazas.hu", ".video.hu", ".hu", ".ac.id", ".co.id", ".or.id", ".go.id",
      ".gov.ie", ".ie", ".ac.il", ".co.il", ".org.il", ".net.il",
      ".gov.il", ".muni.il", ".idf.il", ".co.im", ".ltd.co.im",
      ".plc.co.im", ".net.im", ".gov.im", ".org.im", ".nic.im", ".ac.im",
      ".co.in", ".firm.in", ".net.in", ".org.in", ".gen.in",
      ".ind.in", ".nic.in", ".ac.in", ".edu.in", ".res.in", ".gov.in",
      ".mil.in", ".in", ".ac.ir", ".co.ir", ".gov.ir", ".net.ir",
      ".org.ir", ".sch.ir", ".ir", ".gov.it", ".it", ".co.je",
      ".net.je", ".org.je", ".je", ".edu.jm", ".gov.jm", ".com.jm", ".net.jm",
      ".org.jm", ".com.jo", ".org.jo", ".net.jo", ".edu.jo",
      ".gov.jo", ".mil.jo", ".jo", ".ac.jp", ".ad.jp", ".co.jp",
      ".ed.jp", ".go.jp", ".gr.jp", ".lg.jp", ".ne.jp", ".hokkaido.jp",
      ".aomori.jp", ".iwate.jp", ".miyagi.jp", ".akita.jp",
      ".yamagata.jp", ".fukushima.jp", ".ibaraki.jp", ".tochigi.jp",
      ".gunma.jp", ".saitama.jp", ".chiba.jp", ".tokyo.jp",
      ".kanagawa.jp", ".niigata.jp", ".toyama.jp", ".ishikawa.jp",
      ".fukui.jp", ".yamanashi.jp", ".nagano.jp", ".gifu.jp",
      ".shizuoka.jp", ".aichi.jp", ".mie.jp", ".shiga.jp", ".kyoto.jp",
      ".osaka.jp", ".hyogo.jp", ".nara.jp", ".wakayama.jp",
      ".tottori.jp", ".shimane.jp", ".okayama.jp", ".hiroshima.jp",
      ".yamaguchi.jp", ".tokushima.jp", ".kagawa.jp", ".ehime.jp",
      ".kochi.jp", ".fukuoka.jp", ".saga.jp", ".nagasaki.jp",
      ".kumamoto.jp", ".oita.jp", ".miyazaki.jp", ".kagoshima.jp",
      ".okinawa.jp", ".sapporo.jp", ".sendai.jp", ".yokohama.jp",
      ".kawasaki.jp", ".nagoya.jp", ".kobe.jp", ".kitakyushu.jp", ".jp",
      ".per.kh", ".com.kh", ".edu.kh", ".gov.kh", ".mil.kh", ".net.kh",
      ".org.kh", ".co.kr", ".or.kr", ".kr", ".com.kw", ".edu.kw",
      ".gov.kw", ".net.kw", ".org.kw", ".mil.kw", ".edu.ky",
      ".gov.ky", ".com.ky", ".org.ky", ".net.ky", ".ky", ".org.kz", ".edu.kz",
      ".net.kz", ".gov.kz", ".mil.kz", ".com.kz", ".net.lb", ".org.lb",
      ".gov.lb", ".edu.lb", ".com.lb", ".com.lc", ".org.lc", ".edu.lc",
      ".gov.lc", ".com.li", ".net.li", ".org.li", ".gov.li", ".li",
      ".gov.lk", ".sch.lk", ".net.lk", ".int.lk", ".com.lk",
      ".org.lk", ".edu.lk", ".ngo.lk", ".soc.lk", ".web.lk", ".ltd.lk",
      ".assn.lk", ".grp.lk", ".hotel.lk", ".lk", ".com.lr", ".edu.lr",
      ".gov.lr", ".org.lr", ".net.lr", ".org.ls", ".co.ls",
      ".gov.lt", ".mil.lt", ".lt", ".gov.lu", ".mil.lu", ".org.lu",
      ".net.lu", ".lu", ".com.lv", ".edu.lv", ".gov.lv", ".org.lv",
      ".mil.lv", ".id.lv", ".net.lv", ".asn.lv", ".conf.lv", ".lv",
      ".com.ly", ".net.ly", ".gov.ly", ".plc.ly", ".edu.ly", ".sch.ly",
      ".med.ly", ".org.ly", ".id.ly", ".ly", ".co.ma", ".net.ma",
      ".gov.ma", ".org.ma", ".ma", ".tm.mc", ".asso.mc", ".mc",
      ".org.mg", ".nom.mg", ".gov.mg", ".prd.mg", ".tm.mg", ".com.mg",
      ".edu.mg", ".mil.mg", ".mg", ".army.mil", ".navy.mil", ".com.mk",
      ".org.mk", ".mk", ".com.mo", ".net.mo", ".org.mo", ".edu.mo",
      ".gov.mo", ".mo", ".weather.mobi", ".music.mobi", ".org.mt",
      ".com.mt", ".gov.mt", ".edu.mt", ".net.mt", ".mt", ".com.mu",
      ".co.mu", ".mu", ".aero.mv", ".biz.mv", ".com.mv", ".coop.mv", ".edu.mv",
      ".gov.mv", ".info.mv", ".int.mv", ".mil.mv", ".museum.mv",
      ".name.mv", ".net.mv", ".org.mv", ".pro.mv", ".ac.mw", ".co.mw",
      ".com.mw", ".coop.mw", ".edu.mw", ".gov.mw", ".int.mw",
      ".museum.mw", ".net.mw", ".org.mw", ".com.mx", ".net.mx",
      ".org.mx", ".edu.mx", ".gob.mx", ".com.my", ".net.my", ".org.my",
      ".gov.my", ".edu.my", ".mil.my", ".name.my", ".edu.ng", ".com.ng",
      ".gov.ng", ".org.ng", ".net.ng", ".gob.ni", ".com.ni", ".edu.ni",
      ".org.ni", ".nom.ni", ".net.ni", ".nl", ".mil.no",
      ".stat.no", ".kommune.no", ".herad.no", ".priv.no", ".vgs.no",
      ".fhs.no", ".museum.no", ".fylkesbibl.no", ".folkebibl.no",
      ".idrett.no", ".no", ".com.np", ".org.np", ".edu.np", ".net.np",
      ".gov.np", ".mil.np", ".gov.nr", ".edu.nr", ".biz.nr",
      ".info.nr", ".org.nr", ".com.nr", ".net.nr", ".nr", ".ac.nz", ".co.nz",
      ".cri.nz", ".gen.nz", ".geek.nz", ".govt.nz", ".iwi.nz",
      ".maori.nz", ".mil.nz", ".net.nz", ".org.nz", ".school.nz",
      ".com.om", ".co.om", ".edu.om", ".ac.com", ".sch.om", ".gov.om",
      ".net.om", ".org.om", ".mil.om", ".museum.om", ".biz.om",
      ".pro.om", ".med.om", ".com.pa", ".ac.pa", ".sld.pa", ".gob.pa",
      ".edu.pa", ".org.pa", ".net.pa", ".abo.pa", ".ing.pa", ".med.pa",
      ".nom.pa", ".com.pe", ".org.pe", ".net.pe", ".edu.pe", ".mil.pe",
      ".gob.pe", ".nom.pe", ".com.pf", ".org.pf", ".edu.pf", ".pf",
      ".com.pg", ".net.pg", ".com.ph", ".gov.ph", ".ph",
      ".com.pk", ".net.pk", ".edu.pk", ".org.pk", ".fam.pk", ".biz.pk",
      ".web.pk", ".gov.pk", ".gob.pk", ".gok.pk", ".gon.pk", ".gop.pk",
      ".gos.pk", ".pk", ".com.pl", ".biz.pl", ".net.pl", ".art.pl",
      ".edu.pl", ".org.pl", ".ngo.pl", ".gov.pl", ".info.pl", ".mil.pl",
      ".waw.pl", ".warszawa.pl", ".wroc.pl", ".wroclaw.pl", ".krakow.pl",
      ".poznan.pl", ".lodz.pl", ".gda.pl", ".gdansk.pl", ".slupsk.pl",
      ".szczecin.pl", ".lublin.pl", ".bialystok.pl",
      ".olsztyn.pl.torun.pl", ".pl", ".biz.pr", ".com.pr",
      ".edu.pr", ".gov.pr", ".info.pr", ".isla.pr", ".name.pr",
      ".net.pr", ".org.pr", ".pro.pr", ".pr", ".law.pro", ".med.pro",
      ".cpa.pro", ".edu.ps", ".gov.ps", ".sec.ps", ".plo.ps",
      ".com.ps", ".org.ps", ".net.ps", ".ps", ".com.pt", ".edu.pt",
      ".gov.pt", ".int.pt", ".net.pt", ".nome.pt", ".org.pt", ".publ.pt", ".pt",
      ".net.py", ".org.py", ".gov.py", ".edu.py", ".com.py",
      ".com.ro", ".org.ro", ".tm.ro", ".nt.ro", ".nom.ro", ".info.ro",
      ".rec.ro", ".arts.ro", ".firm.ro", ".store.ro", ".www.ro", ".ro",
      ".com.ru", ".net.ru", ".org.ru", ".pp.ru", ".msk.ru", ".int.ru",
      ".ac.ru", ".ru", ".gov.rw", ".net.rw", ".edu.rw", ".ac.rw",
      ".com.rw", ".co.rw", ".int.rw", ".mil.rw", ".gouv.rw", ".rw", ".com.sa",
      ".edu.sa", ".sch.sa", ".med.sa", ".gov.sa", ".net.sa", ".org.sa",
      ".pub.sa", ".com.sb", ".gov.sb", ".net.sb", ".edu.sb",
      ".com.sc", ".gov.sc", ".net.sc", ".org.sc", ".edu.sc", ".sc",
      ".com.sd", ".net.sd", ".org.sd", ".edu.sd", ".med.sd", ".tv.sd",
      ".gov.sd", ".info.sd", ".sd", ".org.se", ".pp.se", ".tm.se",
      ".brand.se", ".parti.se", ".press.se", ".komforb.se",
      ".kommunalforbund.se", ".komvux.se", ".lanarb.se", ".lanbib.se",
      ".naturbruksgymn.se", ".sshn.se", ".fhv.se", ".fhsk.se", ".fh.se",
      ".ab.se", ".c.se", ".d.se", ".e.se", ".f.se", ".g.se", ".h.se",
      ".i.se", ".k.se", ".m.se", ".n.se", ".o.se", ".s.se", ".t.se",
      ".u.se", ".w.se", ".x.se", ".y.se", ".z.se", ".ac.se", ".bd.se", ".se",
      ".com.sg", ".net.sg", ".org.sg", ".gov.sg", ".edu.sg",
      ".per.sg", ".idn.sg", ".sg", ".edu.sv", ".com.sv", ".gob.sv", ".org.sv",
      ".red.sv", ".gov.sy", ".com.sy", ".net.sy", ".ac.th", ".co.th",
      ".in.th", ".go.th", ".mi.th", ".or.th", ".net.th", ".ac.tj",
      ".biz.tj", ".com.tj", ".co.tj", ".edu.tj", ".int.tj", ".name.tj",
      ".net.tj", ".org.tj", ".web.tj", ".gov.tj", ".go.tj", ".mil.tj", ".tj",
      ".com.tn", ".intl.tn", ".gov.tn", ".org.tn", ".ind.tn", ".nat.tn",
      ".tourism.tn", ".info.tn", ".ens.tn", ".fin.tn", ".net.tn", ".to",
      ".gov.to", ".gov.tp", ".tp", ".com.tr", ".info.tr", ".biz.tr",
      ".net.tr", ".org.tr", ".web.tr", ".gen.tr", ".av.tr", ".dr.tr",
      ".bbs.tr", ".name.tr", ".tel.tr", ".gov.tr", ".bel.tr", ".pol.tr",
      ".mil.tr", ".edu.tr", ".co.tt", ".com.tt", ".org.tt",
      ".net.tt", ".biz.tt", ".info.tt", ".pro.tt", ".name.tt", ".edu.tt",
      ".gov.tt", ".tt", ".gov.tv", ".tv", ".edu.tw", ".gov.tw",
      ".mil.tw", ".com.tw", ".net.tw", ".org.tw", ".idv.tw", ".game.tw",
      ".ebiz.tw", ".club.tw", ".tw", ".co.tz", ".ac.tz", ".go.tz", ".or.tz",
      ".ne.tz", ".com.ua", ".gov.ua", ".net.ua", ".edu.ua",
      ".org.uaGeographical", ".cherkassy.ua", ".ck.ua", ".chernigov.ua",
      ".cn.ua", ".chernovtsy.ua", ".cv.ua", ".crimea.ua",
      ".dnepropetrovsk.ua", ".dp.ua", ".donetsk.ua", ".dn.ua", ".if.ua",
      ".kharkov.ua", ".kh.ua", ".kherson.ua", ".ks.ua",
      ".khmelnitskiy.ua", ".km.ua", ".kiev.ua", ".kv.ua",
      ".kirovograd.ua", ".kr.ua", ".lugansk.ua", ".lg.ua", ".lutsk.ua",
      ".lviv.ua", ".nikolaev.ua", ".mk.ua", ".odessa.ua", ".od.ua",
      ".poltava.ua", ".pl.ua", ".rovno.ua", ".rv.ua", ".sebastopol.ua",
      ".sumy.ua", ".ternopil.ua", ".te.ua", ".uzhgorod.ua",
      ".vinnica.ua", ".vn.ua", ".zaporizhzhe.ua", ".zp.ua",
      ".zhitomir.ua", ".zt.ua", ".ua", ".co.ug", ".ac.ug", ".sc.ug",
      ".go.ug", ".ne.ug", ".or.ug", ".ug", ".ac.uk", ".co.uk", ".gov.uk",
      ".ltd.uk", ".me.uk", ".mil.uk", ".mod.uk", ".net.uk", ".nic.uk",
      ".nhs.uk", ".org.uk", ".plc.uk", ".police.uk", ".bl.uk",
      ".icnet.uk", ".jet.uk", ".nel.uk", ".nls.uk",
      ".parliament.uk.sch.uk", ".uses", ".level", ".domains",
      ".ak.us", ".al.us", ".ar.us", ".az.us", ".ca.us", ".co.us",
      ".ct.us", ".dc.us", ".de.us", ".dni.us", ".fed.us", ".fl.us",
      ".ga.us", ".hi.us", ".ia.us", ".id.us", ".il.us", ".in.us",
      ".isa.us", ".kids.us", ".ks.us", ".ky.us", ".la.us", ".ma.us",
      ".md.us", ".me.us", ".mi.us", ".mn.us", ".mo.us", ".ms.us",
      ".mt.us", ".nc.us", ".nd.us", ".ne.us", ".nh.us", ".nj.us",
      ".nm.us", ".nsn.us", ".nv.us", ".ny.us", ".oh.us", ".ok.us",
      ".or.us", ".pa.us", ".ri.us", ".sc.us", ".sd.us", ".tn.us",
      ".tx.us", ".ut.us", ".vt.us", ".va.us", ".wa.us", ".wi.us",
      ".wv.us", ".wy.us", ".us", ".edu.uy", ".gub.uy", ".org.uy", ".com.uy",
      ".net.uy", ".mil.uy", ".vatican.va", ".com.ve", ".net.ve",
      ".org.ve", ".info.ve", ".co.ve", ".web.ve", ".com.vi",
      ".org.vi", ".edu.vi", ".gov.vi", ".vi", ".com.vn", ".net.vn",
      ".org.vn", ".edu.vn", ".gov.vn", ".int.vn", ".ac.vn", ".biz.vn",
      ".info.vn", ".name.vn", ".pro.vn", ".health.vn", ".vn", ".com.ye",
      ".net.ye", ".ac.yu", ".co.yu", ".org.yu", ".edu.yu", ".ac.za",
      ".city.za", ".co.za", ".edu.za", ".gov.za", ".law.za", ".mil.za",
      ".nom.za", ".org.za", ".school.za", ".alt.za", ".net.za",
      ".ngo.za", ".tm.za", ".web.za", ".co.zm", ".org.zm", ".gov.zm",
      ".sch.zm", ".ac.zm", ".co.zw", ".org.zw", ".gov.zw", ".ac.zw"
    };


  public static String getBaseDomain(String hostname){
    // if hostname is an IP address return that address
    if(Patterns.IP_ADDRESS.matcher(hostname).matches())
      return hostname;

    String cached_domain = DNSCache.getInstance().getRootDomain(hostname);
    if (cached_domain != null) {
      return cached_domain;
    }

    for(String tld : TLD){
      if(hostname.endsWith(tld)){
        String[] host_parts = hostname.split("\\."),
          tld_parts = tld.split("\\.");
        int itld = tld_parts.length,
          ihost = host_parts.length,
          i = 0;

        if ((ihost - itld) == 0 || ihost == 2)
          return hostname;

        StringBuilder sb = new StringBuilder();

        for(i = ihost - itld; i < ihost; i++){
          sb.append(host_parts[i]);
          if(i < ihost - 1) {
            sb.append(".");
          }
        }

        String domain = sb.toString();

        DNSCache.getInstance().addRootDomain(domain);
        return domain;
      }
    }

    int startIndex = 0,
      nextIndex = hostname.indexOf('.'),
      lastIndex = hostname.lastIndexOf('.');

    while(nextIndex < lastIndex) {
      startIndex = nextIndex + 1;
      nextIndex = hostname.indexOf('.', startIndex);
    }

    if(startIndex > 0) {
      DNSCache.getInstance().addRootDomain(hostname.substring(startIndex));
      return hostname.substring(startIndex);
    }
    else
      return hostname;
  }

  public static String getUrlFromRequest(String hostname, String request){
    String[] headers = request.split("\n");

    if(headers != null && headers.length > 0){
      // assuming the GET|POST|WHATEVER /url ... is on the first line
      String req = headers[0];
      // regexp would be nicer, but this way is faster
      int slash = req.indexOf('/'),
        space = req.lastIndexOf(' ');

      if(slash != -1 && space != -1 && space > slash)
        return "http://" + hostname + req.substring(slash, space);
    }

    return null;
  }

  public static ArrayList<HttpCookie> parseRawCookie(String rawCookie){
    String[] rawCookieParams = rawCookie.split(";");
    ArrayList<HttpCookie> cookies = new ArrayList<HttpCookie>();

    for(String rawCookieParam : rawCookieParams){
      String[] rawCookieNameAndValue = rawCookieParam.split("=");

      if(rawCookieNameAndValue.length != 2)
        continue;

      String cookieName = rawCookieNameAndValue[0].trim();
      String cookieValue = rawCookieNameAndValue[1].trim();
      HttpCookie cookie;

      try {
        cookie = new HttpCookie(cookieName, cookieValue);
       } catch (IllegalArgumentException e){
         Logger.error("Invalid cookie. name=" + cookieName + ":" + cookieValue);
         continue;
       }

      for(int i = 1; i < rawCookieParams.length; i++){
        String rawCookieParamNameAndValue[] = rawCookieParams[i].trim().split("=");
        String paramName = rawCookieParamNameAndValue[0].trim();

        if(paramName.equalsIgnoreCase("secure"))
          cookie.setSecure(true);

        else{
          // attribute not a flag or missing value.
          if(rawCookieParamNameAndValue.length == 2){
            String paramValue = rawCookieParamNameAndValue[1].trim();

            if(paramName.equalsIgnoreCase("max-age")){
              long maxAge = Long.parseLong(paramValue);
              cookie.setMaxAge(maxAge);
            } else if(paramName.equalsIgnoreCase("domain"))
              cookie.setDomain(paramValue);

            else if(paramName.equalsIgnoreCase("path"))
              cookie.setPath(paramValue);

            else if(paramName.equalsIgnoreCase("comment"))
              cookie.setComment(paramValue);
          }
        }
      }

      cookies.add(cookie);
    }

    return cookies;
  }

  public static String getHeaderValue(String name, ByteBuffer buffer){
    String value = null;
    int index = -1,
      end = -1;
    byte[] search = (name + ':').getBytes();

    if((index = buffer.indexOf(search)) != -1){
      index = buffer.indexOf(SPACE, index);
      if(index != -1){
        index++;

        end = buffer.indexOf(CARRIAGE_RETURN, index);
        if(end == -1)
          end = buffer.indexOf(LINE_FEED, index);
        if(end != -1)
          value = new String(Arrays.copyOfRange(buffer.getData(), index, end));
      }
    }

    return value == null ? value : value.trim();
  }

  public static String getHeaderValue(String name, ArrayList<String> headers){
    for(String header : headers){
      if(header.indexOf(':') != -1){
        String[] split = header.split(":", 2);
        String hname = split[0].trim(),
          hvalue = split[1].trim();

        if(hname.equals(name))
          return hvalue;
      }
    }

    return null;
  }

  public static ArrayList<String> getHeaderValues(String name, ArrayList<String> headers){
    ArrayList<String> values = new ArrayList<String>();

    for(String header : headers){
      if(header.indexOf(':') != -1){
        String[] split = header.split(":", 2);
        String hname = split[0].trim(),
          hvalue = split[1].trim();

        if(hname.equals(name))
          values.add(hvalue);
      }
    }

    return values;
  }

  public static ArrayList<HttpCookie> getCookiesFromHeaders(ArrayList<String> headers){
    ArrayList<String> values = getHeaderValues("Cookie", headers);

    if(values != null && values.size() > 0){
      ArrayList<HttpCookie> cookies = new ArrayList<HttpCookie>();
      for(String value : values){
        ArrayList<HttpCookie> lineCookies = parseRawCookie(value);
        if(lineCookies != null && lineCookies.size() > 0){
          cookies.addAll(lineCookies);
        }
      }

      // remove google and cloudflare cookies
      Iterator<HttpCookie> it = cookies.iterator();
      while(it.hasNext()){
        HttpCookie cookie = it.next();
        if(cookie.getName().startsWith("__utm") || cookie.getName().equals("__cfduid"))
          it.remove();
      }

      return cookies.size() > 0 ? cookies : null;
    }

    return null;
  }

  /**
   * extract the charset encoding from the HTTP response headers.
   *
   * @param contentType content-type header to be parsed
   * @return returns the charset encoding if we've found it, or null.
   */
  public static String getCharsetFromHeaders(String contentType){
    if (contentType != null && contentType.toLowerCase().trim().contains("charset=")){
      String[] parts = contentType.toLowerCase().trim().split("=");
      if (parts.length > 0)
        return parts[1];
    }

    return null;
  }

  /**
   * extract the charset encoding of a web site from the <meta> headers.
   *
   * @param body html body of the site to be parsed
   * @return returns the charset encoding if we've found it, or null.
   */
  public static String getCharsetFromBody(String body) {
    if (body != null) {
      int headEnd = body.toLowerCase().trim().indexOf("</head>");

      // return null if there's no head tags
      if (headEnd == -1)
        return null;

      String body_head = body.toLowerCase().substring(0, headEnd);

      Pattern p = Pattern.compile("charset=([\"\'a-z0-9A-Z-]+)");
      Matcher m = p.matcher(body_head);
      String str_match = "";
      if (m.find()) {
        str_match = m.toMatchResult().group(1);
        return str_match.replaceAll("[\"']", "");
      }
    }

    return null;
  }
}

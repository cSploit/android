<?php
/* $Id: header.inc.php3,v 1.15 1999/07/18 19:54:10 tobias Exp $ */

if (!isset($no_require))
   include("lib.inc.php3");


?>
<html>
<head>
<style type="text/css">
//<!--
body {  font-family: Arial, Helvetica, sans-serif; font-size: 9pt}
th   {  font-family: Arial, Helvetica, sans-serif; font-size: 9pt; font-weight: bold; background-color: <?php echo $cfgThBgcolor;?>;}
td   {  font-family: Arial, Helvetica, sans-serif; font-size: 9pt;}
form   {  font-family: Arial, Helvetica, sans-serif; font-size: 9pt}
h1   {  font-family: Verdana, Arial, Helvetica, sans-serif; font-size: 12pt; font-weight: bold}
h2   {  font-family: Verdana, Arial, Helvetica, sans-serif; font-size: 9pt; font-weight: bold}
A:link    {  font-family: Arial, Helvetica, sans-serif; font-size: 10pt; text-decoration: none; color: blue}
A:visited {  font-family: Arial, Helvetica, sans-serif; font-size: 10pt; text-decoration: none; color: blue}
A:hover   {  font-family: Arial, Helvetica, sans-serif; font-size: 10pt; text-decoration: underline; color: red}


//-->
</style>
<!--
<META HTTP-EQUIV="Expires" CONTENT="Fri, Jun 12 1981 08:20:00 GMT">
<META HTTP-EQUIV="Pragma" CONTENT="no-cache">
<META HTTP-EQUIV="Cache-Control" CONTENT="no-cache,must-revalidate">

-->
</head>

<body bgcolor="#F5F5F5" text="#000000" background="images/bkg.gif">

<TABLE WIDTH=99% BORDER=0>

<TR ALIGN=center>
<TD ALIGN=center>
   <A HREF="linuxinfos.phtml"> <IMG SRC="images/linuxlog.gif" BORDER=0 width=50 height=50> </a>
</TD>
<TD COLSPAN=3 ALIGN=CENTER VALIGN=CENTER>
 <H1>
 <? echo $strEntete ?>
 <CENTER>
<?
if (isset($message))
   {
   show_message(urldecode($message));
   }
?>
</center>

</H1></TD>
<TD ALIGN=center>

 <A HREF="nwinfos.phtml"> <IMG SRC="images/netware.gif" BORDER=0 ></A>
</TD>
  </TR>
</TABLE>
<HR>
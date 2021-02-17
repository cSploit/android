<?php
/* $Id: lib.inc.php3,v 1.41 1999/10/10 11:39:34 tobias Exp $ */

require("config.inc.php3");

   function mkjoli($string) {
     return ucfirst(strtolower($string));
   }

   function fillblank($string) {
    if (trim($string =="")) return ("&nbsp;");
    else return (mkjoli($string)); 
   }

   function justify($string,$len) {
    while (strlen($string) <$len) {
        $string=$string." ";
    }
    return $string; 
   }
   
   function justifyhtml($string,$len) {
    $l=strlen($string);
    while ($l <$len) {
        $string=$string."&nbsp;";$l++;
    }
    return $string; 
   }

   function premier($string) {
    $prns=split(" ",trim($string));
    return ($prns[0]); 
   }

   function suivant($string) {
    $prns=split(" ",trim($string));
    $s="";
    for ($i=1;$i<count($prns);$i++)  {$s=$s.trim($prns[$i])." ";}
    return ($s); 
    }
   
   function marque($string, $pattern) {  
    $beg="<font color='#FF0000' ><B>";
    $end="</b></font>";  

    if ($pattern=="") return $string;
    $string=strtolower($string);
    $pattern=strtolower($pattern);
    return(str_replace($pattern,$beg.$pattern.$end,$string));
   }


function mysql_die($error = "")
 {
 global $strError,$strMySQLSaid, $strBack;
 echo "<b> $strError </b><p>";
 if (empty($error))
    echo $strMySQLSaid.mysql_error();
 else
    echo $strMySQLSaid.$error;
 echo "<br><a href=\"javascript:history.go(-1)\">$strBack</a>";
 require ("footer.inc.php3");
 exit;
 }

function auth()
 {
 global $cfgServer;
 #$PHP_AUTH_USER = ""; No need to do this since err 401 allready clears that var
 Header("status: 401 Unauthorized"); 
 Header("HTTP/1.0 401 Unauthorized");
 Header("WWW-authenticate: basic realm=\"phpMySQLAdmin on " . $cfgServer['host'] . "\"");
 echo "<HTML><HEAD><TITLE>" . $GLOBALS["strAccessDenied"] . "</TITLE></HEAD>\n";
 echo "<BODY BGCOLOR=#FFFFFF><BR><BR><CENTER><H1>" . $GLOBALS["strWrongUser"] . "</H1>\n";
 echo "</CENTER></BODY></HTML>";
 exit; 
 } 
 

 
function count_records ($db,$table)
{	$result = mysql_db_query($db, "select count(*) as num from $table");
	$num = mysql_result($result,0,"num");
	echo $num;
}


function show_message($message)
 {
 ?>
 <div align="center">
 <table border="<?php echo $GLOBALS['cfgBorder'];?>" width=70%>
  <tr>
   <td align=center bgcolor="<?php echo $GLOBALS['cfgThBgcolor'];?>">
   <b><?php echo $message;?><b><br>
   </td>
  </tr>
 </table>
 </div>
 <?php
 }
 
function split_string($sql, $delimiter)
{
    $sql = trim($sql);
    $buffer = array();
    $ret = array();
    $in_string = false;

    for($i=0; $i<strlen($sql); $i++)
    {
         if($sql[$i] == $delimiter && !$in_string)
        {
            $ret[] = substr($sql, 0, $i);
            $sql = substr($sql, $i + 1);
            $i = 0; 
        }

        if($in_string && ($sql[$i] == $in_string) && $buffer[0] != "\\")
        {
             $in_string = false;
        }
        elseif(!$in_string && ($sql[$i] == "\"" || $sql[$i] == "'") && $buffer[0] != "\\")
        {
             $in_string = $sql[$i];
        }
        $buffer[0] = $buffer[1];
        $buffer[1] = $sql[$i];
     }    
    
    if (!empty($sql))
    {
        $ret[] = $sql;
    }

    return($ret);
}
 
// -----------------------------------------------------------------
?>

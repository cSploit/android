<?
session_start();
#doit être après l'ouverture de session
global $ss_username;
global $ss_tree;
global $ss_server;
global $ss_authenticated;
session_unregister("ss_authenticated");

$auth_err= "Pas de login!";

$ss_authenticated=0 ;
$date= date ("d/m/Y H:i:s");
$machine=getenv('http_host')." [".getenv('remote_addr')."]";

$ss_tree   =$HTTP_POST_VARS["tree"];
$ss_srv    =$HTTP_POST_VARS["srv"];
$ctx       =$HTTP_POST_VARS["ctx"];
$uu        =$HTTP_POST_VARS["uid"];
$pwd       =$HTTP_POST_VARS["pwd"];

if ($ss_tree == "" && $ss_srv == "")
                $auth_err = "Pas d'arbre ni de serveur !";
else if ($uu== "")
                $auth_err = "Nom d'utilisateur incorrect";
     else if ($pwd == "")
                $auth_err = "Mot de passe incorrect";
else {

  if ($ss_tree !="") {
        $ss_username=$uu.$ctx;
        if ($ctx == ".PC") {
           #echo "authentification à CIPCINSA<BR>";
	   $auth_err = auth_nds("CIPCINSA", $ss_username, strtoupper($pwd),"profs.pc");
       } else if ($ctx == ".GCP.PC") {
           #echo "authentification à GCP<BR>";
           $auth_err = auth_nds("GCP", $ss_username, strtoupper($pwd),"profs.gcp.pc");
       } else
	   $auth_err = "Access denied!";

  } else {
      $ss_username=$uu;
      #echo "authentification à ".$HTTP_POST_VARS["svr"]."<BR>";
      $auth_err = auth_bindery($ss_svr, $uu, $pwd, $HTTP_POST_VARS["grp"]);
  }

  # write in /var/log/secure
  openlog ("php_nds_auth",LOG_ID |LOG_PID ,LOG_AUTHPRIV);
  if ($auth_err == "") {
    $ss_authenticated=1;
    session_register("ss_username");
    session_register("ss_authenticated");
    session_register("ss_tree");
    session_register("ss_server");
    syslog (LOG_NOTICE,"granted access to ".$ss_username." the ".$date. " from ".$machine);
    closelog();
    header ("Location: menu.phtml");

  }
  else {
    syslog (LOG_WARNING,"access denied for  ".$uu." from ".$machine." due to ".$auth_err);
    closelog();

   header ("Location: index.phtml?uid=".$uu."&message=".urlencode($auth_err));
  }

}
?>


<BODY>
</body>

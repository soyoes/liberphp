<?php

/**
 *	@file: Auth.inc	
 *	@author: Soyoes 2011/08/16
 *	@uses:
 *	@example: 
 *****************************************************************************/

class Auth extends Annotation{

	/**
	 * @param $uid: 	user id
	 * @param $pass: 	password
	 * @param $schema:	user table name
	 * @return : 0=success|1=failure|2=new user
	 * */
	
	static public function login($email, $pass,$schema=null){
		$schema = $schema | Consts::$schema_user_table;
		$email = str_replace(' ','',$email);//injection
		$user = db_findFirst($schema, ["fields"=>join(",",[Consts::$schema_user_id,Consts::$schema_user_email,Consts::$schema_user_pass]),
					Consts::$schema_user_email=>$email,Consts::$schema_user_id."@>"=>0]);
		if(empty($user))return 2;
		$upass = $user[Consts::$schema_user_pass];
		if(md5($pass)!=$upass){return 1;}
		$_SESSION["uid"]=$user[$schema_user_id];
		return 0;
	}
	
	
	static public function sendActiveMail($from, $to, $redirectURI, $subject, $body){
		$code = sha1($to."-".time());
		$url = Req::getURL($redirectURI."?code=".$code);
		$message = !empty($body)?$body:Mails::render("register_activate",["url"=>$url]);
		Mails::send($from, $to, $subject, $message);
		return $code;//use to set to userdata
	}
	
	static public function activate($uid, $schema=null){
		$schema = $schema | Consts::$schema_user_table;
		db_query($schema,"update $schema set ".Consts::$schema_user_activated."=1 where ".Consts::$schema_user_id."='$uid'");
		$_SESSION["activate"] = 1;
		$_SESSION["uid"] = $uid;
	}
	
	static public function savePausedRequest(){
		$uri = REQ::$uri;
		$_SESSION["pausedURI"] 		= $uri;
		$_SESSION["pausedMethod"]	= strtolower($_SERVER["REQUEST_METHOD"]);
		$_SESSION["pausedData"] 	= REQ::$params;
	}
	
	static public function removePausedRequest(){
		unset($_SESSION["pausedURI"]);
		unset($_SESSION["pausedMethod"]);
		unset($_SESSION["pausedData"]);
	}
	
	static public function resumePausedRequest(){
		if(isset($_SESSION["pausedURI"])){
			$url = ($_SERVER['HTTPS']?"https":"http")."://".$_SERVER['HTTPS'].$_SESSION["pausedURI"];
			$res = call($url, $_SESSION["pausedMethod"], $_SESSION["pausedData"]);
			echo $res;
			self::removePausedRequest();
			exit;
		}
	}

}
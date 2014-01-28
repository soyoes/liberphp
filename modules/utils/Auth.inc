<?php

/**
 *	@file: Auth.inc	
 *	@author: Soyoes 2011/08/16
 *	@uses:
 *	@example: 
 *****************************************************************************/

define("AUTH_OK", 0);
define("AUTH_WRONG_PASSWORD", 1);
define("AUTH_NOT_EXIST", 2);
define("AUTH_WRONG_PARAM", 3);

class Auth extends Annotation{

	
	/**
	 * @params 
	 * 	uid: 	user id
	 * 	pass: 	password
	 * 	type:	users|admins
	 * @return
	 * 	0		success
	 * 	1		failure
	 * 	2		new user
	 * */
	
	static public function login($email, $pass, $type="users"){
		$model = Model::factory($type);
		$user = $model->findFirst("email='$email' and id>0");
		if(empty($user)){
			return AUTH_NOT_EXIST;
		}
		$upass = $user["pass"];
		if(md5($pass)!=$upass){
			return AUTH_WRONG_PASSWORD;
		}
		if(empty($_SESSION)){
			session_start();
		}
		$_SESSION["uid"]=$user["id"];
		return AUTH_OK;
	}
	
	static public function generateCode($email){
		$code = sha1($email."-".time());
		return $code;
	}
	
	static public function sendActiveMail($user, $to){
		$master = Master::load("mails");
		error_log("master load ");
		$from = $master['register']['from'];
		$subject = $master['register']['subject'];
		$code = self::generateCode($to);
		$url = Req::getURL("user/activate?code=".$code);
		error_log("before render ");
		$message = Mails::render("register",array("url"=>$url));
		error_log("rendered ");
		$user->set("activeCode", $code);
		error_log("before sending ");
		Mails::send($from, $to, $subject, $message);
	}
	
	static public function activate($uid){
		$model = Model::factory("user");
		$model->exec("update users set activated=1 where id='$uid'");
		$_SESSION["activate"] = 1;
		$_SESSION["uid"] = $uid;
	}
	
	static public function savePausedRequest(){
		$uri = Req::getCurrentURI();
		$uri = str_replace("showLoginPop=true", "", $uri);
		$_SESSION["pausedURI"] = $uri;
		$_SESSION["pausedMethod"] = strtolower(Req::getMethod());
		$_SESSION["pausedData"] = ("POST"==Req::getMethod())? $_POST:$_GET;
	}
	
	static public function removePausedRequest(){
		unset($_SESSION["pausedURI"]);
		unset($_SESSION["pausedMethod"]);
		unset($_SESSION["pausedData"]);
	}
	
	static public function resumePausedRequest(){
		if(isset($_SESSION["pausedURI"])){
			$uri = $_SESSION["pausedURI"];
			$method = $_SESSION["pausedMethod"];
			$data =$_SESSION["pausedData"];
			$url = Req::getURL($uri);
			$res = Req::call($url, strtoupper($method), $data);
			echo $res;
			self::removePausedRequest();
			exit;
		}
	}

}
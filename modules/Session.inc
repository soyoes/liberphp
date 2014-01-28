<?php

class Session {
	static function start(){
		//TODO CHECK commend line
		/*
		if($_REQUEST["client_type"]=="cmd"){
			return $session;
		}
		*/
		session_start();
		$ua = $_SERVER['HTTP_USER_AGENT'];
		$time = time();
		if(!isset($_COOKIE["sid"])){
			setcookie("sid", md5($_SERVER['REMOTE_ADDR']."|".$ua), $time+86400*30, "/");
			$_SESSION["IP"] = $_SERVER['REMOTE_ADDR'];
			$_SESSION["UA"] = $ua;
			$_SESSION["ISSUED_HOST"] = $_SERVER["SERVER_NAME"];
			$_SESSION["ISSUED_AT"] = $time;
		}else{
			if($_COOKIE["sid"]!=md5($_SESSION["IP"]."|".$_SESSION["UA"])
				|| $time-(isset($_SESSION["ISSUED_AT"])?$_SESSION["ISSUED_AT"]:0)>=Consts::$session_lifetime){
				//clear cookie
				setcookie("sid", "", 1);
				unset($_COOKIE["sid"]);
				session_unset();
				session_destroy();
				return self::start();
			}
		}
		//check CSRF token
		if(Consts::$csrf_check){
			$span = Consts::$csrf_lifetime;
			$expired = isset($_SESSION['@token']) && $time-(isset($_SESSION['@token_time'])?$_SESSION['@token_time']:0)>=$span;
			if($expired) $_SESSION["@token_old"] = $_SESSION["@token"];
			if(!isset($_SESSION['@token']) || $expired){
				$_SESSION['@token'] = md5(uniqid(rand(), TRUE));
				$_SESSION['@token_time'] = $time;
			}
		}
	}
	
}
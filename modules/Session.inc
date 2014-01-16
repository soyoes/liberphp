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
		if(!isset($_COOKIE["sid"])){
			setcookie("sid", md5($_SERVER['REMOTE_ADDR']."|".$_REQUEST["client_type"]), time()+86400*30, "/");
			$_SESSION = [ip => $_SERVER['REMOTE_ADDR'],clientType=>$_REQUEST["client_type"],issuedHost=>HOST_NAME];
		}else{
			if($_COOKIE["sid"]!=md5($_SESSION["ip"]."|".$_SESSION["clientType"])){
				error_log("wrong sessionid = ".$_COOKIE["sid"]);
				//New cookie
				unset($_COOKIE["sid"]);
				$_SESSION=[];
				return self::start();
			}
			error_log("use UUID = ".$_COOKIE["sid"]);
		}
	}

}
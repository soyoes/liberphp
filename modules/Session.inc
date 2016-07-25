<?php

class Session {
	static function start(){
		/* TODO CHECK commend line
		if($_REQUEST['client_type']=='cmd'){
			return $session;
		}*/
		session_start();
		$ua = $_SERVER['HTTP_USER_AGENT'];
		$time = time();
		if(!isset($_COOKIE['sid'])){
			setcookie('sid', md5($_SERVER['REMOTE_ADDR'].'|'.$ua), $time+86400*30, '/');
			$_SESSION['IP'] = $_SERVER['REMOTE_ADDR'];
			$_SESSION['UA'] = $ua;
			$_SESSION['ISSUED_HOST'] = $_SERVER['SERVER_NAME'];
			$_SESSION['ISSUED_AT'] = $time;
			$_SESSION['CSRF_NOUNCE'] = md5(uniqid(rand(), TRUE));//secret cookie for CSRF attack
			setcookie('sidsecr', sha1($_SESSION['CSRF_NOUNCE']), $time+86400*30, '/');
		}else{
			if($_COOKIE['sid']!=md5($_SESSION['IP'].'|'.$_SESSION['UA'])
				|| $time-(isset($_SESSION['ISSUED_AT'])?$_SESSION['ISSUED_AT']:0)>=Consts::$session_lifetime){
				//clear cookie
				self::clear();
				return self::start();
			}else if($_COOKIE['sidsecr']!=sha1($_SESSION['CSRF_NOUNCE'])){
				self::clear();
				return error(400); // maybe cross domain
			}
		}
	}
	
	private static function clear(){
		setcookie('sid', '', 1);
		setcookie('sidsecr', '', 1);
		unset($_COOKIE['sid']);
		unset($_COOKIE['sidsecr']);
		session_unset();
		session_destroy();
	} 
	
}
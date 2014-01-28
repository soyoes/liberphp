<?php

/**
 *	@file: Mails.inc	
 *	@author: Soyoes 2011/05/20
 *	@uses:
 *	@example: 
 *****************************************************************************/

class Mails {

	static $texts;
	
	static public function send($from, $to, $subject, $message){
		if(is_array($to))
			$to=join(",",$to);
		$headers  = "MIME-Version: 1.0\r\n";
 		$headers .= "Content-type: text/html; charset: utf8\r\n";
		$headers .= "From: $from \r\n";
		mail($to, $subject, $message, $headers);
	}
	
	static public function sendAsAdmin($to,$subject,$message){
		//$CONF
		self::send(APP_ADMIN_MAIL, $to, $subject, $message);
	}
	
	//FIXME
	static public function render($template,$data){
		$appName = str_has( $_SERVER['REQUEST_URI'],"/".APP_NAME."/")?"/".APP_NAME:"";
		$render = new Smarty();
		$tmpdir = APP_DIR.__SLASH__."tmp".__SLASH__;
		
		$render->setTemplateDir(APP_DIR.__SLASH__."views".__SLASH__."mail");
		$render->setCompileDir($tmpdir.'templates_c');
		$render->setCacheDir($tmpdir.'cache');
		$render->setConfigDir($tmpdir.'configs');
		$render->assign('CLIENT_TYPE',$_REQUEST['client_type']);
		if(!str_ends($template, ".html"))
			$template .= '.html';
		$render->assign("template",$template);
		foreach ($data as $k=>$v){
			$render->assign($k,$v);
		}
		$render->assign("appPath", "http://".$_SERVER["HTTP_HOST"].$appName);
		$render->assign("imagePath", get_conf("APP_IMAGE_HOST"));
		$render->assign("jsPath", "http://".$_SERVER["HTTP_HOST"].$appName."/js");
		$render->assign("cssPath", "http://".$_SERVER["HTTP_HOST"].$appName."/css");
		//render template file
		$content = $render->fetch("_layout.html");
		return $content;
	}
	
}
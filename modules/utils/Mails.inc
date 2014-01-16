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
	
	/**
	 * get mail text & replace keywords
	 * 
	 * master : conf/masters/mails.json
	 * 
	 * $keyPath: keyPath in mails.json
	 * 
	 * $replacement: words you want to replace. 
	 * 		for example: array('event_name', $e['name']);
	 * 					this will replace all '%event_name%' in texts to $e['name']
	 * 
	 * */
	static public function getText($keyPath,$replacement=array()){
		
	}
	
	static public function render($template,$data){
		$appName = Strings::contains( $_SERVER['REQUEST_URI'],"/".APP_NAME."/")?"/".APP_NAME:"";
		$render = new Smarty();
		$path = VIEW_DIR.'templates/mail';	
		$render->setTemplateDir($path);
		$render->setCompileDir(VIEW_DIR.'templates_c');
		$render->setCacheDir(VIEW_DIR.'cache');
		$render->setConfigDir(VIEW_DIR.'configs');
		$render->assign('CLIENT_TYPE',$_REQUEST['client_type']);
		$uid = $_REQUEST["uid"];
		if($uid!=null && $uid>0){
			$user = $_REQUEST["user"];
			$render->assign("me", $user->data);
		}
		if(!Strings::endsWith($template, ".tpl") && !Strings::endsWith($template, ".html"))
			$template .= '.tpl';
		$render->assign("template",$template);
		foreach ($data as $k=>$v){
			$render->assign($k,$v);
		}
		$render->assign("appPath", "http://".$_SERVER["HTTP_HOST"].$appName);
		$render->assign("imagePath", IMAGE_HOST);
		$render->assign("jsPath", "http://".$_SERVER["HTTP_HOST"].$appName."/js");
		$render->assign("cssPath", "http://".$_SERVER["HTTP_HOST"].$appName."/css");
		//render template file
		$content = $render->fetch("_layout.tpl");
		return $content;
	}
	
}
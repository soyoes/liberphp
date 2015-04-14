<?php

/**
 *	@file: Mails.inc	
 *	@author: Soyoes 2011/05/20
 *	@uses:
 *	@example: 
 *****************************************************************************/

class Mails {

	static $texts;
	
	/*
	 * send noreply mail only !!!
	 * use other ways if you want to trace the reply mails.
	 */
	static public function send($from, $to, $subject, $message){
		if(is_array($to))
			$to=join(",",$to);
		$headers  = "MIME-Version: 1.0\r\n";
 		$headers .= "Content-type: text/html; charset: UTF-8\r\n";
 		$headers .= "Content-Transfer-Encoding: base64\r\n";
		$headers .= "From: $from \r\n";
		$headers .= "Reply-To: noreply@".$_SERVER["HTTP_HOST"]." \r\n";
		$subject = "=?UTF-8?B?".base64_encode($subject)."?=";    
		mail($to, $subject, $message, $headers);
	}
	
	static public function render($template, $data){
		$path = APP_DIR.__SLASH__."views".__SLASH__."mail";
		if(!str_ends($template,".html")) $template .= ".html";
		$render = Render::factory($path);
		return $render->render($template,$data,null,true);
	}
	
}
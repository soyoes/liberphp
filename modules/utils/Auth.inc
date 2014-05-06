<?php

/**
 *	@file: Auth.inc	
 *	@author: Soyoes 2011/08/16
 *	@uses:
 *	@example: 
 *****************************************************************************/

class Auth{
	
	/**
	 * permission for each group is a N-bit binary number
	 * 1) restful request:
	 * 		in conf/schemas/YOUR_SCHEMA.ini
	 * 		[general]
	 * 		permisstion = 08F
	 * 2) normal request:
	 * 		in comment of controllers/YOUR_CONTROLLER.inc 
	 * 		@permission = 8CF
	 * 
	 * available numbers of each bit(auth group)
	 * 	0:0000, no permission for this group
	 *  8:1000, read only(GET)
	 * 	9:1001, read(GET) + delete(DELETE)
	 * 	A:1010, read(GET) + update(PUT)
	 *  B:1011, read(GET) + update(PUT), delete(DELETE)
	 * 	C:1100, read(GET) + add(POST)
	 * 	D:1101, read(GET) + add(POST), delete(DELETE)
	 *	E:1110, read(GET) + add(POST), update(PUT)
	 * 	F:1111, read(GET) + add(POST), update(PUT), delete(DELETE)
	 *  
	 * meanings of each bit
	 * 1st : permission of guest group default=8 (read only)
	 * Nth : permission of Nth auth group (N = Auth::getAuthGroup()["group"]). default = F (all permission)
	 * 
	 * @return : 200=OK, 401=Unauthorized, 403=Forbidden
	 * 
	 * */
	static public function checkPermission(REQ $req){
		$uri = $req->getURI();
		$schemaDef = $req->getData("schema_def");
		$auth = self::getAuthGroup();
		$permission = "";
		if(!empty($schemaDef)){//RESTFUL
			$restful =  strtolower($schemaDef["general"]["restful"]?$schemaDef["general"]["restful"]:"");
			//not permit restful on this schema
			if(!empty($resful) && $restful!="all" && !str_has($restful, $method)){ return false; }
			$permission = isset($schemaDef["general"]["permission"])?$schemaDef["general"]["permission"]:"";
		}else{
			$ctl = $req->getController();
			$ns = $req->getNamespace();
			$tree = cache_get('APP_PERMISSION_'.$ctl, function($key){
				$ctrldir = APP_DIR.__SLASH__.'controllers'.__SLASH__;
				$controller_dir = !empty($ns) ? $ctrldir.$ns.'/':$ctrldir;
				$fp = $controller_dir.str_replace('APP_PERMISSION_',"",$key).".inc";
				$tree = fs_src_tree($fp);
				$permission = ["@file" => $tree["annotations"]["permission"]];
				foreach ($tree["functions"] as $fn => $ftr){
					$permission[$fn] = $ftr["annotations"]["permission"];
				}
				return $permission; 
			},false);
			$act = $req->getAction();
			$permission = isset($tree[$act])?$tree[$act]:$tree["@file"];
		}
		
		$bits = !isset($permission)?($auth["group"]==0?"8":"F"): $permission[$auth["group"]];//bits is hex string (len=1)
		if($bits=="0") return $auth["group"]==0? 401 : 403;
		$bits = base_convert($bits, 16, 2);//int -> 4 bit binary string
		$bitIdx = array_search($req->getMethod(), ['get','post','put','delete']);
		if($bits[$bitIdx]!="1") return $auth["group"]==0? 401 : 403;
		return 200;
	}

	static public function getAuthGroup(){
		return empty($_SESSION)?[group=>0,user=>0]:[
			group => empty($_SESSION[Consts::$auth_group_name])?0:intval($_SESSION[Consts::$auth_group_name]),
			user => empty($_SESSION[Consts::$auth_user_id_name])?0:intval($_SESSION[Consts::$auth_user_id_name])
		];
	}
	
	
	/**
	 * @param $req:	current REQ object
	 * @return : [
	 * 	error => -1=wrong configuration|0=success|1=wrong pass|2=not exists,
	 * 	user => user data from db,
	 *  group => groupId,
	 * ]
	 * */
	static public function login(REQ $req){
		$p = Consts::$auth_patterns[$req->getURI()];
		$schemaDef = db_schema($p["schema"]);
		if(empty($p) || empty($schemaDef)){
			return ['error'=>-1];
		}
		$r = AuthDelegate::login($p ,$req->params);
		if($r["error"]){ return ["error"=>$r["error"]];}
		$pk = $schemaDef['general']['pk'];
		$_SESSION[Consts::$auth_group_name] = $p["group"];
		$uid_key = $r["activated"]?Consts::$auth_user_id_name:"__".Consts::$auth_user_id_name;
		$u_key = $r["activated"]?Consts::$auth_user_data_name:"__".Consts::$auth_user_data_name;
		$_SESSION[$uid_key] = $r["user"][$pk];
		$_SESSION[$u_key] = $r["user"];
		return [
			'group'	=> $p['group'],
			'user'	=> $r["user"],
			'userId'=> $r["user"][$pk],
			'activated'=>$r['activated']
		];
	}
	
	static public function logout(){
		$_SESSION[Consts::$auth_user_id_name] = 0;
		$_SESSION[Consts::$auth_group_name] = 0;
		unset($_SESSION[Consts::$auth_user_data_name]);
	}
	
	static public function sendActiveMail($from, $to, $redirectURI, $subject, $body){
		$code = sha1($to."-".time());
		$url = Req::getURL($redirectURI."?code=".$code);
		$message = !empty($body)?$body:Mails::render("register_activate",["url"=>$url]);
		Mails::send($from, $to, $subject, $message);
		return $code;//use to set to userdata
	}
	
	static public function activate(REQ $req){
		$p = Consts::$auth_patterns[$req->getURI()];
		$res = AuthDelegate::activate($p["schema"],$req->params);
		if($res){
			$_SESSION[Consts::$auth_user_id_name]=$_SESSION["__".Consts::$auth_user_id_name];
			$_SESSION[Consts::$auth_user_data_name]=$_SESSION["__".Consts::$auth_user_data_name];
			unset($_SESSION["__".Consts::$auth_user_id_name]);
			unset($_SESSION["__".Consts::$auth_user_data_name]);
		}
		return $res;
	}
	
	static public function savePausedRequest(REQ $req){
		$_SESSION["pausedURI"] 		= $req->getURI();
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
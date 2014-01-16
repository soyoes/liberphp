<?php
/**
 *	@file: Tests.inc	
 *	@author: Soyoes 2011/09/08
 *	@uses: running test case, automatically
 *	@example: 
 *
 	1) create test case file under /test with a name of ${YOUR_CONTROLLER}.json 
	2) add test case to /test/${YOUR_CONTROLLER}.json
	3) debug with URL of /${YOUR_CONTROLLER}/test_${YOUR_ACTION}, without any parameters!.
 *
 *
 *
 *****************************************************************************/

class Tests {
	public static function prepare($case){
		$params = $case["params"];
		//TODO
	}
	
	public static function exec($controller, $action, $case){
		$uri = "/".APP_NAME."/".$controller."/".$action;
		$method = $case["method"];
		$params = $case["params"];
		$keys = array();
		if(isset($params)){
			$keys = array_keys($params);
			$paramStr = "?";
			foreach ($params as $k=>$v){
				$v = is_array($v) ? json_encode($v) : $v;
				$paramStr.=$k."=".$v."&";
				$_REQUEST[$k] = $v;
			}
			$uri .= $paramStr;	
		}
		echo "<div><div style='background-color:black;color:green'><hr>TestCase:$uri</div>";
		$req = new Req($uri);
		$req->exec();
		$contents = $req->currentController->contents;
		foreach ($keys as $key){
			unset($_REQUEST[$key]);
		}
		echo join("", $contents)."</div>";
	}
}
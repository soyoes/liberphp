<?php
/**
 *	@file: Tests.inc	
 *	@author: Soyoes 2011/09/08
 *	@uses: running test case, automatically
 *	@example: 
 *
 	1) create test case file under /test with a name of ${YOUR_CONTROLLER}_${YOUR_ACTOIN}.csv
	2) add test case to /test/${YOUR_CONTROLLER}_${YOUR_ACTION}.csv
	3) cURL of /${YOUR_CONTROLLER}/test_${YOUR_ACTION} from browser, without any parameters!.
 *
 *
 *
 *****************************************************************************/

class Tests {
	public static function run($controller, $action){
		$head = <<<EOF
<!DOCTYPE html>
<html>
<head>
	<title>LiberPHP2 Test Result</title>
	<meta http-equiv="X-UA-Compatible" content="IE=edge"> 
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
	<script src="http://liberjs.org/liber.js"></script>
	<style>
	*{font-family:"Hiragino Maru Gothic Pro",'Meiryo',"Osaka","Verdana";}
	div,td{font-size:10pt;}
	table{border-collapse:collapse;margin-bottom:10px;}
	td, th{padding-left:5px;padding-right:5px;border:1px solid #CCC;}
	.case{width:960px;margin-left:auto;margin-right:auto;margin-bottom:10px;}
	.case > h3{background-color:rgb(54,70,83);color:white;padding:5px;padding-left:10px;}
	</style>
</head>
<body>
EOF;
		echo $head;
		$cases = self::loadCases($controller,$action);
		if (!empty($cases)){
			$idx = 1;
			foreach ($cases as $case){
				self::exec($case,$idx);
				$idx++;
			}
		}else{
			echo "<H1>Error: No Case Found.</H1><br>Test case does not exist <br> Pls add cases to /test/[CONTROLLER]_[ACTION].csv<br>And try again.";
			echo "</body></html>";
			error(400,"html");
		}
		echo "</body></html>";
	}

	static function writeJSON($text){
		$data = json_decode($text,true);
		echo "<table>";
		if(is_hash($data)){
			echo "<tr><th>".join("</th><th>",array_keys($data))."</th></tr>";
			echo "<tr><td>".join("</td><td>",array_values($data))."</td></tr>";
			echo "</table><div>Response Type : JSON(assoc_array)</div>";	
		}else if(is_array($data)){
			$cols = [];
			foreach($data as $d){$cols += array_keys($d);}
			$cols = array_unique($cols);
			$row = 0;
			foreach($data as $d){
				echo "<tr>";
				if($row==0)
					echo "<th>".join("</th><th>",array_keys($d))."</th></tr><tr>";
				echo "<td>".join("</td><td>",array_values($d))."</td>";
				echo "</tr>";
				$row ++;
			}
			echo "</table><div>Response Type : JSON(array)</div>";	
		}
		
	}


	private static function loadCases($controller, $action){
		$file = APP_DIR.__SLASH__."test".__SLASH__.$controller."_".$action.".csv";
		if (file_exists($file)){
			//$file = fopen($file, 'r');
			$row = 0; $titles = []; $cols=0;
			$cases = [];
			$lines = str_getcsv(file_get_contents($file),"\r\n");
			foreach ($lines as $line) {
				$line = explode("\t",$line);
				if($row == 0) {
					$titles = $line;
					$cols = count($titles);
				}else{
					$case = ['[method]'=>'get', '[uri]'=>__SLASH__.$controller.__SLASH__.$action, '[controller]'=>$controller,'[action]'=>$action];
					$empty = true;
					for($i=0;$i<$cols;$i++){
						if(!empty($line[$i])) {
							$case[$titles[$i]] = $line[$i];
							$empty = false;
						}
					}
					if(!$empty) $cases[]=$case;
				}
				$row++;
			}
			return $cases;
		}
		return [];
	}

	public static function exec($case,$idx){
		$uri =$case["[uri]"];
		$method = $case["[method]"];
		$params = [];
		foreach ($case as $k=>$v){
			if(!str_has($k,"[")) $params[$k] = $v;
		}
		$paramStr  = str_replace("%2C",",", http_build_query($params));
		echo "<div class='case'>";
		echo "<h3> $idx) ".strtoupper($method)." : $uri $paramStr</h3>";
		$req = REQ::dispatch($uri, $method, $params);
		echo "</div>";
	}
}
?>
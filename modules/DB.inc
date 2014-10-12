<?php
/**
 *	@file: DB.inc	
 *	@author: Soyoes 2014/01/10
 *	@uses: db library
 *	@example: 
 *****************************************************************************/

if (!apc_load_constants("DB_FMTS")){
	apc_define_constants("DB_FMTS", [
	'FORMAT_CREATE_DB' 			 => "CREATE TABLE `%s`.`%s` ( %s PRIMARY KEY (`%s`)) ENGINE=InnoDB %s DEFAULT CHARSET=utf8;",
	'FORMAT_CREATE_DB_MULTI_KEY' => "CREATE TABLE `%s`.`%s` ( %s CONSTRAINT %s PRIMARY KEY (%s)) ENGINE=InnoDB %s DEFAULT CHARSET=utf8;",
	'FORMAT_INSERT_DB' 			 => "INSERT %s INTO `%s` (%s) VALUES(%s);",
	'FORMAT_UPDATE_DB'			 => "UPDATE `%s` SET %s WHERE `%s`=%s;",
	]);
}

/**
 * For HTTP request only
 * get connection handler of PDO
 * @param opts:use conf.ini as default
 * @return PDO object.
 * */
function db_conn($opts=null, $pdoOpts=null){
	$db = REQ::getDB();
	if(!isset($db)){
		$db = pdo_conn($opts,$pdoOpts);
		REQ::setDB($db);
	}
	return $db;
}

/**
 *create pdo connection, return pdo instance.
 **/
function pdo_conn($opts=null, $pdoOpts=null){
	$opts = $opts ? $opts: [
		'engine'=>Consts::$db_engine,
		'host'	=>Consts::$db_host,
		'port'	=>Consts::$db_port,
		'db'	=>Consts::$db_name,
		'user'	=>Consts::$db_user,
		'pass'	=>Consts::$db_pass,
	];
	$conn_str = $opts["engine"].":host=".$opts["host"].";port=".$opts["port"].";dbname=".$opts["db"].";charset=utf8";
	$pdoOpts = $pdoOpts ? $pdoOpts :[PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION];
	return new PDO($conn_str,$opts["user"],$opts["pass"],$pdoOpts);
}

/**
 *query with pdo instance 
 **/
function pdo_query($pdo, $sql, $datas=[]) {
	if(!$pdo || empty($sql))return false;
	error_log($sql);
	$isQeury = str_starts(strtolower(trim($sql)), "select");
	$statement = $pdo->prepare($sql);
	if ($statement->execute ($datas) == FALSE) {
		return false;
	}
	return $isQeury? $statement->fetchAll(PDO::FETCH_ASSOC):true;
}

function pdo_count($pdo, $sql, $datas=[], $col=0){
	if(!$pdo || empty($sql))return false;
	$statement = $pdo->prepare($sql);
	if ($statement->execute ($datas) == FALSE) {
		return false;
	}
	$res =  $statement->fetchColumn();
	return intval($res);
}

function pdo_import($pdo, $table, $datas, $regName="regAt", $updName="updAt"){
	if(!isset($pdo) ||!isset($table) || count($datas)==0)
		return false;
	$schema = db_schema($table)["schema"];
	$cols = [];
	foreach ($datas as $d){
		$cols = array_unique(array_merge($cols,array_keys($d)));
	}
	$cls = $cols;$cols=[];$schema_cols =array_keys($schema);
	foreach($cls as $c){
		if(in_array($c, $schema_cols)){
			$cols[]=$c;
		}
	}
	$hasRegStamp = !empty($regName) && array_key_exists($regName,$schema);
	if($hasRegStamp && !in_array($regName, $cols)) $cols[] = $regName;
	$hasTimestamp = !empty($updName) && array_key_exists($updName,$schema);
	if($hasTimestamp && !in_array($updName, $cols)) $cols[] = $updName;
	$sql = "INSERT IGNORE INTO ".$table." (`".join("`,`", $cols)."`) VALUES ";
	$time = time();
	foreach ($datas as $d){
		if($hasRegStamp && empty($d[$regName])){$d[$regName]=$time;}
		if($hasTimestamp && empty($d[$updName])){$d[$updName]=$time;}
		$vals = [];
		foreach ($cols as $c){
			$v = array_key_exists($c, $d) ? $d[$c] : null;
			$vals[]=db_v($v, $schema[$c]);
		}
		$sql.=" (".join(",", $vals)."), ";
	}
	$sql = substr($sql, 0, strlen($sql)-2);
	$pdo->setAttribute(PDO::ATTR_TIMEOUT, 1000);
	try{
		pdo_query($pdo, $sql);
		$pdo->setAttribute(PDO::ATTR_TIMEOUT, 10);
	}catch(Exception $e){
		$pdo->setAttribute(PDO::ATTR_TIMEOUT, 10);
		$file = '/tmp/'.$table.'_imp.sql';
		file_put_contents($file,$sql);
		return false;
	}
}
function pdo_save($pdo, $table, $data, $returnId=false, $bson=false){
	if(!isset($pdo) || !isset($table) || !is_hash($data) || empty($data))return false;
	$regName = Consts::$schema_reg;
	$updName = Consts::$schema_upd;
	$schema_def = db_schema($table);
	$schema = $schema_def["schema"];
	$pk = $schema_def["general"]["pk"];
	$pks=[];$qo=null;$isUpdate=false;
	if (str_has($pk, "|")||str_has($pk, "+")||str_has($pk, ",")){
		$pks = preg_split("/[\|\+,]/", $pk);
		$qo = [];
		foreach ($pks as $p){
			if(empty($data[$p])){
				return false;
			}else 
				$qo[$p] = $data[$p];
		}
		try{
			$ext = pdo_find($pdo, $table, $qo);
		}catch(Exception $e){
			echo $e->getMessage()."\n";
		}
		$isUpdate = !empty($ext);
	} else{
		$id = isset($data[$pk]) ? $data[$pk] : null;
		$isUpdate = isset($id) && pdo_exists($pdo, $table, $id);
	}
	$sql = "";
	if(array_key_exists($updName,$schema) && !isset($data[$updName]))
		$data[$updName] = ms();
	$qdatas = [];
	if ($isUpdate){
		if($id)cache_del($table."_".$id);
		foreach ($data as $col => $val){
			if(str_ends($col,"+")) {
				$opr = substr($col,-1);
				$col = substr($col,0,-1); 
			}
			if($col==$pk || in_array($col, $pks) || !isset($schema[$col]))continue;
			if(!empty($colStmt))$colStmt .= ",";
			$colStmt .= $opr? "`$col`=`$col` + :$col " : "`$col`=:$col ";
			$qdatas[$col]= $val;//db_v($val, $schema[$col], $bson);
		}
		if(empty($pks)){
			$sql = sprintf(FORMAT_UPDATE_DB, $table, $colStmt, $pk, db_v($id));
		}else{
			list($colStr,$optStr,$qrdatas) = db_make_query($table, $qo);
			foreach ($qrdatas as $qk=>$qv){
				$qdatas[$qk]= $qv;
			}
			$sql ="update `$table` set $colStmt $optStr"; 
		}
		//echo $sql."\n";
	}else{								
		if(array_key_exists($regName,$schema) && !isset($data[$regName]))
			$data[$regName] = ms();
		foreach ($data as $col => $val){
			if(str_ends($col,"+")) {
				$opr = substr($col,-1);
				$col = substr($col,0,-1);
			}
			if(!isset($schema[$col]))continue;
			if(!empty($colStmt))$colStmt .= ",";
			if(!empty($valStmt))$valStmt .= ",";
			$colStmt .= "`$col`";
			$valStmt .= $opr? "`$col` + :$col " : ":$col";
			$qdatas[$col]= $val;//db_v($val, $schema[$col], $bson);
		}
		$sql = sprintf(FORMAT_INSERT_DB, $ignore, $table, $colStmt, $valStmt);
	}
	try {
		if($returnId==true && !$isUpdate) {
			if(!$pdo->inTransaction()) {
				$res = pdo_trans($pdo,[$sql, "SELECT LAST_INSERT_ID() as 'last_id'"],[$qdatas]);
				$data["id"] = $res[0]["last_id"];
			}else{
				pdo_query($pdo, $sql,$qdatas);
				$res = pdo_query($pdo, "SELECT LAST_INSERT_ID() as 'last_id'", [ ]);
				$data["id"] = $res[0]["last_id"];
			}
		}else{
			pdo_query($pdo, $sql,$qdatas);
		}
		return $data;
	} catch (Exception $e) {
		error_log("ERROR ".$e->getMessage());
		error_log($sql);
		return false;
	}
}
function pdo_find($pdo,$table, $opts=[], $withCount=false){
	if(!$pdo || !$table)return false;
	list($colStr, $optStr,$datas) = db_make_query($table, $opts);
	$sql = "SELECT ".$colStr." FROM ".$table.$optStr;
	$res = pdo_query($pdo, $sql, $datas, $opts["useCache"]);
	if($withCount){
		$sql = "SELECT count(*) FROM ".$table.preg_replace("/ORDER BY\s*(.*)\s*([LIMIT .*]*)/", '${3}',$optStr);
		$cnt = pdo_count($pdo,$sql, $datas, $opts["useCache"]);
		return ["count"=>$cnt,"result"=>$res];
	}else{
		return $res;
	}
}
function pdo_exists($pdo, $table, $id){
	if(!isset($pdo) ||!isset($table) || !isset($id))
		return false;
	$pk = db_schema($table)["general"]["pk"];
	$entity =pdo_count($pdo, "select count(*) from $table where `$pk`=:$pk",[$pk=>$id]);
	return $entity>0;
}
function pdo_trans($pdo,$querys,$datas){
	if(!isset($pdo)||!isset($querys))
		return false;
	$mod = $pdo->getAttribute(PDO::ATTR_ERRMODE);
	$pdo->setAttribute( PDO::ATTR_AUTOCOMMIT, 0 );
	$pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
	$pdo->beginTransaction();
	$cnt = 0;
	$res = true;
	try{
		if(is_callable($querys)){
			$cnt = $querys($pdo);
		}else if(is_array($querys)){
			$i=0;
			foreach($querys as $q){
				$i++;
				if($q==="@rollback"){
					$pdo->rollBack();$cnt--;
				}else{
					$statement = $pdo->prepare($q);
					$data = isset($datas[$i-1])?$datas[$i-1]:[];
					if ($statement->execute($data) == false) {
						continue;
					}
					if(str_starts(strtolower($q), "select")){
						$res = $statement->fetchAll(PDO::FETCH_ASSOC);					}
						$cnt++;
				}
			}
		}
	}catch(Exception $e){
		error_log("DB Transaction ERR:",$e->getMessage());
		$pdo->rollBack();
        $res = false;
	}
	if($cnt>0)
		$pdo->commit();
	$pdo->setAttribute( PDO::ATTR_AUTOCOMMIT, 1 );
	$pdo->setAttribute(PDO::ATTR_ERRMODE, $mod);
	return $res;
}

/**
 * exec query SQL like select ...
 * @param $sql: sql statement with ?.
 * @param $datas: variables inside sql statement.
 * @param $useCache: whether save/load result from APC/Memcache
 * @param $PDOOpt: FETCH_COLUMN, FETCH_ASSOC ...
 * @return array or assoc array.
 * 
 * @example :
 * 	db_query("select * from tasks");
 * @example :
 * 	db_query("select * from tasks where `id`=:id",[id=>1]);
 * @example :
 * 	db_query("select * from tasks where `id`=?",[1]);
 */
function db_query($sql, $datas=[], $useCache = false) {
	try {
		$key = $sql;
		$isQeury = str_starts(strtolower($sql), "select");
		if ($useCache && $isQeury) {
			$key = $sql."-".json_encode($datas);
			$value = cache_get($key);
			if (isset ( $value ) && $value != false)
				return $value;
		}
		$db = db_conn();
		$res = pdo_query($db, $sql, $datas);
		if ($useCache && $isQeury && $res != null) {
			cache_set($key, $res);
		}
		return $res;
	} catch ( PDOException $e ) {
		error_log ("DB ERR :". $e->getMessage() );
		error_log ("DB ERR SQL:". $sql );
		return null;
	}
}

/**
 * return number result of "SELECT COUNT"
 */
function db_count($sql=null, $datas=[], $useCache=false){
	try {
		if($useCache){
			$value = cache_get($sql);
			if(isset($value) && $value!=false)
				return $value;
		}
		$db = db_conn();
		$res = pdo_count($db, $sql, $datas);
		if($useCache && $res){
			cache_set($sql, $res);
		}
		return $res;
	} catch (PDOException $e) {
		error_log($sql);
		return -1;
	}
}

/**
 * change PDO settings 
 * @example : 
 * 	db_attr(PDO::ATTR_TIMEOUT,1000);
 * 
 * */
function db_attr($attr, $val){
	$db = db_conn();
	$db->setAttribute($attr, $val);
}

/**
 * query db with data options
 * @param $table : tablename
 * @param $opts : @see db_make_query
 * 		KEY|KEY@= 	= 	eq, equals
		KEY@!		= 	ne, not equals
		KEY@<		= 	lt, less than
		KEY@>		= 	gt, greater than
		KEY@<=		= 	le, less than or equals
		KEY@>=		= 	ge, greater than or equals
		KEY@[]		= 	in, sql in clause
		KEY@![]		= 	not in, sql not in clause
		KEY@()		= 	between
		KEY@!()		= 	not between
		KEY@?		= 	like, sql like '%YOUR_VALUE%'
		KEY@!?		= 	not like, sql not like '%YOUR_VALUE%'
		KEY@~		= 	regexp match, mysql | postgres
		KEY@!~		= 	regexp not match , mysql | postgres
		KEY@~~		= 	regexp match ignore case, mysql | postgres
		KEY@!~~		= 	regexp not match ignore case, mysql | postgres
 * 
 * @param $withCount: whether result contains total matched amount
 *  
 * @example : db_find("tasks", [
		"ownerId"	=> 3,
		"title@~"	=> "task",	//title regexp 'task'
		"fields" 	=> "id,title",
		"order"		=> "id desc",
		"limit"		=> 10
	]);
 * 
 */
function db_find($table, $opts=[], $withCount=false){
	return pdo_find(db_conn(), $table, $opts, $withCount);
}
/**
 * search with data options, returns 1st matched record.
 * @param $table : tablename
 * @param $opts : @see db_make_query
 *
 * @example : db_find1st("tasks", [
 		"ownerId"	=> 3,
 		"title@~"	=> "task",	//title regexp 'task'
 	]);
 *
 */
function db_find1st($table, $opts=[]){
	$opts["limit"]=1;
	$res = db_find($table,$opts,false,$decodeBson);
	return isset($res)&&$res!=false ? $res[0]:false;
}

/**
 * insert many records to DB
 * @param $datas : hash list
 * @param table:tablename
 * @example:
 * 	db_import("tasks", [
		[
			"ownerId"	=> 3,
			"title"=> "taskD with bson",
			"data" => ["name"=>"ssss","age"=>18]
		],
		[
			"ownerId"	=> 4,
			"title"=> "taskE with bson",
			"data" => ["name"=>"ssss","age"=>18]
		]
	]);
 * 
 * */
function db_import($table, $datas){
	return pdo_import(db_conn(), $table, $datas, Consts::$schema_reg, Consts::$schema_upd);
}

/**
 * make SQL query from query condition array
 * @param $table:tablename 
 * @param $opts:hash or str
		if opts is Array : reserved keywords in opts  
 		* fields = default = * ||  id,pass,... || [id, pass, name ...] || count(*) 
 		* limit : 0/default=all, 0,20=select 0~20, 21~80= select from 21 get 60 records 
 		* order : default = "" || id desc || updAt desc 
 		* useCache : false | true 
 		* @id : $opts[PK]=$opts["@id"]
 * @param $omit : array, fields in opts to omit.
 * @return array or false
 */
function db_make_query($table, $opts=[], $omit=[]){
	db_init_filters();
	if(!isset($table))return false;
	$colStr =  is_array($opts['fields'])? (count($opts['fields'])==0? "*" :"`".join("`,`", $opts['fields'])."`")
		 : (isset($opts['fields'])? '`'.preg_replace('/[`\s]*,[`\s]*/',"`,`",$opts['fields']).'`':'*');
	$schemaDef = db_schema($table);
	$pk = $schemaDef["general"]["pk"];
	$schema = $schemaDef["schema"];
	$data = [];
	if(is_hash($opts)){
		$optStr = [];
		// fix: When "@id" and "id" both exist.
		if(array_key_exists("@id", $opts) && array_key_exists("id", $opts)) {
			unset($opts['@id']);unset($opts['limit']);
		}
		foreach ($opts as $k => $v){
			if($k=="@id")$k=$pk;
			list($k,$cmd) = explode("@",$k);
			$keys = array_filter(preg_split('/\|/',$k), function($k) use($schema, $omit){
				return array_key_exists($k, $schema) && !in_array($k,$omit);
			});
			if(!empty($keys)){
				$cmd = !isset($cmd)||$cmd=="" ? "=":$cmd;
				$cmd = strpbrk($cmd, 'begilmnqt') !==false? Consts::$query_filter_names[$cmd]:$cmd;
				$func = Consts::$db_query_filters[$cmd];
				$vStr = $func(join("|", $keys), $v, $data);
				if($vStr) $optStr []= $vStr;
			}
		}
		$optStr =  empty($optStr) ? "": " WHERE ".join(" AND ", $optStr);
		
		if(!in_array("order",$omit) && !empty($opts["order"]))
			$optStr .= " ORDER BY ".$opts["order"];
		if(!in_array("limit",$omit) && !empty($opts["limit"]))
			$optStr .= " LIMIT ".$opts["limit"];
	}else {
		$optStr = !empty($opts)? " WHERE ". $opts : "";
	}
	return [$colStr,$optStr,$data];
}

function db_exists($table, $id){
	return pdo_exists(db_conn(), $table, $id);
}

/**
 * @param $table:tablename
 * @param $opts:query options
 *
 * @example : db_delete("tasks",[groupId=>3]); //delete from ... where groupId=3
 * */
function db_delete($table, $opts){
	if(empty($opts))return false;
	list($cs,$optStr,$data) = db_make_query($table, $opts,["order","limit","fields"]);
	$sql = "DELETE FROM $table ".$optStr;
	return db_query($sql,$data);
}

/**
 * @param $table:tablename
 * @param $data:hash, new value to update 
 * @param $opts:query options
 * 
 * @example : db_update("tasks",[groupId=>3],["id@>"=>38]); //set groupId=3 where id>38
 * */
function db_update($table, $data, $opts=[]){
	if(!isset($table) || empty($data) || !is_hash($data))
		return false;
	$vStrs = [];
	$schema = db_schema($table)["schema"];
	foreach($data as $k=>$v){
		$vStrs[]="`$k`=".db_v($v, $schema[$k]);
	} 
	$vStrs = join(",",$vStrs);
	list($cs,$optStr,$data) = db_make_query($table, $opts,["order","limit","fields"]);
	$sql = "UPDATE $table SET $vStrs ".$optStr;
	return db_query($sql,$data);
}

function db_migrate($table){
	$dbn = Consts::$db_name;
	$sql = sprintf("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = '%s' AND table_name = '%s';", $dbn, $table);
	$res = db_count($sql,[], false);
	if ($res<=0 ){//schema doesn't exist
		$schema_def = db_schema($table);
		$schema = $schema_def["schema"];
		$pk = $schema_def["general"]["pk"];
		$colStmt = "";
		foreach ($schema as $col => $type){
			$colStmt .= "`".$col."` ".$type.", ";
		}
		$incStmt = "";
		$sql = "";
		if (str_has($pk, "|")||str_has($pk, "+")||str_has($pk, ",")){
			$parts = preg_split("/[\|\+,]/", $pk);
			$pkName = join('_',$parts);
			$keys = '`'.join('`,`',$parts)."`";
			$sql = sprintf(FORMAT_CREATE_DB_MULTI_KEY, $dbn, $table, $colStmt, $pkName, $keys, $incStmt);
		}else{
			$sql = sprintf(FORMAT_CREATE_DB, $dbn, $table, $colStmt, $pk, $incStmt);
		}
		$res = db_query($sql);
		
		//index
		$index = $schema_def["index"];
		if(!empty($index)){
			foreach ($index as $cols => $desc){
				$name = "index_".preg_replace('/[\+\.\|]/', '_',$cols);
				$cols = preg_replace('/[\+\.\|]/', '`,`',$cols);
				$desc = $desc=="unique"?"UNIQUE ":" ";
				$sql = "CREATE ${desc}INDEX index_$name ON `$table` (`$cols`)";
				echo $sql."\n";
				db_query($sql);
			}
		}
		
	}
	echo "Created ".$table."</br>\n";
}


/**
 * insert / update a single record
 * @params $tabel:table name
 * @params $data: assoc array , one data record
 * 		insert => new record data, update => changes
 * @params $returnId : whether return new id.
 * @params $bson : compress json field to bson
 * @example :
 	db_save("tasks", [
		"ownerId"	=> 3,
		"title"=> "taskD with bson",
		"data" => ["name"=>"ssss","age"=>18]
	],true,true) //data will be saved as bson.
 * 
 * */
function db_save($table, $data, $returnId=false, $bson=false){
	return pdo_save(db_conn(), $table, $data, $returnId);
}

/**
 * load schema configuration data from SCHEMA_DIR/$schemaName.ini using APC cache.
 * */
function db_schema($schemaName){
	$schemaDef = cache_get("SCHEMA_$schemaName", function($key){
		$schemaname = str_replace("SCHEMA_","",$key);
		$schemaDef = @parse_ini_file(APP_DIR.__SLASH__."conf".__SLASH__."schemas".__SLASH__.$schemaname.".ini", true);
		if(!$schemaDef){
			//error_log("schema=".LIBER_DIR."common/schemas/".$schemaname.".ini");
			$schemaDef = @parse_ini_file(LIBER_DIR."common/schemas/".str_replace("liber_","",$schemaname).".ini", true);
		}
		return $schemaDef;
	},false);
	return $schemaDef;
}

/**TODO test
 * @param $querys: function or array
 * 	@example function
 * 		db_trans(function($dbh){ //must return int;
 * 			$sth = $dbh->exec("DROP TABLE fruit");
 *			$sth = $dbh->exec("UPDATE dessert SET name = 'hamburger'");
 *			$dbh->rollBack();
 *			return 2; //MUST return int !!!
 * 		});
 * 	@example array
 * 		db_trans(["DROP TABLE fruit","UPDATE dessert SET name = 'hamburger'","@rollback"])
 * 
 * 		$rs = db_trans(["update tasks set members='dds' where id=35", "select * from tasks where id=35"]);
		echo json_encode($rs);
 * */
function db_trans($querys,$datas){
	return pdo_trans(db_conn(), $querys,$datas);
}
function bson_enc($arr){
	$str = json_encode($arr);
	$str = str_replace("\\", "", $str);
	return str2hex($str);
}

function bson_dec($bson){
	if(isset($bson)){
		$json = hex2str($bson);
		return json_decode($json,true);
	}
	return false;
}

/**
 * change php value to db value
 * @param $v : php value
 * @param $typeDef : type def in schemas/xxx.ini file. 
 * 			example : "int not null", "bigint", "varchar",  "text"....
 * @param $bsonText : compress text/mediumtext to BSON (binary json)
 * 
 * */
function db_v($v, $typeDef="", $bsonText=false){
	if(!isset($v))
		return "NULL";
	if(is_bool($v))
		return $v ? 1 : 0;
	if (is_array($v)){
		return $bsonText&&(isset($typeDef)&&preg_match("/text/i", $typeDef))? "'".bson_enc($v)."'"
				: "'".mysql_escape_string(json_encode($v))."'";
	}
	if(is_string($v)){
		if(preg_match("/bigint/i", $typeDef) && str_has($v, "-"))
			return strtotime($v);
		if(preg_match("/(int|byte)/i", $typeDef))
			return (int)$v;
		return "'".mysql_escape_string($v)."'";
	} 
	return $v;
}

function db_make_filters($k,$k_operator, $v, $v_operator, &$o, $func_make) {
	$keys = is_array($k)? $k : preg_split('/\|/',$k);
	$values = is_array($v)? $v : preg_split('/\|/',$v);
	$conditions =[]; $idx=0;
	foreach($keys as $_k) {
		$sub_cond = [];
		foreach($values as $_v) {
			$sql = $func_make($_k, $_v, $o, $idx);
			if($sql) $sub_cond[] = $sql;
			$idx++;
		}
		if(!empty($sub_cond))
			$conditions[] = count($values)>1 ? "(". join(" $v_operator ", $sub_cond) .")" : join(" $v_operator ", $sub_cond);
	}
	if(count($conditions) <=0 ) return false;
	return count($conditions)>1 ? "(". join(" $k_operator ", $conditions) .")" : join(" $k_operator ", $conditions);	
}

function db_init_filters(){
	/*Test cases
	 "id"=> 16
	"id@>"=>16,
	"id@<"=>16,
	"id@>="=>16,
	"id@<="=>16,
	"lng@[]"=>[1,3,55],
	"lng@![]"=>[],
	"lng@![]"=>[2,4],
	"lat@()"=>[153, 340],
	"lat@!()"=>[999, 20],
	"title@?"=>"cocoa",
	"title@!?"=>"co%coa",
	"title@~"=>"/^My/",
	"title@~~"=>"^My",
	"title@!~~"=>"'"
	**/

	if(empty(Consts::$db_query_filters))
		Consts::$db_query_filters = [
		"=" 	=> function($k,$v,&$o){
			return db_make_filters($k, "or", $v, "or", $o, function($k,$v, &$o, $idx){
				if($v==="NULL") return "`$k` IS NULL"; else { $_k = $k."_".$idx; $o[$_k]=$v;return "`$k`=:". $_k; }
			});
		},
		"!" 	=> function($k,$v,&$o){
			return db_make_filters($k, "and", $v, "and", $o, function($k,$v, &$o, $idx){
				if($v==="NULL") return "`$k` IS NOT NULL"; else { $_k = $k."_".$idx; $o[$_k]=$v;return "`$k`!=:$_k"; }
			});
		},
		"<" 	=> function($k,$v,&$o){
			return db_make_filters($k, "and", $v, "and", $o, function($k,$v, &$o, $idx){
				$_k = $k."_".$idx; $o[$_k]=$v;return "`$k`<:$_k";
			});
		},
		">" 	=> function($k,$v,&$o){
			return db_make_filters($k, "and", $v, "and", $o, function($k,$v, &$o, $idx){
				$_k = $k."_".$idx; $o[$_k]=$v;return "`$k`>:$_k";
			});
		},
		"<=" 	=> function($k,$v,&$o){
			return db_make_filters($k, "and", $v, "and", $o, function($k,$v, &$o, $idx){
				$_k = $k."_".$idx; $o[$_k]=$v;return "`$k`<=:$_k";
			});
		},
		">=" 	=> function($k,$v,&$o){
			return db_make_filters($k, "and", $v, "and", $o, function($k,$v, &$o, $idx){
				$_k = $k."_".$idx; $o[$_k]=$v;return "`$k`>=:$_k";
			});
		},
		"[]" 	=> function($k,$v,&$o){
			return db_make_filters($k, "and", $v, "and", $o, function($k,$v, &$o, $idx){
				if(is_string($v))$v=explode(',',$v);if(count($v)==0)return false;$vs=array_map(function($e){return db_v($e);},$v);return "`$k` IN (".join(",",$vs).")";
			});
		},
		"![]" 	=> function($k,$v,&$o){
			return db_make_filters($k, "and", $v, "and", $o, function($k,$v, &$o, $idx){
				if(is_string($v))$v=explode(',',$v);if(count($v)==0)return false; $vs=array_map(function($e){return db_v($e);},$v);return "`$k` NOT IN (".join(",",$vs).")";
			});
		},
		"()" 	=> function($k,$v,&$o){
			return db_make_filters($k, "and", $v, "and", $o, function($k,$v, &$o, $idx){
				if(is_string($v))$v=explode(',',$v);if(count($v)!=2)return false; return "(`$k` BETWEEN ".min($v[0],$v[1])." AND ".max($v[0],$v[1]).")";
			});
		},
		"!()" 	=> function($k,$v,&$o){
			return db_make_filters($k, "and", $v, "and", $o, function($k,$v, &$o, $idx){
				if(is_string($v))$v=explode(',',$v);if(count($v)!=2)return false; return "(`$k` NOT BETWEEN ".min($v[0],$v[1])." AND ".max($v[0],$v[1]).")";
			});
		},
		"?"  	=> function($k,$v,&$o){
			return db_make_filters($k, "or", $v, "or", $o, function($k,$v, &$o, $idx){
				if(!str_has($v,"%"))$v="%$v%";return "`$k` LIKE '".preg_replace('/[\+\s]+/','%',$v)."'";
			});
		},
		"!?"  	=> function($k,$v,&$o){
			return db_make_filters($k, "and", $v, "and", $o, function($k,$v, &$o, $idx){
				if(!str_has($v,"%"))$v="%$v%";return "`$k` NOT LIKE '".preg_replace('/[\+\s]+/','%',$v)."'";
			});
		},
		"~" 	=> function($k,$v,&$o){
			return db_make_filters($k, "or", $v, "or", $o, function($k,$v, &$o, $idx){
				$op = Consts::$db_regexp_op[Consts::$db_engine];if(!isset($op))return false;return "`$k` $op '".mysql_escape_string(preg_replace('/^\/|\/$/',"",$v))."'";
			});
		},
		//Regexp only mysql or regexp
		/*	@example db_find("users", ["title@~" => "^My"]) == select * from users where title regexp '^My'
			For MySQL : these are available : ^ , $ , . , [...] , [^...] , p1|p2|p3 , * , + , {n} , {m,n}
		*/
		"!~"	=> function($k,$v,&$o){
			return db_make_filters($k, "or", $v, "or", $o, function($k,$v, &$o, $idx){
				$op = Consts::$db_regexp_op[Consts::$db_engine];if(!isset($op))return false;return "`$k` NOT $op '".mysql_escape_string(preg_replace('/^\/|\/$/',"",$v))."'";
			});
		},
		"~~"	=> function($k,$v,&$o){
			return db_make_filters($k, "or", $v, "or", $o, function($k,$v, &$o, $idx){
				$op = Consts::$db_regexp_op[Consts::$db_engine];if(!isset($op))return false;return "LOWER(`$k`) $op '".mysql_escape_string(preg_replace('/^\/|\/$/',"",$v))."'";
			});
		},
		"!~~"	=> function($k,$v,&$o){
			return db_make_filters($k, "or", $v, "or", $o, function($k,$v, &$o, $idx){
				$op = Consts::$db_regexp_op[Consts::$db_engine];if(!isset($op))return false;return "LOWER(`$k`) NOT $op '".mysql_escape_string(preg_replace('/^\/|\/$/',"",$v))."'";
			});
		},
		//Regexp with ignorecase option only mysql or regexp
	];
}
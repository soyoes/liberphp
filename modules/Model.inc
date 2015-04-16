<?php
/**
 *	@file: Model.inc
 *	@author: Soyoes 2014/01/10
 *	@uses: model class
 *	@example:
 *****************************************************************************/

class Model{
	
	var $db;
	var $pk;		//primary key
	var $table;		//table name
	var $schema;	//array
	var $restricts;	//array migration only
	var $indexes;	//array migration only
	
	private $data=[];
	private $changes;
	
	private function __construct(){}
	
	/**
	 * 	Get a new model instance 
	 * 	@param $schemaName : null -> binds nothing, tablename -> binds a certain table name
	 *  @return : Model
	 */
	public static function factory($schemaName){
		try{
			if(!isset($schemaName))
				throw Exception("Model must have a schema name");
			$model = new Model();
	        $db = null;
	        if(str_has($schemaName, '.')){
	        	list($db, $schemaName) = explode('.', $schemaName);
	        	$model->db = $db;
	        } 
	        //$model->addObserver($model);
			if(isset($schemaName) && $db==null)
				$model->loadSchema($schemaName);
			return $model;
		}catch(Exception $e){
			throw new Exception($schemaName.'Model not found');
		}
	}
	
	public function set($keyPath, $val){
		$change = hash_set($this->data, $keyPath, $val);
		//$this->notify($change);
		if(isset($this->schema[$change["key"]])){
			$this->changes[$change['key']] = $change["val"];
		}
	}
	
	public function get($keyPath){
		return hash_get($this->data, $keyPath);
	}
	
	/**
	 * @param $schemaname	schema def file name(without .ini) under conf/schemas
	 */
	public function loadSchema($schemaname){
		try{
			$schemaDef = db_schema($schemaname);
			$general = $schemaDef["general"];
			if(isset($general["db"])) $this->db = $general["db"];
			$this->pk	 	= $general["pk"];
			$this->table	= $general["name"];
			if(isset($general["index"])) $this->indexes = $general["index"];
			$this->schema 	= $schemaDef["schema"];
		}catch(Exception $e){
			throw new Exception($schemaName.' schema file not found');
		}
	}
	
	/**
	 * insert many records to DB
	 * @param $datas : hash list
	 * @param table:tablename
	 * */
	public function import($datas, $sql_dump_path=null){
		return db_import($this->table, $datas, $sql_dump_path);
	}

	/**
	 * search by conditions
	 * @param array or string $opts
	 * 		if opts is Array : reserved keywords in opts
	 * 		* fields = default = * ||  id,pass,... || [id, pass, name ...]
	 * 		* limit : 0/default=all, 0,20=select 0~20, 21~80= select from 21 get 60 records 
	 * 		* order : default = "" || id desc || updAt desc
	 * 		* useCache : false | true
	 * @return array or false
	 */
	public function find($opts=array(),$withCount=false){
		return db_find($this->table,$opts,$withCount);
	}
	
	
	/**
	 * get a single record from db by pk
	 * @param string $id		:pk value
	 * @param boolean $useCache	:true = use memcache
	 * @param string $cols :	cols to fetch
	 * @return Entity 
	 */
	public function fetch($id, $useCache=false, $cols="*",$pkName=null){
		try {
			if($pkName==null)	
				$pkName = $this->pk;
			if(!isset($this->table))
				return null;
			if($cols==null) $cols ="*";
			$sql = "SELECT ".$cols." FROM ".$this->table." WHERE `".$pkName."`=:$pkName LIMIT 1";
			error_log($sql);
			$res = db_query($sql,[$pkName=>$id],$useCache);
		    $data = $res[0];
		    $this->data = $data;
		    return $data;
		} catch (PDOException $e) {
			error_log($sql);
		    return null;
		}		
	}
	public function find1st($opts=array()){
		return db_find1st($this->table,$opts);
	}
	
	public function exists($pk){
		return db_exists($this->table,$pk);
	}

	/**
	 * insert or update by checking pk
	 * @param boolean $useCache
	 */
	public function save($data=null, $returnId=false){
		if (isset($data))
			foreach ($data as $k => $v){
				$this->set($k, $v);
			}
		$id = isset($this->data[$this->pk]) ? $this->data[$this->pk] : null;
		$isUpdate = isset($id);
		$d = $isUpdate?$this->changes:$this->data;
		if($isUpdate)$d[$this->pk] = $id;
		return db_save($this->table, $d, $returnId);
	}
	
	/**
	 *delete itself from db 
	 */
	public function delete(){
		if(empty($this->data)) return false;
		$id = $this->data[$this->pk];
		if(empty($id)) return false;
		$sql = "delete from ".$this->table." where `".$this->pk."`=:id";
		return db_query($sql, ["id"=>$id]);
	}
	
}

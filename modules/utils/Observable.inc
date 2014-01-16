<?php

class Observable{
	
  	/**
  	 * @var array: all observers list
  	 */
  	var $observers;
  	var $changes;
  	
  	var $data=array();
  	
	/**
	 * @param Observer $obj
	 * add a observer to $mObservers
	 */
	public function addObserver(Observer $obj){
		if (!isset($this->observers))
			$this->observers=array();
		//TODO:check this
		$this->observers[] = &$obj;	
	}

  	/**
  	 * notify all 
  	 */
  	public function notify($data){
  		if (isset($this->observers)){
  			foreach ($this->observers as $observer){
  				$observer->update($this, $data); 
  			}
  		}	
  	}
  	
  	public function set($keyPath, $val){
  		if(!isset($this->data)) 
  			$this->data = array();
  		$change = Arrays::set($this->data, $keyPath, $val);
		$this->notify($change);
  	}
  	
}


?>